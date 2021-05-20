#include "ClientTestFixture.h"

UPLOAD_HANDLE StreamingSession::addNewConsumerSession(MockConsumerConfig consumerConfig,
                                                      STREAM_HANDLE mStreamHandle,
                                                      UPLOAD_HANDLE uploadHandleOverride) {
    if (IS_VALID_UPLOAD_HANDLE(uploadHandleOverride)) {
        mUploadHandleCounter = uploadHandleOverride;
    }
    UPLOAD_HANDLE currentHandle = mUploadHandleCounter;
    MockConsumer *mockConsumer = new MockConsumer(consumerConfig,
                                                  (UPLOAD_HANDLE) currentHandle,
                                                  mStreamHandle);
    mConsumerList[currentHandle] = mockConsumer;
    mUploadHandleCounter++;
    return currentHandle;
}

void StreamingSession::removeConsumerSession(UPLOAD_HANDLE uploadHandle) {
    if (mConsumerList.count(uploadHandle) != 0) {
        delete mConsumerList[uploadHandle];
        mConsumerList.erase(uploadHandle);
    }
}

void StreamingSession::clearSessions() {
    for(std::map<UPLOAD_HANDLE, MockConsumer*>::iterator it = mConsumerList.begin(); it != mConsumerList.end(); it++) {
        delete it->second;
    }
    mConsumerList.clear();
    mUploadHandleCounter = 0;
}

void StreamingSession::signalDataAvailable(UPLOAD_HANDLE uploadHandle) {
    if (mConsumerList.count(uploadHandle) != 0) {
        mConsumerList[uploadHandle]->dataAvailable();
    }
}

/*
 * Fills a vector with active upload handles. If any upload handles' connection was closed due to error,
 * they will be removed and wont be returned.
 */
void StreamingSession::getActiveUploadHandles(std::vector<UPLOAD_HANDLE> &activeUploadHandles) {
    activeUploadHandles.clear();
    UINT64 lastTransferDataTime = INVALID_TIMESTAMP_VALUE;
    std::map<UPLOAD_HANDLE, MockConsumer*>::iterator it = mConsumerList.begin();
    while (it != mConsumerList.end()) {
        std::map<UPLOAD_HANDLE, MockConsumer*>::iterator curr = it++;
        if (curr->second->isConnectionClosed()) {
            delete curr->second;
            mConsumerList.erase(curr);
        } else if (curr != mConsumerList.end()) {
            activeUploadHandles.push_back(curr->first);

            // The loop steps from oldest upload handle to the latest. The timestamp when upload handle
            // last received data should be monotonically increasing between upload handles.
            if (IS_VALID_TIMESTAMP(lastTransferDataTime) &&
                IS_VALID_TIMESTAMP(curr->second->mLastGetStreamDataTime)) {
                EXPECT_TRUE(lastTransferDataTime < curr->second->mLastGetStreamDataTime);
            }

            lastTransferDataTime = curr->second->mLastGetStreamDataTime;
        }
    }
}

MockConsumer *StreamingSession::getConsumer(UPLOAD_HANDLE uploadHandle) {
    MockConsumer *mockConsumer = NULL;
    if (mConsumerList.count(uploadHandle) != 0) {
        mockConsumer = mConsumerList[uploadHandle];
    }

    return mockConsumer;
}
