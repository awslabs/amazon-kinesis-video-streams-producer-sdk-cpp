#ifndef __MOCK_CONSUMER_H__
#define __MOCK_CONSUMER_H__

typedef struct {
    UINT32 mDataBufferSizeByte;
    UINT64 mUploadSpeedBytesPerSecond;
    UINT64 mBufferingAckDelayMs;
    UINT64 mReceiveAckDelayMs;
    UINT64 mPersistAckDelayMs;
    BOOL mEnableAck;
    UINT64 mRetention;
} MockConsumerConfig;

// struct to keep track of an FragmentAck struct and when to submit the FragmentAck
typedef struct {
    FragmentAck mFragmentAck;
    UINT64 mAckTimestamp;
} AckItem;

class AckItemComparator {
  public:
    bool operator()(const AckItem& lhs, const AckItem& rhs) const
    {
        return lhs.mAckTimestamp > rhs.mAckTimestamp;
    }
};

// Mimics a putMedia session that calls getKinesisVideoStreamData and kinesisVideoStreamFragmentAck periodlically
class MockConsumer {
    volatile ATOMIC_BOOL mDataAvailable; // where getKinesisVideoStreamData should be called.
    BOOL mCurrentInitialized;
    STREAM_HANDLE mStreamHandle;
    UINT64 mOldCurrent; // stores the old current before getKinesisVideoStreamData call
    UINT64 mFragmentTimestamp;

    UINT64 mSendDataDelay;     // send data delay in MS
    UINT64 mBufferingAckDelay; // buffering ack delay in MS
    UINT64 mReceiveAckDelay;   // received ack delay in MS
    UINT64 mPersistAckDelay;   // persisted ack delay in MS

    BOOL mEnableAck;
    UINT64 mRetention;

    UINT64 mNextGetStreamDataTime;
    // upload handle for this consumer
    FragmentAck mFragmentAck; // FragmentAck container
    BOOL mConnectionClosed;   // whether error has happend, in which case connection is closed

    // create and add AckItem to the mAckQueue. Generate proper ackTimestamp for each FragmentAck
    void enqueueAckItem(UINT64 fragmentTimestamp, PUINT64 submitAckTime);

    /*
     * This should be called once after the first getKinesisVideoStreamData call for each new consumer session.
     * initOldCurrent sets up mOldCurrent to point to the view item that this session starts at, so that createAckEvent
     * can create acks with proper timestamps.
     */
    void initOldCurrent();

    /*
     * After getKinesisVideoStreamData, traverse the content view from old current to current. At each view item that has
     * ITEM_FLAG_FRAGMENT_START flag, store its ackTimestamp in mFragmentTimestamp. If mFragmentTimestamp is valid, it means
     * the previous fragment is seen as "sent" to the backend, therefore call enqueueAckItem to queue up fragment acks.
     * if isEosSent is TRUE, queue up acks for the last fragment. The resulting ack timeline should look like follows:
     *
     * |---------------------------------------------------------------------------------------------------------------|
     * |                      |           |              |              |               |                   |
     * getStreamDataTime      |           |              |              |               |                   |
     *                   mSendDataDelay   |              |              |               |                   |
     *                            mBufferingAckDelay     |              |               |                   |
     *                           (buffering ack for f1)  |              |               |                   |
     *                                             mReceiveAckDelay     |               |                   |
     *                                             (receive ack for f1) |               |                   |
     *                                                            mPersistAckDelay      |                   |
     *                                                            (persisted ack for f1)|                   |
     *                                                                          mBufferingAckDelay          |
     *                                                                          (buffering ack for f2)     ......
     */
    void createAckEvent(BOOL isEosSent, UINT64 sendAckTime);

    void purgeAckItemWithTimestamp(UINT64 ackTimestamp);

  public:
    UINT64 mUploadSpeed; // bytes per second
    std::priority_queue<AckItem, std::vector<AckItem>, AckItemComparator> mAckQueue;
    UPLOAD_HANDLE mUploadHandle;
    PBYTE mDataBuffer;
    UINT32 mDataBufferSize;
    UINT64 mLastGetStreamDataTime;

    MockConsumer(MockConsumerConfig config, UPLOAD_HANDLE mUploadHandle, STREAM_HANDLE mStreamHandle);

    ~MockConsumer()
    {
        MEMFREE(mDataBuffer);
    }

    /**
     * set mDataAvailable to TRUE, if mDataAvailable is FALSE, timedGetStreamData would do nothing.
     */
    void dataAvailable()
    {
        ATOMIC_STORE_BOOL(&mDataAvailable, TRUE);
    }

    /**
     * time based getKinesisVideoStreamData
     *
     * @param 1 UINT64 - Current time.
     * @param 2 PBOOL - Whether getKinesisVideoStreamData was called.
     * @param 3 PUINT32 - If getKinesisVideoStreamData was called, retrievedSize will be set to the retrieved size.
     *
     * @return STATUS code of getKinesisVideoStreamData if it happened, otherwise STATUS_SUCCESS
     */
    STATUS timedGetStreamData(UINT64 currentTime, PBOOL pDidGetStreamData, PUINT32 pRetrievedSize = NULL);

    /**
     * time based calling kinesisVideoStreamFragmentAck
     *
     * @param 1 UINT64 - Current time.
     * @param 2 PBOOL - Whether getKinesisVideoStreamData was called.
     *
     * @return STATUS code of getKinesisVideoStreamData if it happened, otherwise STATUS_SUCCESS
     */
    STATUS timedSubmitNormalAck(UINT64 currentTime, PBOOL pSubmittedAck);

    /**
     * attempt to submit an error ack with service_call_result
     *
     * @param 1 SERVICE_CALL_RESULT - error call result.
     * @param 2 PBOOL - Whether error ack was submited.
     *
     * @return STATUS code of kinesisVideoStreamFragmentAck if it happened, otherwise STATUS_SUCCESS
     */
    STATUS submitErrorAck(SERVICE_CALL_RESULT service_call_result, PBOOL pSubmittedAck);

    /**
     * attempt to submit an normal ack on given timestamp with service_call_result
     *
     * @param 1 SERVICE_CALL_RESULT - error call result.
     * @param 2 FRAGMENT_ACK_TYPE - fragment ack type
     * @param 3 UINT64 - timestamp of the ack
     * @param 4 PBOOL - Whether error ack was submited.
     *
     * @return STATUS code of kinesisVideoStreamFragmentAck if it happened, otherwise STATUS_SUCCESS
     */
    STATUS submitNormalAck(SERVICE_CALL_RESULT service_call_result, FRAGMENT_ACK_TYPE ackType, UINT64 timestamp, PBOOL pSubmittedAck);

    /**
     * Call kinesisVideoStreamTerminated immediately because connection error can happen at any time
     *
     * @param 1 SERVICE_CALL_RESULT - error call result.
     *
     * @return STATUS code of kinesisVideoStreamTerminated call.
     */
    STATUS submitConnectionError(SERVICE_CALL_RESULT service_call_result);

    /**
     * Return whether connection has been closed
     *
     * @return BOOL whether connection has been closed
     */
    BOOL isConnectionClosed()
    {
        return mConnectionClosed;
    }

    UINT32 submitErrorAck(SERVICE_CALL_RESULT service_call_result, UINT64 timestamp, PBOOL pSubmittedAck);
};

#endif //__MOCK_CONSUMER_H__
