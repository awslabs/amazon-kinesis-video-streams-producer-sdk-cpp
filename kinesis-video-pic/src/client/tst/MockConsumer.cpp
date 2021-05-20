#include "ClientTestFixture.h"

MockConsumer::MockConsumer(MockConsumerConfig config,
                           UPLOAD_HANDLE mUploadHandle,
                           STREAM_HANDLE mStreamHandle)
        : mOldCurrent(0),
          mDataBufferSize(config.mDataBufferSizeByte),
          mUploadSpeed(config.mUploadSpeedBytesPerSecond),
          mNextGetStreamDataTime(0),
          mUploadHandle(mUploadHandle),
          mStreamHandle(mStreamHandle),
          mBufferingAckDelay(config.mBufferingAckDelayMs),
          mReceiveAckDelay(config.mReceiveAckDelayMs),
          mPersistAckDelay(config.mPersistAckDelayMs),
          mCurrentInitialized(FALSE),
          mConnectionClosed(FALSE),
          mEnableAck(config.mEnableAck),
          mRetention(config.mRetention),
          mFragmentTimestamp(INVALID_TIMESTAMP_VALUE),
          mLastGetStreamDataTime(INVALID_TIMESTAMP_VALUE) {
    ATOMIC_STORE_BOOL(&mDataAvailable, FALSE);

    // init FragmentAck struct
    mFragmentAck.timestamp = INVALID_TIMESTAMP_VALUE;
    mFragmentAck.result = SERVICE_CALL_RESULT_OK;
    STRCPY(mFragmentAck.sequenceNumber, "SequenceNumber");
    mFragmentAck.version = FRAGMENT_ACK_CURRENT_VERSION;

    mDataBuffer = (PBYTE) MEMALLOC(mDataBufferSize);
}

void MockConsumer::enqueueAckItem(UINT64 fragmentTimestamp, PUINT64 submitAckTime) {
    AckItem ackItem;
    mFragmentAck.timestamp = fragmentTimestamp;
    mFragmentAck.result = SERVICE_CALL_RESULT_OK;
    mFragmentAck.ackType = FRAGMENT_ACK_TYPE_BUFFERING;
    *submitAckTime += mBufferingAckDelay * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    ackItem.mFragmentAck = mFragmentAck;
    ackItem.mAckTimestamp = *submitAckTime;
    mAckQueue.push(ackItem);

    mFragmentAck.ackType = FRAGMENT_ACK_TYPE_RECEIVED;
    *submitAckTime += mReceiveAckDelay * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    ackItem.mFragmentAck = mFragmentAck;
    ackItem.mAckTimestamp = *submitAckTime;
    mAckQueue.push(ackItem);

    if (mRetention != 0) {
        mFragmentAck.ackType = FRAGMENT_ACK_TYPE_PERSISTED;
        *submitAckTime += mPersistAckDelay * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
        ackItem.mFragmentAck = mFragmentAck;
        ackItem.mAckTimestamp = *submitAckTime;
        mAckQueue.push(ackItem);
    }
}

void MockConsumer::initOldCurrent() {
    STATUS retStatus = STATUS_SUCCESS;
    PKinesisVideoClient pKinesisVideoClient = NULL;
    PViewItem pViewItem;

    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(mStreamHandle);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;

    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);

    // Traverse back from current to look for the view item that has the STREAM_START_DEBUG flag.
    mOldCurrent = pKinesisVideoStream->curViewItem.viewItem.index;
    pViewItem = &pKinesisVideoStream->curViewItem.viewItem;

    while (!CHECK_ITEM_STREAM_START_DEBUG(pViewItem->flags) && STATUS_SUCCEEDED(retStatus)) {
        mOldCurrent--;
        retStatus = contentViewGetItemAt(pKinesisVideoStream->pView, mOldCurrent, &pViewItem);
    }

    EXPECT_EQ(STATUS_SUCCESS, retStatus);
    EXPECT_TRUE(CHECK_ITEM_STREAM_START_DEBUG(pViewItem->flags));

    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);

    mCurrentInitialized = TRUE;
}

void MockConsumer::createAckEvent(BOOL isEosSent, UINT64 sendAckTime) {
    PKinesisVideoClient pKinesisVideoClient = NULL;
    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(mStreamHandle);
    pKinesisVideoClient = pKinesisVideoStream->pKinesisVideoClient;
    pKinesisVideoClient->clientCallbacks.lockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
    PViewItem pViewItem = NULL;
    UINT64 endIndex = pKinesisVideoStream->curViewItem.viewItem.index;
    STATUS status = STATUS_SUCCESS;
    if (pKinesisVideoStream->curViewItem.offset == pKinesisVideoStream->curViewItem.viewItem.length) {
        endIndex++;
    }

    for(UINT64 i = mOldCurrent; i < endIndex; i++) {
        /* not expecting contentViewGetItemAt to always succeed because it may fail in the negative scenario tests.
         * For example view items were deliberately dropped. */
        status = contentViewGetItemAt(pKinesisVideoStream->pView, i, &pViewItem);
        if (STATUS_SUCCEEDED(status) && pViewItem != NULL && !CHECK_ITEM_SKIP_ITEM(pViewItem->flags)) {
            if (CHECK_ITEM_FRAGMENT_START(pViewItem->flags)) {
                if (IS_VALID_TIMESTAMP(mFragmentTimestamp)) {
                    enqueueAckItem(mFragmentTimestamp, &sendAckTime);
                }
                mFragmentTimestamp = pViewItem->ackTimestamp / HUNDREDS_OF_NANOS_IN_A_MILLISECOND; // convert to MKV timecode
            } else if (CHECK_ITEM_FRAGMENT_END(pViewItem->flags)) {
                enqueueAckItem(mFragmentTimestamp, &sendAckTime);
                mFragmentTimestamp = INVALID_TIMESTAMP_VALUE;
            }
        }
    }
    // At eos, queue up last mFragmentTimestamp as there wont be a next one to trigger it.
    if (isEosSent) {
        enqueueAckItem(mFragmentTimestamp, &sendAckTime);
    }

    // record the current
    mOldCurrent = endIndex;

    pKinesisVideoClient->clientCallbacks.unlockMutexFn(pKinesisVideoClient->clientCallbacks.customData, pKinesisVideoStream->base.lock);
}

void MockConsumer::purgeAckItemWithTimestamp(UINT64 ackTimestamp)
{
    UINT32 ackPurged = 0;
    std::priority_queue<AckItem, std::vector<AckItem>, AckItemComparator> tempAckQueue;
    AckItem ackItem;

    if (mAckQueue.empty()) {
        return;
    }

    do {
        ackItem = mAckQueue.top();
        if (ackItem.mFragmentAck.timestamp == ackTimestamp) {
            ackPurged++;
            DLOGI("purge ack %llu", ackItem.mFragmentAck.timestamp);
        } else {
            tempAckQueue.push(ackItem);
        }
        mAckQueue.pop();
    } while (ackPurged < 3 && !mAckQueue.empty());

    while (!tempAckQueue.empty()) {
        mAckQueue.push(tempAckQueue.top());
        tempAckQueue.pop();
    }
}

STATUS MockConsumer::timedGetStreamData(UINT64 currentTime, PBOOL pDidGetStreamData, PUINT32 pRetrievedSize) {
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 actualDataSize;
    *pDidGetStreamData = FALSE;

    if (ATOMIC_LOAD_BOOL(&mDataAvailable) && (currentTime >= mNextGetStreamDataTime)) {
        *pDidGetStreamData = TRUE;
        retStatus = getKinesisVideoStreamData(mStreamHandle, mUploadHandle, mDataBuffer, mDataBufferSize,
                                              &actualDataSize);

        // stop calling getKinesisVideoStreamData if there is no more data.
        if (retStatus == STATUS_NO_MORE_DATA_AVAILABLE || retStatus == STATUS_AWAITING_PERSISTED_ACK) {
            ATOMIC_STORE_BOOL(&mDataAvailable, FALSE);
        }

        if (actualDataSize > 0) {
            mLastGetStreamDataTime = currentTime;
        }

        if (pRetrievedSize != NULL) {
            *pRetrievedSize = actualDataSize;
        }

        mSendDataDelay = (UINT64) ((DOUBLE) actualDataSize / mUploadSpeed * HUNDREDS_OF_NANOS_IN_A_SECOND);
        DLOGD("wrote %llu bytes for upload handle %llu", actualDataSize, mUploadHandle);
        if (retStatus != STATUS_UPLOAD_HANDLE_ABORTED && actualDataSize != 0) {
            // initialize mOldCurrent after first getKinesisVideoStreamData call.
            // ActualDataSize has to be non zero so that at least one frame has been consumed, in which case
            // a view item with ITEM_FLAG_STREAM_START_DEBUG must exist. Also when retStatus is STATUS_UPLOAD_HANDLE_ABORTED,
            // actualDataSize should also be zero.
            if (!mCurrentInitialized) {
                initOldCurrent();
            }

            createAckEvent(retStatus == STATUS_AWAITING_PERSISTED_ACK, currentTime + mSendDataDelay);
        }

        mNextGetStreamDataTime = currentTime + mSendDataDelay;
    }

    return retStatus;
}

STATUS MockConsumer::timedSubmitNormalAck(UINT64 currentTime, PBOOL pSubmittedAck) {
    STATUS retStatus = STATUS_SUCCESS;
    *pSubmittedAck = FALSE;
    AckItem ackItem;

    EXPECT_NE(mConnectionClosed, TRUE);

    CHK(!mAckQueue.empty(), STATUS_SUCCESS);

    // ack items in the queue are ordered by time, so just need to check the front
    ackItem = mAckQueue.top();
    if (currentTime >= ackItem.mAckTimestamp) {
        if (mEnableAck) {
            CHK_STATUS(kinesisVideoStreamFragmentAck(mStreamHandle, mUploadHandle, &ackItem.mFragmentAck));
            *pSubmittedAck = TRUE;

            switch(ackItem.mFragmentAck.ackType) {
                case FRAGMENT_ACK_TYPE_PERSISTED:
                    DLOGD("got persisted ack. timestamp %llu", ackItem.mFragmentAck.timestamp);
                    break;
                case FRAGMENT_ACK_TYPE_RECEIVED:
                    DLOGD("got received ack. timestamp %llu", ackItem.mFragmentAck.timestamp);
                    break;
                case FRAGMENT_ACK_TYPE_BUFFERING:
                    DLOGD("got buffering ack. timestamp %llu", ackItem.mFragmentAck.timestamp);
                    break;
                default:
                    break;
            }
        }

        // pop the ack item.
        mAckQueue.pop();
    }

CleanUp:
    return retStatus;
}

STATUS MockConsumer::submitErrorAck(SERVICE_CALL_RESULT service_call_result, PBOOL pSubmittedAck) {
    STATUS retStatus = STATUS_SUCCESS;
    *pSubmittedAck = FALSE;

    EXPECT_NE(mConnectionClosed, TRUE);

    // Error ack needs to have a fragment timestamp.
    if (!mAckQueue.empty()) {
        *pSubmittedAck = TRUE;
        mFragmentAck.result = service_call_result;
        mFragmentAck.ackType = FRAGMENT_ACK_TYPE_ERROR;
        mFragmentAck.timestamp = mAckQueue.top().mFragmentAck.timestamp;
        purgeAckItemWithTimestamp(mFragmentAck.timestamp);
        DLOGD("submitting error ack with call result %u, timestamp %" PRIu64 " to upload handle %" PRIu64,
              service_call_result, mFragmentAck.timestamp, mUploadHandle);
        retStatus = kinesisVideoStreamFragmentAck(mStreamHandle, mUploadHandle, &mFragmentAck);
    }

    return retStatus;
}

STATUS MockConsumer::submitErrorAck(SERVICE_CALL_RESULT service_call_result, UINT64 timestamp, PBOOL pSubmittedAck) {
    STATUS retStatus = STATUS_SUCCESS;
    *pSubmittedAck = FALSE;

    EXPECT_NE(mConnectionClosed, TRUE);

    *pSubmittedAck = TRUE;
    mFragmentAck.result = service_call_result;
    mFragmentAck.ackType = FRAGMENT_ACK_TYPE_ERROR;
    mFragmentAck.timestamp = timestamp;
    purgeAckItemWithTimestamp(mFragmentAck.timestamp);
    DLOGD("No ackItem used. submitting error ack with call result %u, timestamp %" PRIu64 " to upload handle %" PRIu64,
          service_call_result, mFragmentAck.timestamp, mUploadHandle);
    retStatus = kinesisVideoStreamFragmentAck(mStreamHandle, mUploadHandle, &mFragmentAck);

    return retStatus;
}

STATUS MockConsumer::submitNormalAck(SERVICE_CALL_RESULT service_call_result, FRAGMENT_ACK_TYPE ackType,
        UINT64 timestamp, PBOOL pSubmittedAck) {
    STATUS retStatus = STATUS_SUCCESS;
    *pSubmittedAck = FALSE;

    EXPECT_NE(mConnectionClosed, TRUE);

    // ack needs to have a fragment timestamp.
    *pSubmittedAck = TRUE;
    mFragmentAck.result = service_call_result;
    mFragmentAck.ackType = ackType;
    mFragmentAck.timestamp = timestamp;
    retStatus = kinesisVideoStreamFragmentAck(mStreamHandle, mUploadHandle, &mFragmentAck);

    return retStatus;
}

STATUS MockConsumer::submitConnectionError(SERVICE_CALL_RESULT service_call_result) {
    mConnectionClosed = TRUE;

    // connection error can happen at any time.
    return kinesisVideoStreamTerminated(mStreamHandle, mUploadHandle, service_call_result);
}
