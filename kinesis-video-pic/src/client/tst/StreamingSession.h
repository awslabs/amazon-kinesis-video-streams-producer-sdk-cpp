#ifndef __CONSUMER_SESSION_MANAGER_H__
#define __CONSUMER_SESSION_MANAGER_H__

class StreamingSession {
  public:
    std::map<UPLOAD_HANDLE, MockConsumer*> mConsumerList;
    std::vector<UPLOAD_HANDLE> mActiveUploadHandles;
    UPLOAD_HANDLE mUploadHandleCounter;

    StreamingSession() : mUploadHandleCounter(0)
    {
    }

    /**
     * Initialize a MockConsumer, assign it a upload handle and store the pair in mConsumerList.
     * @param consumerConfig is used to initialize MockConsumer
     * @param mStreamHandle is used to initialize MockConsumer
     * @param uploadHandleOverride is used to override the upload handle counter
     * @return
     */
    UPLOAD_HANDLE addNewConsumerSession(MockConsumerConfig consumerConfig, STREAM_HANDLE mStreamHandle,
                                        UPLOAD_HANDLE uploadHandleOverride = INVALID_UPLOAD_HANDLE_VALUE);

    /**
     * Look up the MockConcumer mapped to uploadHandle, free its MockConsumer if it exists and remove uploadHandle from
     * mConsumerList
     * @param uploadHandle
     */
    void removeConsumerSession(UPLOAD_HANDLE uploadHandle);

    /**
     * Clear the mConsumerList map and free all MockConsumers.
     */
    void clearSessions();

    /**
     * Look up the MockConcumer mapped to uploadHandle and call its dataAvailable method.
     * @param uploadHandle
     */
    void signalDataAvailable(UPLOAD_HANDLE uploadHandle);

    /**
     * Fills a vector with active upload handles. If any upload handles' connection was closed due to error,
     * they will be removed and wont be in the vector returned.
     */
    void getActiveUploadHandles(std::vector<UPLOAD_HANDLE>& activeUploadHandles);

    /**
     * @param uploadHandle
     * @return MockConsumer* that is mapped to uploadHandle. NULL if uploadHandle does not exist.
     */
    MockConsumer* getConsumer(UPLOAD_HANDLE uploadHandle);

    ~StreamingSession()
    {
        clearSessions();
    }
};

#endif //__CONSUMER_SESSION_MANAGER_H__
