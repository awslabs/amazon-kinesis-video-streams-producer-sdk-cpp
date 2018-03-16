/**
 * Implementation of Kinesis Video Producer client wrapper
 */
#define LOG_CLASS "KinesisVideoClientWrapper"
#include "com/amazonaws/kinesis/video/producer/jni/KinesisVideoClientWrapper.h"

KinesisVideoClientWrapper::KinesisVideoClientWrapper(JNIEnv* env,
                                         jobject thiz,
                                         jobject deviceInfo) : mClientHandle(INVALID_CLIENT_HANDLE_VALUE),
                                                               mJvm(NULL),
                                                               mGlobalJniObjRef(NULL)
{
    UINT32 retStatus;

    CHECK(env != NULL && thiz != NULL && deviceInfo != NULL);

    // Get and store the JVM so the callbacks can use it later
    if (env->GetJavaVM(&mJvm) != 0) {
        CHECK_EXT(FALSE, "Couldn't retrieve the JavaVM reference.");
    }

    // Set the callbacks
    if (!setCallbacks(env, thiz)) {
        throwNativeException(env, EXCEPTION_NAME, "Failed to set the callbacks.", retStatus);
        return;
    }

    // Extract the DeviceInfo structure
    if (!setDeviceInfo(env, deviceInfo, &mDeviceInfo)) {
        throwNativeException(env, EXCEPTION_NAME, "Failed to set the DeviceInfo structure.", retStatus);
        return;
    }

    // Creating the client object might return an error as well so freeing potentially allocated tags right after the call.
    retStatus = createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &mClientHandle);
    releaseTags(mDeviceInfo.tags);
    if (STATUS_FAILED(retStatus)) {
        throwNativeException(env, EXCEPTION_NAME, "Failed to create Kinesis Video client.", retStatus);
        return;
    }

    // Auxiliary initialization
    mAuthInfo.version = AUTH_INFO_CURRENT_VERSION;
    mAuthInfo.size = 0;
    mAuthInfo.expiration = 0;
    mAuthInfo.type = AUTH_INFO_UNDEFINED;
}

KinesisVideoClientWrapper::~KinesisVideoClientWrapper()
{
    STATUS retStatus = STATUS_SUCCESS;
    if (IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        if (STATUS_FAILED(retStatus = freeKinesisVideoClient(&mClientHandle))) {
            DLOGE("Failed to free the producer client object");
            JNIEnv *env;
            mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
            throwNativeException(env, EXCEPTION_NAME, "Failed to free the producer client object.", retStatus);
            return;
        }
    }
}

void KinesisVideoClientWrapper::deleteGlobalRef(JNIEnv* env)
{
    // Release the global reference to JNI object
    if (mGlobalJniObjRef != NULL) {
        env->DeleteGlobalRef(mGlobalJniObjRef);
    }
}

jobject KinesisVideoClientWrapper::getGlobalRef()
{
    return mGlobalJniObjRef;
}

void KinesisVideoClientWrapper::stopKinesisVideoStreams()
{
    STATUS retStatus = STATUS_SUCCESS;
    if (!IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        DLOGE("Invalid client object");
        JNIEnv *env;
        mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
        throwNativeException(env, EXCEPTION_NAME, "Invalid call after the client is freed.", STATUS_INVALID_OPERATION);
        return;
    }

    if (STATUS_FAILED(retStatus = ::stopKinesisVideoStreams(mClientHandle)))
    {
        DLOGE("Failed to stop the streams with status code 0x%08x", retStatus);
        JNIEnv *env;
        mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
        throwNativeException(env, EXCEPTION_NAME, "Failed to stop the streams.", retStatus);
        return;
    }
}

void KinesisVideoClientWrapper::stopKinesisVideoStream(jlong streamHandle)
{
    STATUS retStatus = STATUS_SUCCESS;
    JNIEnv *env;
    mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);

    if (!IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        DLOGE("Invalid client object");
        throwNativeException(env, EXCEPTION_NAME, "Invalid call after the client is freed.", STATUS_INVALID_OPERATION);
        return;
    }

    if (!IS_VALID_STREAM_HANDLE(streamHandle))
    {
        DLOGE("Invalid stream handle 0x%016" PRIx64, (UINT64) streamHandle);
        throwNativeException(env, EXCEPTION_NAME, "Invalid stream handle.", STATUS_INVALID_OPERATION);
        return;
    }

    if (STATUS_FAILED(retStatus = ::stopKinesisVideoStream(streamHandle)))
    {
        DLOGE("Failed to stop kinesis video stream with status code 0x%08x", retStatus);
        throwNativeException(env, EXCEPTION_NAME, "Failed to stop kinesis video stream.", retStatus);
        return;
    }
}

void KinesisVideoClientWrapper::getKinesisVideoMetrics(jobject kinesisVideoMetrics)
{
    STATUS retStatus = STATUS_SUCCESS;
    JNIEnv *env;
    mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);

    if (!IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        DLOGE("Invalid client object");
        throwNativeException(env, EXCEPTION_NAME, "Invalid call after the client is freed.", STATUS_INVALID_OPERATION);
        return;
    }

    if (NULL == kinesisVideoMetrics)
    {
        DLOGE("KinesisVideoMetrics object is null");
        throwNativeException(env, EXCEPTION_NAME, "KinesisVideoMetrics object is null.", STATUS_NULL_ARG);
        return;
    }

    ClientMetrics metrics;
    metrics.version = CLIENT_METRICS_CURRENT_VERSION;

    if (STATUS_FAILED(retStatus = ::getKinesisVideoMetrics(mClientHandle, &metrics)))
    {
        DLOGE("Failed to get KinesisVideoMetrics with status code 0x%08x", retStatus);
        throwNativeException(env, EXCEPTION_NAME, "Failed to get KinesisVideoMetrics.", retStatus);
        return;
    }

    //get the class
    jclass metricsClass = env->GetObjectClass(kinesisVideoMetrics);
    if (metricsClass == NULL){
        DLOGE("Failed to get metrics class object");
        throwNativeException(env, EXCEPTION_NAME, "Failed to get metrics class object.", STATUS_INVALID_OPERATION);
        return;
    }

    // Set the Java object
    jmethodID setterMethodId = env->GetMethodID(metricsClass, "setMetrics", "(JJJJJJ)V");
    if (setterMethodId == NULL)
    {
        DLOGE("Failed to get the setter method id.");
        throwNativeException(env, EXCEPTION_NAME, "Failed to get setter method id.", STATUS_INVALID_OPERATION);
        return;
    }

    // call the setter method
    env->CallVoidMethod(kinesisVideoMetrics,
                        setterMethodId,
                        metrics.contentStoreSize,
                        metrics.contentStoreAllocatedSize,
                        metrics.contentStoreAvailableSize,
                        metrics.totalContentViewsSize,
                        metrics.totalFrameRate,
                        metrics.totalTransferRate);
}

void KinesisVideoClientWrapper::getKinesisVideoStreamMetrics(jlong streamHandle, jobject kinesisVideoStreamMetrics)
{
    STATUS retStatus = STATUS_SUCCESS;
    JNIEnv *env;
    mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);

    if (!IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        DLOGE("Invalid client object");
        throwNativeException(env, EXCEPTION_NAME, "Invalid call after the client is freed.", STATUS_INVALID_OPERATION);
        return;
    }

    if (!IS_VALID_STREAM_HANDLE(streamHandle))
    {
        DLOGE("Invalid stream handle 0x%016" PRIx64 , (UINT64) streamHandle);
        throwNativeException(env, EXCEPTION_NAME, "Invalid stream handle.", STATUS_INVALID_OPERATION);
        return;
    }

    if (NULL == kinesisVideoStreamMetrics)
    {
        DLOGE("KinesisVideoStreamMetrics object is null");
        throwNativeException(env, EXCEPTION_NAME, "KinesisVideoStreamMetrics object is null.", STATUS_NULL_ARG);
        return;
    }

    StreamMetrics metrics;
    metrics.version = STREAM_METRICS_CURRENT_VERSION;

    if (STATUS_FAILED(retStatus = ::getKinesisVideoStreamMetrics(streamHandle, &metrics)))
    {
        DLOGE("Failed to get StreamMetrics with status code 0x%08x", retStatus);
        throwNativeException(env, EXCEPTION_NAME, "Failed to get StreamMetrics.", retStatus);
        return;
    }

    //get the class
    jclass metricsClass = env->GetObjectClass(kinesisVideoStreamMetrics);
    if (metricsClass == NULL){
        DLOGE("Failed to get metrics class object");
        throwNativeException(env, EXCEPTION_NAME, "Failed to get metrics class object.", STATUS_INVALID_OPERATION);
        return;
    }

    // Set the Java object
    jmethodID setterMethodId = env->GetMethodID(metricsClass, "setMetrics", "(JJJJDJ)V");
    if (setterMethodId == NULL)
    {
        DLOGE("Failed to get the setter method id.");
        throwNativeException(env, EXCEPTION_NAME, "Failed to get setter method id.", STATUS_INVALID_OPERATION);
        return;
    }

    // call the setter method
    env->CallVoidMethod(kinesisVideoStreamMetrics,
                        setterMethodId,
                        metrics.overallViewSize,
                        metrics.currentViewSize,
                        metrics.overallViewDuration,
                        metrics.currentViewDuration,
                        metrics.currentFrameRate,
                        metrics.currentTransferRate);
}

STREAM_HANDLE KinesisVideoClientWrapper::createKinesisVideoStream(jobject streamInfo)
{
    STATUS retStatus = STATUS_SUCCESS;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    JNIEnv *env;
    StreamInfo kinesisVideoStreamInfo;
    mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);

    if (!IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        DLOGE("Invalid client object");
        throwNativeException(env, EXCEPTION_NAME, "Invalid call after the client is freed.", STATUS_INVALID_OPERATION);
        goto CleanUp;
    }

    // Convert the StreamInfo object
    MEMSET(&kinesisVideoStreamInfo, 0x00, SIZEOF(kinesisVideoStreamInfo));
    if (!setStreamInfo(env, streamInfo, &kinesisVideoStreamInfo))
    {
        DLOGE("Failed converting stream info object.");
        throwNativeException(env, EXCEPTION_NAME, "Failed converting stream info object.", STATUS_INVALID_OPERATION);
        goto CleanUp;
    }

    if (STATUS_FAILED(retStatus = ::createKinesisVideoStream(mClientHandle, &kinesisVideoStreamInfo, &streamHandle)))
    {
        DLOGE("Failed to create a stream with status code 0x%08x", retStatus);
        throwNativeException(env, EXCEPTION_NAME, "Failed to create a stream.", retStatus);
        goto CleanUp;
    }

CleanUp:

    // Release the temporary memory allocated for cpd
    if (kinesisVideoStreamInfo.streamCaps.codecPrivateData != NULL) {
        MEMFREE(kinesisVideoStreamInfo.streamCaps.codecPrivateData);
    }

    releaseTags(kinesisVideoStreamInfo.tags);

    return streamHandle;
}

void KinesisVideoClientWrapper::putKinesisVideoFrame(jlong streamHandle, jobject kinesisVideoFrame)
{
    STATUS retStatus = STATUS_SUCCESS;
    JNIEnv *env;
    mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);

    if (!IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        DLOGE("Invalid client object");
        throwNativeException(env, EXCEPTION_NAME, "Invalid call after the client is freed.", STATUS_INVALID_OPERATION);
        return;
    }

    if (!IS_VALID_STREAM_HANDLE(streamHandle))
    {
        DLOGE("Invalid stream handle 0x%016" PRIx64, (UINT64) streamHandle);
        throwNativeException(env, EXCEPTION_NAME, "Invalid stream handle.", STATUS_INVALID_OPERATION);
        return;
    }

    if (kinesisVideoFrame == NULL)
    {
        DLOGE("Invalid kinesis video frame.");
        throwNativeException(env, EXCEPTION_NAME, "Kinesis video frame is null.", STATUS_INVALID_OPERATION);
        return;
    }

    // Convert the KinesisVideoFrame object
    Frame frame;
    if (!setFrame(env, kinesisVideoFrame, &frame))
    {
        DLOGE("Failed converting frame object.");
        throwNativeException(env, EXCEPTION_NAME, "Failed converting frame object.", STATUS_INVALID_OPERATION);
        return;
    }

    if (STATUS_FAILED(retStatus = ::putKinesisVideoFrame(streamHandle, &frame)))
    {
        DLOGE("Failed to put a frame with status code 0x%08x", retStatus);
        throwNativeException(env, EXCEPTION_NAME, "Failed to put a frame into the stream.", retStatus);
        return;
    }
}

void KinesisVideoClientWrapper::getKinesisVideoStreamData(jlong streamHandle, jobject dataBuffer, jint offset, jint length, jobject readResult)
{
    STATUS retStatus = STATUS_SUCCESS;
    JNIEnv *env;
    mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    UINT64 uploadHandle = 0;
    UINT32 filledSize = 0, bufferSize = 0;
    PBYTE pBuffer = NULL;
    jboolean isEos = JNI_FALSE;
    jclass readResultClass;
    jmethodID setterMethodId;

    if (NULL == readResult)
    {
        DLOGE("NULL ReadResult object");
        throwNativeException(env, EXCEPTION_NAME, "NULL ReadResult object is passsed.", STATUS_NULL_ARG);
        goto CleanUp;
    }

    if (!IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        DLOGE("Invalid client object");
        throwNativeException(env, EXCEPTION_NAME, "Invalid call after the client is freed.", STATUS_INVALID_OPERATION);
        goto CleanUp;
    }

    if (!IS_VALID_STREAM_HANDLE(streamHandle))
    {
        DLOGE("Invalid stream handle 0x%016" PRIx64, (UINT64) streamHandle);
        throwNativeException(env, EXCEPTION_NAME, "Invalid stream handle.", STATUS_INVALID_OPERATION);
        goto CleanUp;
    }

    if (dataBuffer == NULL)
    {
        DLOGE("Invalid buffer object.");
        throwNativeException(env, EXCEPTION_NAME, "Invalid buffer object.", STATUS_INVALID_OPERATION);
        goto CleanUp;
    }

    // Convert the buffer of stream data to get
    if (!setStreamDataBuffer(env, dataBuffer, offset, &pBuffer))
    {
        DLOGE("Failed converting kinesis video stream data buffer object.");
        throwNativeException(env, EXCEPTION_NAME, "Failed converting kinesis video stream data buffer object.", STATUS_INVALID_OPERATION);
        goto CleanUp;
    }

    retStatus = ::getKinesisVideoStreamData(streamHandle, &uploadHandle, pBuffer, (UINT32) length, &filledSize);
    if (STATUS_SUCCESS != retStatus && STATUS_NO_MORE_DATA_AVAILABLE != retStatus && STATUS_END_OF_STREAM != retStatus)
    {
        DLOGE("Failed to get data from the stream with status code 0x%08x", retStatus);
        throwNativeException(env, EXCEPTION_NAME, "Failed to get data from the stream.", retStatus);
        goto CleanUp;
    }

    if (STATUS_END_OF_STREAM == retStatus) {
        isEos = JNI_TRUE;
    }

    // Get the class
    readResultClass = env->GetObjectClass(readResult);
    if (readResultClass == NULL){
        DLOGE("Failed to get ReadResult class object");
        throwNativeException(env, EXCEPTION_NAME, "Failed to get ReadResult class object.", STATUS_INVALID_OPERATION);
        goto CleanUp;
    }

    // Get the Java method id
    setterMethodId = env->GetMethodID(readResultClass, "setReadResult", "(JIZ)V");
    if (setterMethodId == NULL)
    {
        DLOGE("Failed to get the setter method id.");
        throwNativeException(env, EXCEPTION_NAME, "Failed to get setter method id.", STATUS_INVALID_OPERATION);
        goto CleanUp;
    }

    // Call the setter method
    env->CallVoidMethod(readResult,
                        setterMethodId,
                        uploadHandle,
                        filledSize,
                        isEos);

CleanUp:

    if (!releaseStreamDataBuffer(env, dataBuffer, offset, pBuffer))
    {
        DLOGE("Failed releasing kinesis video stream data buffer object.");
        throwNativeException(env, EXCEPTION_NAME, "Failed releasing kinesis video stream data buffer object.", STATUS_INVALID_OPERATION);
    }
}

void KinesisVideoClientWrapper::kinesisVideoStreamFragmentAck(jlong streamHandle, jlong uploadHandle, jobject fragmentAck)
{
    STATUS retStatus = STATUS_SUCCESS;
    JNIEnv *env;
    mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    FragmentAck ack;

    if (!IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        DLOGE("Invalid client object");
        throwNativeException(env, EXCEPTION_NAME, "Invalid call after the client is freed.", STATUS_INVALID_OPERATION);
        return;
    }

    if (!IS_VALID_STREAM_HANDLE(streamHandle))
    {
        DLOGE("Invalid stream handle 0x%016" PRIx64, (UINT64) streamHandle);
        throwNativeException(env, EXCEPTION_NAME, "Invalid stream handle.", STATUS_INVALID_OPERATION);
        return;
    }

    if (fragmentAck == NULL)
    {
        DLOGE("Invalid fragment ack");
        throwNativeException(env, EXCEPTION_NAME, "Invalid fragment ack.", STATUS_INVALID_OPERATION);
        return;
    }

    // Convert the KinesisVideoFrame object
    if (!setFragmentAck(env, fragmentAck, &ack))
    {
        DLOGE("Failed converting frame object.");
        throwNativeException(env, EXCEPTION_NAME, "Failed converting fragment ack object.", STATUS_INVALID_OPERATION);
        return;
    }

    if (STATUS_FAILED(retStatus = ::kinesisVideoStreamFragmentAck(streamHandle, uploadHandle, &ack)))
    {
        DLOGE("Failed to report a fragment ack with status code 0x%08x", retStatus);
        throwNativeException(env, EXCEPTION_NAME, "Failed to report a fragment ack.", retStatus);
        return;
    }
}

void KinesisVideoClientWrapper::kinesisVideoStreamParseFragmentAck(jlong streamHandle, jlong uploadHandle, jstring ack)
{
    STATUS retStatus = STATUS_SUCCESS;
    JNIEnv *env;
    mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);

    if (!IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        DLOGE("Invalid client object");
        throwNativeException(env, EXCEPTION_NAME, "Invalid call after the client is freed.", STATUS_INVALID_OPERATION);
        return;
    }

    if (!IS_VALID_STREAM_HANDLE(streamHandle))
    {
        DLOGE("Invalid stream handle 0x%016" PRIx64, (UINT64) streamHandle);
        throwNativeException(env, EXCEPTION_NAME, "Invalid stream handle.", STATUS_INVALID_OPERATION);
        return;
    }

    if (ack == NULL)
    {
        DLOGE("Invalid ack");
        throwNativeException(env, EXCEPTION_NAME, "Invalid ack.", STATUS_INVALID_OPERATION);
        return;
    }

    // Convert the jstring to PCHAR
    PCHAR pAckStr = (PCHAR) env->GetStringUTFChars(ack, NULL);

    // Call the API
    retStatus = ::kinesisVideoStreamParseFragmentAck(streamHandle, uploadHandle, pAckStr, 0);

    // Release the string
    env->ReleaseStringUTFChars(ack, pAckStr);

    if (STATUS_FAILED(retStatus))
    {
        DLOGE("Failed to parse a fragment ack with status code 0x%08x", retStatus);
        throwNativeException(env, EXCEPTION_NAME, "Failed to parse a fragment ack.", retStatus);
        return;
    }
}

void KinesisVideoClientWrapper::streamFormatChanged(jlong streamHandle, jobject codecPrivateData)
{
    STATUS retStatus = STATUS_SUCCESS;
    JNIEnv *env;
    mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    UINT32 bufferSize = 0;
    PBYTE pBuffer = NULL;
    BOOL releaseBuffer = FALSE;

    if (!IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        DLOGE("Invalid client object");
        throwNativeException(env, EXCEPTION_NAME, "Invalid call after the client is freed.", STATUS_INVALID_OPERATION);
        goto CleanUp;
    }

    if (!IS_VALID_STREAM_HANDLE(streamHandle))
    {
        DLOGE("Invalid stream handle 0x%016" PRIx64, (UINT64) streamHandle);
        throwNativeException(env, EXCEPTION_NAME, "Invalid stream handle.", STATUS_INVALID_OPERATION);
        goto CleanUp;
    }

    // Get the codec private data byte buffer - null object has a special semmantic of clearing the CPD
    if (codecPrivateData != NULL) {
        bufferSize = (UINT32) env->GetArrayLength((jbyteArray) codecPrivateData);
        if (NULL == (pBuffer = (PBYTE) env->GetByteArrayElements((jbyteArray) codecPrivateData, NULL)))
        {
            DLOGE("Failed getting byte buffer from the java array.");
            throwNativeException(env, EXCEPTION_NAME, "Failed getting byte buffer from the java array.", STATUS_INVALID_OPERATION);
            goto CleanUp;
        }

        // Make sure the buffer is released at the end
        releaseBuffer = TRUE;
    } else {
        pBuffer = NULL;
        bufferSize = 0;
    }

    if (STATUS_FAILED(retStatus = ::kinesisVideoStreamFormatChanged(streamHandle, bufferSize, pBuffer)))
    {
        DLOGE("Failed to set the stream format with status code 0x%08x", retStatus);
        throwNativeException(env, EXCEPTION_NAME, "Failed to set the stream format.", retStatus);
        goto CleanUp;
    }

CleanUp:

    if (releaseBuffer)
    {
        env->ReleaseByteArrayElements((jbyteArray) codecPrivateData, (jbyte*) pBuffer, JNI_ABORT);
    }
}

void KinesisVideoClientWrapper::describeStreamResult(jlong streamHandle, jint httpStatusCode, jobject streamDescription)
{
    STATUS retStatus = STATUS_SUCCESS;
    JNIEnv *env;
    mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);

    if (!IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        DLOGE("Invalid client object");
        throwNativeException(env, EXCEPTION_NAME, "Invalid call after the client is freed.", STATUS_INVALID_OPERATION);
        return;
    }

    StreamDescription streamDesc;
    PStreamDescription pStreamDesc = NULL;
    if (NULL != streamDescription) {
        if (!setStreamDescription(env, streamDescription, &streamDesc)) {
            DLOGE("Failed converting stream description object.");
            throwNativeException(env, EXCEPTION_NAME, "Failed converting stream description object.",
                                 STATUS_INVALID_OPERATION);
            return;
        }

        // Assign the converted object address.
        pStreamDesc = &streamDesc;
    }

    if (STATUS_FAILED(retStatus = ::describeStreamResultEvent(streamHandle, (SERVICE_CALL_RESULT) httpStatusCode, pStreamDesc)))
    {
        DLOGE("Failed to describe stream result event with status code 0x%08x", retStatus);
        throwNativeException(env, EXCEPTION_NAME, "Failed to describe stream result event.", retStatus);
        return;
    }
}

void KinesisVideoClientWrapper::kinesisVideoStreamTerminated(jlong streamHandle, jlong uploadHandle, jint httpStatusCode)
{
    STATUS retStatus = STATUS_SUCCESS;
    JNIEnv *env;
    mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);

    if (!IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        DLOGE("Invalid client object");
        throwNativeException(env, EXCEPTION_NAME, "Invalid call after the client is freed.", STATUS_INVALID_OPERATION);
        return;
    }

    if (STATUS_FAILED(retStatus = ::kinesisVideoStreamTerminated(streamHandle, uploadHandle, (SERVICE_CALL_RESULT) httpStatusCode)))
    {
        DLOGE("Failed to submit stream terminated event with status code 0x%08x", retStatus);
        throwNativeException(env, EXCEPTION_NAME, "Failed to submit stream terminated event.", retStatus);
        return;
    }
}

void KinesisVideoClientWrapper::createStreamResult(jlong streamHandle, jint httpStatusCode, jstring streamArn)
{
    STATUS retStatus = STATUS_SUCCESS;
    JNIEnv *env;
    PCHAR pStreamArn = NULL;
    mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);

    if (!IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        DLOGE("Invalid client object");
        throwNativeException(env, EXCEPTION_NAME, "Invalid call after the client is freed.", STATUS_INVALID_OPERATION);
        return;
    }

    if (streamArn != NULL) {
        pStreamArn = (PCHAR) env->GetStringUTFChars(streamArn, NULL);
    }

    retStatus = ::createStreamResultEvent(streamHandle, (SERVICE_CALL_RESULT) httpStatusCode, pStreamArn);

    // Ensure we release the string
    if (pStreamArn != NULL) {
        env->ReleaseStringUTFChars(streamArn, pStreamArn);
    }

    if (STATUS_FAILED(retStatus))
    {
        DLOGE("Failed to create stream result event with status code 0x%08x", retStatus);
        throwNativeException(env, EXCEPTION_NAME, "Failed to create stream result event.", retStatus);
        return;
    }
}

void KinesisVideoClientWrapper::putStreamResult(jlong streamHandle, jint httpStatusCode, jlong clientStreamHandle)
{
    STATUS retStatus = STATUS_SUCCESS;
    JNIEnv *env;
    mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);

    if (!IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        DLOGE("Invalid client object");
        throwNativeException(env, EXCEPTION_NAME, "Invalid call after the client is freed.", STATUS_INVALID_OPERATION);
        return;
    }

    if (STATUS_FAILED(retStatus = ::putStreamResultEvent(streamHandle, (SERVICE_CALL_RESULT) httpStatusCode, (UINT64) clientStreamHandle)))
    {
        DLOGE("Failed to put stream result event with status code 0x%08x", retStatus);
        throwNativeException(env, EXCEPTION_NAME, "Failed to put stream result event.", retStatus);
        return;
    }
}

void KinesisVideoClientWrapper::tagResourceResult(jlong customData, jint httpStatusCode)
{
    STATUS retStatus = STATUS_SUCCESS;
    JNIEnv *env;
    mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);

    if (!IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        DLOGE("Invalid client object");
        throwNativeException(env, EXCEPTION_NAME, "Invalid call after the client is freed.", STATUS_INVALID_OPERATION);
        return;
    }

    if (STATUS_FAILED(retStatus = ::tagResourceResultEvent(customData, (SERVICE_CALL_RESULT) httpStatusCode)))
    {
        DLOGE("Failed on tag resource result event with status code 0x%08x", retStatus);
        throwNativeException(env, EXCEPTION_NAME, "Failed on tag resource result event.", retStatus);
        return;
    }
}

void KinesisVideoClientWrapper::getStreamingEndpointResult(jlong streamHandle, jint httpStatusCode, jstring streamingEndpoint)
{
    STATUS retStatus = STATUS_SUCCESS;
    JNIEnv *env;
    mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);

    if (!IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        DLOGE("Invalid client object");
        throwNativeException(env, EXCEPTION_NAME, "Invalid call after the client is freed.", STATUS_INVALID_OPERATION);
        return;
    }

    CHAR pEndpoint[MAX_URI_CHAR_LEN];
    if (!setStreamingEndpoint(env, streamingEndpoint, pEndpoint))
    {
        DLOGE("Failed converting streaming endpoint object.");
        throwNativeException(env, EXCEPTION_NAME, "Failed converting streaming endpoint object.", STATUS_INVALID_OPERATION);
        return;
    }

    if (STATUS_FAILED(retStatus = ::getStreamingEndpointResultEvent(streamHandle, (SERVICE_CALL_RESULT) httpStatusCode, pEndpoint)))
    {
        DLOGE("Failed to get streaming endpoint result event with status code 0x%08x", retStatus);
        throwNativeException(env, EXCEPTION_NAME, "Failed to get streaming endpoint result event.", retStatus);
        return;
    }
}

void KinesisVideoClientWrapper::getStreamingTokenResult(jlong streamHandle, jint httpStatusCode, jbyteArray streamingToken, jint tokenSize, jlong expiration)
{
    STATUS retStatus = STATUS_SUCCESS;
    JNIEnv *env;
    mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);

    if (!IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        DLOGE("Invalid client object");
        throwNativeException(env, EXCEPTION_NAME, "Invalid call after the client is freed.", STATUS_INVALID_OPERATION);
        return;
    }

    if (tokenSize > MAX_AUTH_LEN) {
        DLOGE("Invalid token size");
        throwNativeException(env, EXCEPTION_NAME, "Invalid token size", STATUS_INVALID_OPERATION);
        return;
    }

    BYTE pToken[MAX_AUTH_LEN];
    env->GetByteArrayRegion(streamingToken, 0, tokenSize, (jbyte *) pToken);

    if (STATUS_FAILED(retStatus = ::getStreamingTokenResultEvent(streamHandle,
                                                                 (SERVICE_CALL_RESULT) httpStatusCode,
                                                                 pToken,
                                                                 (UINT32) tokenSize,
                                                                 (UINT64) expiration)))
    {
        DLOGE("Failed to get streaming token result event with status code 0x%08x", retStatus);
        throwNativeException(env, EXCEPTION_NAME, "Failed to get streaming token result event.", retStatus);
        return;
    }
}

void KinesisVideoClientWrapper::createDeviceResult(jlong clientHandle, jint httpStatusCode, jstring deviceArn)
{
    STATUS retStatus = STATUS_SUCCESS;
    JNIEnv *env;
    PCHAR pDeviceArn = NULL;
    mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);

    if (!IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        DLOGE("Invalid client object");
        throwNativeException(env, EXCEPTION_NAME, "Invalid call after the client is freed.", STATUS_INVALID_OPERATION);
        return;
    }

    if (deviceArn != NULL) {
        pDeviceArn = (PCHAR) env->GetStringUTFChars(deviceArn, NULL);
    }

    retStatus = ::createDeviceResultEvent(clientHandle, (SERVICE_CALL_RESULT) httpStatusCode, pDeviceArn);

    // Ensure we release the string
    if (pDeviceArn != NULL) {
        env->ReleaseStringUTFChars(deviceArn, pDeviceArn);
    }

    if (STATUS_FAILED(retStatus))
    {
        DLOGE("Failed to create device result event with status code 0x%08x", retStatus);
        throwNativeException(env, EXCEPTION_NAME, "Failed to create device result event.", retStatus);
        return;
    }
}

void KinesisVideoClientWrapper::deviceCertToTokenResult(jlong clientHandle, jint httpStatusCode, jbyteArray token, jint tokenSize, jlong expiration)
{
    STATUS retStatus = STATUS_SUCCESS;
    JNIEnv *env;
    mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);

    if (!IS_VALID_CLIENT_HANDLE(mClientHandle))
    {
        DLOGE("Invalid client object");
        throwNativeException(env, EXCEPTION_NAME, "Invalid call after the client is freed.", STATUS_INVALID_OPERATION);
        return;
    }

    if (tokenSize > MAX_AUTH_LEN) {
        DLOGE("Invalid token size");
        throwNativeException(env, EXCEPTION_NAME, "Invalid token size", STATUS_INVALID_OPERATION);
        return;
    }

    BYTE pToken[MAX_AUTH_LEN];
    env->GetByteArrayRegion(token, 0, tokenSize, (jbyte *) pToken);

    if (STATUS_FAILED(retStatus = ::deviceCertToTokenResultEvent(clientHandle,
                                                                 (SERVICE_CALL_RESULT) httpStatusCode,
                                                                 pToken,
                                                                 (UINT32) tokenSize,
                                                                 (UINT64) expiration)))
    {
        DLOGE("Failed the deviceCertToToken result event with status code 0x%08x", retStatus);
        throwNativeException(env, EXCEPTION_NAME, "Failed the deviceCertToToken result event.", retStatus);
        return;
    }
}

BOOL KinesisVideoClientWrapper::setCallbacks(JNIEnv* env, jobject thiz)
{
    CHECK(env != NULL && thiz != NULL);

    // The customData will point to this object
    mClientCallbacks.customData = POINTER_TO_HANDLE(this);

    // Need the current version of the structure
    mClientCallbacks.version = CALLBACKS_CURRENT_VERSION;

    // Set the function pointers
    mClientCallbacks.getCurrentTimeFn = getCurrentTimeFunc;
    mClientCallbacks.getRandomNumberFn = getRandomNumberFunc;
    mClientCallbacks.createMutexFn = createMutexFunc;
    mClientCallbacks.lockMutexFn = lockMutexFunc;
    mClientCallbacks.unlockMutexFn = unlockMutexFunc;
    mClientCallbacks.tryLockMutexFn = tryLockMutexFunc;
    mClientCallbacks.freeMutexFn = freeMutexFunc;
    mClientCallbacks.getDeviceCertificateFn = getDeviceCertificateFunc;
    mClientCallbacks.getSecurityTokenFn = getSecurityTokenFunc;
    mClientCallbacks.getDeviceFingerprintFn = getDeviceFingerprintFunc;
    mClientCallbacks.streamUnderflowReportFn = streamUnderflowReportFunc;
    mClientCallbacks.storageOverflowPressureFn = storageOverflowPressureFunc;
    mClientCallbacks.streamLatencyPressureFn = streamLatencyPressureFunc;
    mClientCallbacks.streamConnectionStaleFn = streamConnectionStaleFunc;
    mClientCallbacks.fragmentAckReceivedFn = fragmentAckReceivedFunc;
    mClientCallbacks.droppedFrameReportFn = droppedFrameReportFunc;
    mClientCallbacks.droppedFragmentReportFn = droppedFragmentReportFunc;
    mClientCallbacks.streamErrorReportFn = streamErrorReportFunc;
    mClientCallbacks.streamDataAvailableFn = streamDataAvailableFunc;
    mClientCallbacks.streamReadyFn = streamReadyFunc;
    mClientCallbacks.streamClosedFn = streamClosedFunc;
    mClientCallbacks.createStreamFn = createStreamFunc;
    mClientCallbacks.describeStreamFn = describeStreamFunc;
    mClientCallbacks.getStreamingEndpointFn = getStreamingEndpointFunc;
    mClientCallbacks.getStreamingTokenFn = getStreamingTokenFunc;
    mClientCallbacks.putStreamFn = putStreamFunc;
    mClientCallbacks.tagResourceFn = tagResourceFunc;
    mClientCallbacks.clientReadyFn = clientReadyFunc;
    mClientCallbacks.createDeviceFn = createDeviceFunc;
    mClientCallbacks.deviceCertToTokenFn = deviceCertToTokenFunc;

    // Extract the method IDs for the callbacks and set a global reference
    jclass thizCls = env->GetObjectClass(thiz);
    if (thizCls == NULL) {
        DLOGE("Failed to get the object class for the JNI object.");
        return FALSE;
    }

    // Setup the environment and the callbacks
    if (NULL == (mGlobalJniObjRef = env->NewGlobalRef(thiz))) {
        DLOGE("Failed to create a global reference for the JNI object.");
        return FALSE;
    }

    // Extract the method IDs
    mGetDeviceCertificateMethodId = env->GetMethodID(thizCls, "getDeviceCertificate", "()Lcom/amazonaws/kinesisvideo/producer/AuthInfo;");
    if (mGetDeviceCertificateMethodId == NULL) {
        DLOGE("Couldn't find method id getDeviceCertificate");
        return FALSE;
    }

    mGetSecurityTokenMethodId = env->GetMethodID(thizCls, "getSecurityToken", "()Lcom/amazonaws/kinesisvideo/producer/AuthInfo;");
    if (mGetSecurityTokenMethodId == NULL) {
        DLOGE("Couldn't find method id getSecurityToken");
        return FALSE;
    }

    mGetDeviceFingerprintMethodId = env->GetMethodID(thizCls, "getDeviceFingerprint", "()Ljava/lang/String;");
    if (mGetDeviceFingerprintMethodId == NULL) {
        DLOGE("Couldn't find method id getDeviceFingerprint");
        return FALSE;
    }

    mStreamUnderflowReportMethodId = env->GetMethodID(thizCls, "streamUnderflowReport", "(J)V");
    if (mStreamUnderflowReportMethodId == NULL) {
        DLOGE("Couldn't find method id streamUnderflowReport");
        return FALSE;
    }

    mStorageOverflowPressureMethodId = env->GetMethodID(thizCls, "storageOverflowPressure", "(J)V");
    if (mStorageOverflowPressureMethodId == NULL) {
        DLOGE("Couldn't find method id storageOverflowPressure");
        return FALSE;
    }

    mStreamLatencyPressureMethodId = env->GetMethodID(thizCls, "streamLatencyPressure", "(JJ)V");
    if (mStreamLatencyPressureMethodId == NULL) {
        DLOGE("Couldn't find method id streamLatencyPressure");
        return FALSE;
    }

    mStreamConnectionStaleMethodId = env->GetMethodID(thizCls, "streamConnectionStale", "(JJ)V");
    if (mStreamConnectionStaleMethodId == NULL) {
        DLOGE("Couldn't find method id streamConnectionStale");
        return FALSE;
    }

    mFragmentAckReceivedMethodId = env->GetMethodID(thizCls, "fragmentAckReceived", "(JLcom/amazonaws/kinesisvideo/producer/KinesisVideoFragmentAck;)V");
    if (mFragmentAckReceivedMethodId == NULL) {
        DLOGE("Couldn't find method id fragmentAckReceived");
        return FALSE;
    }

    mDroppedFrameReportMethodId = env->GetMethodID(thizCls, "droppedFrameReport", "(JJ)V");
    if (mDroppedFrameReportMethodId == NULL) {
        DLOGE("Couldn't find method id droppedFrameReport");
        return FALSE;
    }

    mDroppedFragmentReportMethodId = env->GetMethodID(thizCls, "droppedFragmentReport", "(JJ)V");
    if (mDroppedFragmentReportMethodId == NULL) {
        DLOGE("Couldn't find method id droppedFragmentReport");
        return FALSE;
    }

    mStreamErrorReportMethodId = env->GetMethodID(thizCls, "streamErrorReport", "(JJJ)V");
    if (mStreamErrorReportMethodId == NULL) {
        DLOGE("Couldn't find method id streamErrorReport");
        return FALSE;
    }

    mStreamDataAvailableMethodId = env->GetMethodID(thizCls, "streamDataAvailable", "(JLjava/lang/String;JJJ)V");
    if (mStreamDataAvailableMethodId == NULL) {
        DLOGE("Couldn't find method id streamDataAvailable");
        return FALSE;
    }

    mStreamReadyMethodId = env->GetMethodID(thizCls, "streamReady", "(J)V");
    if (mStreamReadyMethodId == NULL) {
        DLOGE("Couldn't find method id streamReady");
        return FALSE;
    }

    mStreamClosedMethodId = env->GetMethodID(thizCls, "streamClosed", "(JJ)V");
    if (mStreamClosedMethodId == NULL) {
        DLOGE("Couldn't find method id streamClosed");
        return FALSE;
    }

    mCreateStreamMethodId = env->GetMethodID(thizCls, "createStream", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JJJ[BIJ)I");
    if (mCreateStreamMethodId == NULL) {
        DLOGE("Couldn't find method id createStream");
        return FALSE;
    }

    mDescribeStreamMethodId = env->GetMethodID(thizCls, "describeStream", "(Ljava/lang/String;JJ[BIJ)I");
    if (mDescribeStreamMethodId == NULL) {
        DLOGE("Couldn't find method id describeStream");
        return FALSE;
    }

    mGetStreamingEndpointMethodId = env->GetMethodID(thizCls, "getStreamingEndpoint", "(Ljava/lang/String;Ljava/lang/String;JJ[BIJ)I");
    if (mGetStreamingEndpointMethodId == NULL) {
        DLOGE("Couldn't find method id getStreamingEndpoint");
        return FALSE;
    }

    mGetStreamingTokenMethodId = env->GetMethodID(thizCls, "getStreamingToken", "(Ljava/lang/String;JJ[BIJ)I");
    if (mGetStreamingTokenMethodId == NULL) {
        DLOGE("Couldn't find method id getStreamingToken");
        return FALSE;
    }

    mPutStreamMethodId = env->GetMethodID(thizCls, "putStream", "(Ljava/lang/String;Ljava/lang/String;JZZLjava/lang/String;JJ[BIJ)I");
    if (mPutStreamMethodId == NULL) {
        DLOGE("Couldn't find method id putStream");
        return FALSE;
    }

    mTagResourceMethodId = env->GetMethodID(thizCls, "tagResource", "(Ljava/lang/String;[Lcom/amazonaws/kinesisvideo/producer/Tag;JJ[BIJ)I");
    if (mTagResourceMethodId == NULL) {
        DLOGE("Couldn't find method id tagResource");
        return FALSE;
    }

    mClientReadyMethodId = env->GetMethodID(thizCls, "clientReady", "(J)V");
    if (mClientReadyMethodId == NULL) {
        DLOGE("Couldn't find method id clientReady");
        return FALSE;
    }

    mCreateDeviceMethodId = env->GetMethodID(thizCls, "createDevice", "(Ljava/lang/String;JJ[BIJ)I");
    if (mCreateDeviceMethodId == NULL) {
        DLOGE("Couldn't find method id createDevice");
        return FALSE;
    }

    mDeviceCertToTokenMethodId = env->GetMethodID(thizCls, "deviceCertToToken", "(Ljava/lang/String;JJ[BIJ)I");
    if (mDeviceCertToTokenMethodId == NULL) {
        DLOGE("Couldn't find method id deviceCertToToken");
        return FALSE;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////
// Static callbacks definitions implemented here
//////////////////////////////////////////////////////////////////////////////////////
UINT64 KinesisVideoClientWrapper::getCurrentTimeFunc(UINT64 customData)
{
    DLOGS("TID 0x%016" PRIx64 " getCurrentTimeFunc called.", GETTID());
    UNUSED_PARAM(customData);

    // We use System.currentTimeMillis() in Java land so need to enforce the UNIX time here.
    struct timeval nowTime;
    if (0 != gettimeofday(&nowTime, NULL)) {
        return 0;
    }

    return (UINT64) nowTime.tv_sec * HUNDREDS_OF_NANOS_IN_A_SECOND + (UINT64) nowTime.tv_usec * HUNDREDS_OF_NANOS_IN_A_MICROSECOND;
}

UINT32 KinesisVideoClientWrapper::getRandomNumberFunc(UINT64 customData)
{
    DLOGS("TID 0x%016" PRIx64 " getRandomNumberFunc called.", GETTID());
    UNUSED_PARAM(customData);
    return RAND();
}

MUTEX KinesisVideoClientWrapper::createMutexFunc(UINT64 customData, BOOL reentant)
{
    DLOGS("TID 0x%016" PRIx64 " createMutexFunc called.", GETTID());
    UNUSED_PARAM(customData);
    return MUTEX_CREATE(reentant);
}

VOID KinesisVideoClientWrapper::lockMutexFunc(UINT64 customData, MUTEX mutex)
{
    DLOGS("TID 0x%016" PRIx64 " lockMutexFunc called.", GETTID());
    UNUSED_PARAM(customData);
    return MUTEX_LOCK(mutex);
}

VOID KinesisVideoClientWrapper::unlockMutexFunc(UINT64 customData, MUTEX mutex)
{
    DLOGS("TID 0x%016" PRIx64 " unlockMutexFunc called.", GETTID());
    UNUSED_PARAM(customData);
    return MUTEX_UNLOCK(mutex);
}

VOID KinesisVideoClientWrapper::tryLockMutexFunc(UINT64 customData, MUTEX mutex)
{
    DLOGS("TID 0x%016" PRIx64 " tryLockMutexFunc called.", GETTID());
    UNUSED_PARAM(customData);
    return MUTEX_TRYLOCK(mutex);
}

VOID KinesisVideoClientWrapper::freeMutexFunc(UINT64 customData, MUTEX mutex)
{
    DLOGS("TID 0x%016" PRIx64 " freeMutexFunc called.", GETTID());
    UNUSED_PARAM(customData);
    return MUTEX_FREE(mutex);
}

//////////////////////////////////////////////////////////////////////////////////////
// Static callbacks definitions implemented in the Java layer
//////////////////////////////////////////////////////////////////////////////////////
STATUS KinesisVideoClientWrapper::getDeviceCertificateFunc(UINT64 customData, PBYTE* ppCert, PUINT32 pSize, PUINT64 pExpiration)
{
    DLOGS("TID 0x%016" PRIx64 " getDeviceCertificateFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL && ppCert != NULL && pSize != NULL && pExpiration != NULL);

    return pWrapper->getAuthInfo(pWrapper->mGetDeviceCertificateMethodId, ppCert, pSize, pExpiration);
}

STATUS KinesisVideoClientWrapper::getSecurityTokenFunc(UINT64 customData, PBYTE* ppToken, PUINT32 pSize, PUINT64 pExpiration)
{
    DLOGS("TID 0x%016" PRIx64 " getSecurityTokenFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL && ppToken != NULL && pSize != NULL && pExpiration != NULL);

    return pWrapper->getAuthInfo(pWrapper->mGetSecurityTokenMethodId, ppToken, pSize, pExpiration);
}

STATUS KinesisVideoClientWrapper::getDeviceFingerprintFunc(UINT64 customData, PCHAR* ppFingerprint)
{
    DLOGS("TID 0x%016" PRIx64 " getDeviceFingerprintFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL && ppFingerprint != NULL);

    // Get the ENV from the JavaVM
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;
    jstring jstr = NULL;
    const jchar* bufferPtr = NULL;
    UINT32 strLen;

    INT32 envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func
    jstr = (jstring) env->CallObjectMethod(pWrapper->mGlobalJniObjRef, pWrapper->mGetDeviceFingerprintMethodId);
    CHK_JVM_EXCEPTION(env);

    if (jstr != NULL) {
        // Extract the bits from the byte buffer
        bufferPtr = env->GetStringChars(jstr, NULL);
        strLen = STRLEN((PCHAR) bufferPtr);
        if (strLen >= MAX_AUTH_LEN) {
            retStatus = STATUS_INVALID_ARG;
            goto CleanUp;
        }

        // Copy the bits
        STRCPY((PCHAR) pWrapper->mAuthInfo.data, (PCHAR) bufferPtr);
        pWrapper->mAuthInfo.type = AUTH_INFO_NONE;
        pWrapper->mAuthInfo.size = strLen;

        *ppFingerprint = (PCHAR) pWrapper->mAuthInfo.data;
    } else {
        pWrapper->mAuthInfo.type = AUTH_INFO_NONE;
        pWrapper->mAuthInfo.size = 0;

        // Set the returned values
        *ppFingerprint = NULL;
    }

CleanUp:

    // Release the array object if allocated
    if (bufferPtr != NULL) {
        env->ReleaseStringChars(jstr, bufferPtr);
    }

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::streamUnderflowReportFunc(UINT64 customData, STREAM_HANDLE streamHandle)
{
    DLOGS("TID 0x%016" PRIx64 " streamUnderflowReportFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Get the ENV from the JavaVM
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;

    INT32 envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func
    env->CallVoidMethod(pWrapper->mGlobalJniObjRef, pWrapper->mStreamUnderflowReportMethodId, streamHandle);
    CHK_JVM_EXCEPTION(env);

CleanUp:

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::storageOverflowPressureFunc(UINT64 customData, UINT64 remainingSize)
{
    DLOGS("TID 0x%016" PRIx64 " storageOverflowPressureFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Get the ENV from the JavaVM
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;

    INT32 envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func
    env->CallVoidMethod(pWrapper->mGlobalJniObjRef, pWrapper->mStorageOverflowPressureMethodId, remainingSize);
    CHK_JVM_EXCEPTION(env);

CleanUp:

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::streamLatencyPressureFunc(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 duration)
{
    DLOGS("TID 0x%016" PRIx64 " streamLatencyPressureFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Get the ENV from the JavaVM
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;

    INT32 envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func
    env->CallVoidMethod(pWrapper->mGlobalJniObjRef, pWrapper->mStreamLatencyPressureMethodId, streamHandle, duration);
    CHK_JVM_EXCEPTION(env);

CleanUp:

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::streamConnectionStaleFunc(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 duration)
{
    DLOGS("TID 0x%016" PRIx64 " streamConnectionStaleFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Get the ENV from the JavaVM
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;

    INT32 envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func
    env->CallVoidMethod(pWrapper->mGlobalJniObjRef, pWrapper->mStreamConnectionStaleMethodId, streamHandle, duration);
    CHK_JVM_EXCEPTION(env);

CleanUp:

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::fragmentAckReceivedFunc(UINT64 customData, STREAM_HANDLE streamHandle, PFragmentAck pFragmentAck)
{
    DLOGS("TID 0x%016" PRIx64 " fragmentAckReceivedFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Get the ENV from the JavaVM
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;
    jstring jstrSequenceNum = NULL;
    jobject ack = NULL;
    jmethodID methodId = NULL;
    jclass ackClass = NULL;

    INT32 envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Get the class object for Ack
    ackClass = env->FindClass("com/amazonaws/kinesisvideo/producer/KinesisVideoFragmentAck");
    CHK(ackClass != NULL, STATUS_INVALID_OPERATION);

    // Get the Ack constructor method id
    methodId = env->GetMethodID(ackClass, "<init>", "(IJLjava/lang/String;I)V");
    CHK(methodId != NULL, STATUS_INVALID_OPERATION);

    jstrSequenceNum = env->NewStringUTF(pFragmentAck->sequenceNumber);
    CHK(jstrSequenceNum != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Create a new tag object
    ack = env->NewObject(ackClass,
                         methodId,
                         (jint) pFragmentAck->ackType,
                         (jlong) pFragmentAck->timestamp,
                         jstrSequenceNum,
                         (jint) pFragmentAck->result);
    CHK(ack != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Call the Java func
    env->CallVoidMethod(pWrapper->mGlobalJniObjRef, pWrapper->mFragmentAckReceivedMethodId, streamHandle, ack);
    CHK_JVM_EXCEPTION(env);

CleanUp:

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::droppedFrameReportFunc(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 frameTimecode)
{
    DLOGS("TID 0x%016" PRIx64 " droppedFrameReportFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Get the ENV from the JavaVM
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;

    INT32 envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func
    env->CallVoidMethod(pWrapper->mGlobalJniObjRef, pWrapper->mDroppedFrameReportMethodId, streamHandle, frameTimecode);
    CHK_JVM_EXCEPTION(env);

CleanUp:

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::droppedFragmentReportFunc(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 fragmentTimecode)
{
    DLOGS("TID 0x%016" PRIx64 " droppedFragmentReportFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Get the ENV from the JavaVM
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;

    INT32 envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func
    env->CallVoidMethod(pWrapper->mGlobalJniObjRef, pWrapper->mDroppedFragmentReportMethodId, streamHandle, fragmentTimecode);
    CHK_JVM_EXCEPTION(env);

CleanUp:

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::streamErrorReportFunc(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 fragmentTimecode, STATUS statusCode)
{
    DLOGS("TID 0x%016" PRIx64 " streamErrorReportFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Get the ENV from the JavaVM
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;

    INT32 envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func
    env->CallVoidMethod(pWrapper->mGlobalJniObjRef, pWrapper->mStreamErrorReportMethodId, streamHandle, fragmentTimecode, statusCode);
    CHK_JVM_EXCEPTION(env);

    CleanUp:

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::streamReadyFunc(UINT64 customData, STREAM_HANDLE streamHandle)
{
    DLOGS("TID 0x%016" PRIx64 " streamReadyFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Get the ENV from the JavaVM
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;

    INT32 envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func
    env->CallVoidMethod(pWrapper->mGlobalJniObjRef, pWrapper->mStreamReadyMethodId, streamHandle);
    CHK_JVM_EXCEPTION(env);

CleanUp:

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::streamClosedFunc(UINT64 customData, STREAM_HANDLE streamHandle, UINT64 uploadHandle)
{
    DLOGS("TID 0x%016" PRIx64 " streamClosedFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Get the ENV from the JavaVM
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;

    INT32 envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func
    env->CallVoidMethod(pWrapper->mGlobalJniObjRef, pWrapper->mStreamClosedMethodId, streamHandle, uploadHandle);
    CHK_JVM_EXCEPTION(env);

    CleanUp:

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::streamDataAvailableFunc(UINT64 customData, STREAM_HANDLE streamHandle, PCHAR streamName, UINT64 uploadHandle, UINT64 duration, UINT64 availableSize)
{
    DLOGS("TID 0x%016" PRIx64 " streamDataAvailableFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Get the ENV from the JavaVM
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;
    jstring jstrStreamName = NULL;

    INT32 envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func
    jstrStreamName = env->NewStringUTF(streamName);
    if (jstrStreamName == NULL) {
        retStatus = STATUS_NOT_ENOUGH_MEMORY;
        goto CleanUp;
    }

    env->CallVoidMethod(pWrapper->mGlobalJniObjRef, pWrapper->mStreamDataAvailableMethodId, streamHandle, jstrStreamName, uploadHandle, duration, availableSize);
    CHK_JVM_EXCEPTION(env);

CleanUp:

    if (jstrStreamName != NULL) {
        env->DeleteLocalRef(jstrStreamName);
    }

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::createStreamFunc(UINT64 customData,
                                             PCHAR deviceName,
                                             PCHAR streamName,
                                             PCHAR contentType,
                                             PCHAR kmsKeyId,
                                             UINT64 retention,
                                             PServiceCallContext pCallbackContext)
{
    DLOGS("TID 0x%016" PRIx64 " createStreamFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Get the ENV from the JavaVM
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;
    jstring jstrDeviceName = NULL, jstrStreamName = NULL, jstrContentType = NULL, jstrKmsKeyId = NULL;
    jbyteArray authByteArray = NULL;

    INT32 envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func
    jstrDeviceName = env->NewStringUTF(deviceName);
    jstrStreamName = env->NewStringUTF(streamName);
    jstrContentType = env->NewStringUTF(contentType);
    if (kmsKeyId != NULL) {
        jstrKmsKeyId = env->NewStringUTF(kmsKeyId);
    }

    authByteArray = env->NewByteArray(pCallbackContext->pAuthInfo->size);
    if (jstrContentType == NULL ||
            jstrDeviceName == NULL ||
            jstrStreamName == NULL ||
            authByteArray == NULL) {
        retStatus = STATUS_NOT_ENOUGH_MEMORY;
        goto CleanUp;
    }

    // Copy the bits into the managed array
    env->SetByteArrayRegion(authByteArray,
                            0,
                            pCallbackContext->pAuthInfo->size,
                            (const jbyte*) pCallbackContext->pAuthInfo->data);

    // Invoke the callback
    retStatus = env->CallIntMethod(pWrapper->mGlobalJniObjRef,
                                   pWrapper->mCreateStreamMethodId,
                                   jstrDeviceName,
                                   jstrStreamName,
                                   jstrContentType,
                                   jstrKmsKeyId,
                                   retention,
                                   pCallbackContext->callAfter,
                                   pCallbackContext->timeout,
                                   authByteArray,
                                   pCallbackContext->pAuthInfo->type,
                                   pCallbackContext->customData
    );

    CHK_JVM_EXCEPTION(env);

CleanUp:

    if (jstrDeviceName != NULL) {
        env->DeleteLocalRef(jstrDeviceName);
    }

    if (jstrStreamName != NULL) {
        env->DeleteLocalRef(jstrStreamName);
    }

    if (jstrContentType != NULL) {
        env->DeleteLocalRef(jstrContentType);
    }

    if (jstrKmsKeyId != NULL) {
        env->DeleteLocalRef(jstrKmsKeyId);
    }

    if (authByteArray != NULL) {
        env->DeleteLocalRef(authByteArray);
    }

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::describeStreamFunc(UINT64 customData,
                                               PCHAR streamName,
                                               PServiceCallContext pCallbackContext)
{
    DLOGS("TID 0x%016" PRIx64 " describeStreamFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Get the ENV from the JavaVM
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;
    jstring jstrStreamName = NULL;
    jbyteArray authByteArray = NULL;

    INT32 envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func
    jstrStreamName = env->NewStringUTF(streamName);
    authByteArray = env->NewByteArray(pCallbackContext->pAuthInfo->size);
    if (jstrStreamName == NULL ||
            authByteArray == NULL) {
        retStatus = STATUS_NOT_ENOUGH_MEMORY;
        goto CleanUp;
    }

    // Copy the bits into the managed array
    env->SetByteArrayRegion(authByteArray,
                            0,
                            pCallbackContext->pAuthInfo->size,
                            (const jbyte*) pCallbackContext->pAuthInfo->data);

    // Invoke the callback
    retStatus = env->CallIntMethod(pWrapper->mGlobalJniObjRef,
                                   pWrapper->mDescribeStreamMethodId,
                                   jstrStreamName,
                                   pCallbackContext->callAfter,
                                   pCallbackContext->timeout,
                                   authByteArray,
                                   pCallbackContext->pAuthInfo->type,
                                   pCallbackContext->customData
    );

    CHK_JVM_EXCEPTION(env);

CleanUp:

    if (jstrStreamName != NULL) {
        env->DeleteLocalRef(jstrStreamName);
    }

    if (authByteArray != NULL) {
        env->DeleteLocalRef(authByteArray);
    }

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::getStreamingEndpointFunc(UINT64 customData,
                                                     PCHAR streamName,
                                                     PCHAR apiName,
                                                     PServiceCallContext pCallbackContext)
{
    DLOGS("TID 0x%016" PRIx64 " getStreamingEndpointFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Get the ENV from the JavaVM
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;
    jstring jstrStreamName = NULL;
    jstring jstrApiName = NULL;
    jbyteArray authByteArray = NULL;

    INT32 envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func
    jstrStreamName = env->NewStringUTF(streamName);
    jstrApiName = env->NewStringUTF(apiName);
    authByteArray = env->NewByteArray(pCallbackContext->pAuthInfo->size);
    if (jstrStreamName == NULL ||
            jstrApiName == NULL ||
            authByteArray == NULL) {
        retStatus = STATUS_NOT_ENOUGH_MEMORY;
        goto CleanUp;
    }

    // Copy the bits into the managed array
    env->SetByteArrayRegion(authByteArray,
                            0,
                            pCallbackContext->pAuthInfo->size,
                            (const jbyte*) pCallbackContext->pAuthInfo->data);

    // Invoke the callback
    retStatus = env->CallIntMethod(pWrapper->mGlobalJniObjRef,
                                   pWrapper->mGetStreamingEndpointMethodId,
                                   jstrStreamName,
                                   jstrApiName,
                                   pCallbackContext->callAfter,
                                   pCallbackContext->timeout,
                                   authByteArray,
                                   pCallbackContext->pAuthInfo->type,
                                   pCallbackContext->customData
    );

    CHK_JVM_EXCEPTION(env);

CleanUp:

    if (jstrStreamName != NULL) {
        env->DeleteLocalRef(jstrStreamName);
    }

    if (authByteArray != NULL) {
        env->DeleteLocalRef(authByteArray);
    }

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::getStreamingTokenFunc(UINT64 customData,
                                                  PCHAR streamName,
                                                  STREAM_ACCESS_MODE accessMode,
                                                  PServiceCallContext pCallbackContext)
{
    DLOGS("TID 0x%016" PRIx64 " getStreamingTokenFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Get the ENV from the JavaVM
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;
    jstring jstrStreamName = NULL;
    jbyteArray authByteArray = NULL;

    INT32 envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func
    jstrStreamName = env->NewStringUTF(streamName);
    authByteArray = env->NewByteArray(pCallbackContext->pAuthInfo->size);
    if (jstrStreamName == NULL ||
            authByteArray == NULL) {
        retStatus = STATUS_NOT_ENOUGH_MEMORY;
        goto CleanUp;
    }

    // Copy the bits into the managed array
    env->SetByteArrayRegion(authByteArray,
                            0,
                            pCallbackContext->pAuthInfo->size,
                            (const jbyte*) pCallbackContext->pAuthInfo->data);

    // Invoke the callback
    retStatus = env->CallIntMethod(pWrapper->mGlobalJniObjRef,
                                   pWrapper->mGetStreamingTokenMethodId,
                                   jstrStreamName,
                                   pCallbackContext->callAfter,
                                   pCallbackContext->timeout,
                                   authByteArray,
                                   pCallbackContext->pAuthInfo->type,
                                   pCallbackContext->customData
    );

    CHK_JVM_EXCEPTION(env);

CleanUp:

    if (jstrStreamName != NULL) {
        env->DeleteLocalRef(jstrStreamName);
    }

    if (authByteArray != NULL) {
        env->DeleteLocalRef(authByteArray);
    }

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::putStreamFunc(UINT64 customData,
                                          PCHAR streamName,
                                          PCHAR containerType,
                                          UINT64 streamStartTime,
                                          BOOL absoluteFragmentTimestamp,
                                          BOOL ackRequired,
                                          PCHAR streamingEndpoint,
                                          PServiceCallContext pCallbackContext)
{
    DLOGS("TID 0x%016" PRIx64 " putStreamFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Get the ENV from the JavaVM
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;
    jstring jstrStreamName = NULL, jstrContainerType = NULL, jstrStreamingEndpoint = NULL;
    jbyteArray authByteArray = NULL;

    INT32 envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func
    jstrStreamName = env->NewStringUTF(streamName);
    jstrContainerType = env->NewStringUTF(containerType);
    jstrStreamingEndpoint = env->NewStringUTF(streamingEndpoint);
    authByteArray = env->NewByteArray(pCallbackContext->pAuthInfo->size);
    if (jstrStreamName == NULL ||
            jstrContainerType == NULL ||
            jstrStreamingEndpoint == NULL ||
            authByteArray == NULL) {
        retStatus = STATUS_NOT_ENOUGH_MEMORY;
        goto CleanUp;
    }

    // Copy the bits into the managed array
    env->SetByteArrayRegion(authByteArray,
                            0,
                            pCallbackContext->pAuthInfo->size,
                            (const jbyte*) pCallbackContext->pAuthInfo->data);

    // Invoke the callback
    retStatus = env->CallIntMethod(pWrapper->mGlobalJniObjRef,
                                   pWrapper->mPutStreamMethodId,
                                   jstrStreamName,
                                   jstrContainerType,
                                   streamStartTime,
                                   absoluteFragmentTimestamp == TRUE,
                                   ackRequired == TRUE,
                                   jstrStreamingEndpoint,
                                   pCallbackContext->callAfter,
                                   pCallbackContext->timeout,
                                   authByteArray,
                                   pCallbackContext->pAuthInfo->type,
                                   pCallbackContext->customData
    );

    CHK_JVM_EXCEPTION(env);

CleanUp:

    if (jstrStreamName != NULL) {
        env->DeleteLocalRef(jstrStreamName);
    }

    if (jstrContainerType != NULL) {
        env->DeleteLocalRef(jstrContainerType);
    }

    if (authByteArray != NULL) {
        env->DeleteLocalRef(authByteArray);
    }

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::tagResourceFunc(UINT64 customData,
                                            PCHAR streamArn,
                                            UINT32 tagCount,
                                            PTag tags,
                                            PServiceCallContext pCallbackContext)
{
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;
    jstring jstrStreamArn = NULL, jstrTagName = NULL, jstrTagValue = NULL;
    jbyteArray authByteArray = NULL;
    jobjectArray tagArray = NULL;
    jobject tag = NULL;
    jmethodID methodId = NULL;
    jclass tagClass = NULL;
    INT32 envState;
    UINT32 i;

    DLOGS("TID 0x%016" PRIx64 " tagResourceFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Early return if no tags
    CHK(tagCount != 0 && tags != NULL, STATUS_SUCCESS);

    // Get the ENV from the JavaVM and ensure we have a JVM thread
    envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func to create a new string
    jstrStreamArn = env->NewStringUTF(streamArn);

    // Allocate a new byte array
    authByteArray = env->NewByteArray(pCallbackContext->pAuthInfo->size);

    // Get the class object for tags
    tagClass = env->FindClass("com/amazonaws/kinesisvideo/producer/Tag");
    CHK(tagClass != NULL, STATUS_INVALID_OPERATION);

    // Get the Tag constructor method id
    methodId = env->GetMethodID(tagClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;)V");
    CHK(methodId != NULL, STATUS_INVALID_OPERATION);

    // Allocate a new object array for tags
    tagArray = env->NewObjectArray((jsize) tagCount, tagClass, NULL);

    if (jstrStreamArn == NULL || authByteArray == NULL || tagArray == NULL) {
        retStatus = STATUS_NOT_ENOUGH_MEMORY;
        goto CleanUp;
    }

    for (i = 0; i < tagCount; i++) {
        // Create the strings
        jstrTagName = env->NewStringUTF(tags[i].name);
        jstrTagValue = env->NewStringUTF(tags[i].value);
        CHK(jstrTagName != NULL && jstrTagValue != NULL, STATUS_NOT_ENOUGH_MEMORY);

        // Create a new tag object
        tag = env->NewObject(tagClass, methodId, jstrTagName, jstrTagValue);
        CHK(tag != NULL, STATUS_NOT_ENOUGH_MEMORY);

        // Set the value of the array index
        env->SetObjectArrayElement(tagArray, i, tag);

        // Remove the references to the constructed strings
        env->DeleteLocalRef(jstrTagName);
        env->DeleteLocalRef(jstrTagValue);
    }

    // Copy the bits into the managed array
    env->SetByteArrayRegion(authByteArray,
                            0,
                            pCallbackContext->pAuthInfo->size,
                            (const jbyte*) pCallbackContext->pAuthInfo->data);

    // Invoke the callback
    retStatus = env->CallIntMethod(pWrapper->mGlobalJniObjRef,
                                   pWrapper->mTagResourceMethodId,
                                   jstrStreamArn,
                                   tagArray,
                                   pCallbackContext->callAfter,
                                   pCallbackContext->timeout,
                                   authByteArray,
                                   pCallbackContext->pAuthInfo->type,
                                   pCallbackContext->customData
    );

    CHK_JVM_EXCEPTION(env);

CleanUp:

    if (jstrStreamArn != NULL) {
        env->DeleteLocalRef(jstrStreamArn);
    }

    if (authByteArray != NULL) {
        env->DeleteLocalRef(authByteArray);
    }

    if (tagArray != NULL) {
        env->DeleteLocalRef(tagArray);
    }

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::getAuthInfo(jmethodID methodId, PBYTE* ppCert, PUINT32 pSize, PUINT64 pExpiration)
{
    // Get the ENV from the JavaVM
    JNIEnv *env;

    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;
    jbyteArray byteArray = NULL;
    jobject jAuthInfoObj = NULL;
    jbyte* bufferPtr = NULL;
    jsize arrayLen = 0;
    jclass authCls = NULL;
    jint authType = 0;
    jlong authExpiration = 0;
    jmethodID authTypeMethodId = NULL;
    jmethodID authDataMethodId = NULL;
    jmethodID authExpirationMethodId = NULL;

    // Store this pointer so we can run the common macros
    KinesisVideoClientWrapper *pWrapper = this;

    INT32 envState = mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func
    jAuthInfoObj = env->CallObjectMethod(mGlobalJniObjRef, methodId);

    // Extract the method IDs for the auth object
    authCls = env->GetObjectClass(jAuthInfoObj);
    CHK_JVM_EXCEPTION(env);
    if (authCls == NULL) {
        DLOGE("Failed to get the object class for the AuthInfo object.");
        retStatus = STATUS_INVALID_ARG;
        goto CleanUp;
    }

    // Extract the method ids
    authTypeMethodId = env->GetMethodID(authCls, "getIntAuthType", "()I");
    CHK_JVM_EXCEPTION(env);
    authDataMethodId = env->GetMethodID(authCls, "getData", "()[B");
    CHK_JVM_EXCEPTION(env);
    authExpirationMethodId = env->GetMethodID(authCls, "getExpiration", "()J");
    CHK_JVM_EXCEPTION(env);
    if (authTypeMethodId == NULL || authDataMethodId == NULL || authExpirationMethodId == NULL) {
        DLOGE("Couldn't find methods in AuthType object");
        retStatus = STATUS_INVALID_ARG;
        goto CleanUp;
    }

    authType = env->CallIntMethod(jAuthInfoObj, authTypeMethodId);
    CHK_JVM_EXCEPTION(env);

    authExpiration = env->CallLongMethod(jAuthInfoObj, authExpirationMethodId);
    CHK_JVM_EXCEPTION(env);

    byteArray = (jbyteArray) env->CallObjectMethod(jAuthInfoObj, authDataMethodId);
    CHK_JVM_EXCEPTION(env);

    if (byteArray != NULL) {
        // Extract the bits from the byte buffer
        bufferPtr = env->GetByteArrayElements(byteArray, NULL);
        arrayLen = env->GetArrayLength(byteArray);

        if (arrayLen >= MAX_AUTH_LEN) {
            retStatus = STATUS_INVALID_ARG;
            goto CleanUp;
        }

        // Copy the bits
        MEMCPY(mAuthInfo.data, bufferPtr, arrayLen);
        mAuthInfo.type = authInfoTypeFromInt((UINT32) authType);
        mAuthInfo.expiration = (UINT64) authExpiration;
        mAuthInfo.size = arrayLen;

        // Set the returned values
        *ppCert = (PBYTE) &mAuthInfo.data;
        *pSize = mAuthInfo.size;
        *pExpiration = mAuthInfo.expiration;
    } else {
        mAuthInfo.type = AUTH_INFO_NONE;
        mAuthInfo.size = 0;
        mAuthInfo.expiration = 0;

        // Set the returned values
        *ppCert = NULL;
        *pSize = 0;
        *pExpiration = 0;
    }

CleanUp:

    // Release the array object if allocated
    if (byteArray != NULL) {
        env->ReleaseByteArrayElements(byteArray, bufferPtr, 0);
    }

    // Detach the thread if we have attached it to JVM
    if (detached) {
        mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::clientReadyFunc(UINT64 customData, CLIENT_HANDLE clientHandle)
{
    DLOGS("TID 0x%016" PRIx64 " clientReadyFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Get the ENV from the JavaVM
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;

    INT32 envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func
    env->CallVoidMethod(pWrapper->mGlobalJniObjRef, pWrapper->mClientReadyMethodId, (jlong) TO_WRAPPER_HANDLE(pWrapper));
    CHK_JVM_EXCEPTION(env);

CleanUp:

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::createDeviceFunc(UINT64 customData, PCHAR deviceName, PServiceCallContext pCallbackContext)
{
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;
    jstring jstrDeviceName = NULL;
    jbyteArray authByteArray = NULL;
    INT32 envState;

    DLOGS("TID 0x%016" PRIx64 " createDeviceFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Device name should be valid
    CHK(deviceName != 0, STATUS_NULL_ARG);

    // Get the ENV from the JavaVM and ensure we have a JVM thread
    envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func to create a new string
    jstrDeviceName = env->NewStringUTF(deviceName);

    // Allocate a new byte array
    authByteArray = env->NewByteArray(pCallbackContext->pAuthInfo->size);

    if (jstrDeviceName == NULL || authByteArray == NULL) {
        retStatus = STATUS_NOT_ENOUGH_MEMORY;
        goto CleanUp;
    }

    // Copy the bits into the managed array
    env->SetByteArrayRegion(authByteArray,
                            0,
                            pCallbackContext->pAuthInfo->size,
                            (const jbyte*) pCallbackContext->pAuthInfo->data);

    // Invoke the callback
    retStatus = env->CallIntMethod(pWrapper->mGlobalJniObjRef,
                                   pWrapper->mCreateDeviceMethodId,
                                   jstrDeviceName,
                                   pCallbackContext->callAfter,
                                   pCallbackContext->timeout,
                                   authByteArray,
                                   pCallbackContext->pAuthInfo->type,
                                   pCallbackContext->customData
    );

    CHK_JVM_EXCEPTION(env);

CleanUp:

    if (jstrDeviceName != NULL) {
        env->DeleteLocalRef(jstrDeviceName);
    }

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

STATUS KinesisVideoClientWrapper::deviceCertToTokenFunc(UINT64 customData, PCHAR deviceName, PServiceCallContext pCallbackContext)
{
    JNIEnv *env;
    BOOL detached = FALSE;
    STATUS retStatus = STATUS_SUCCESS;
    jstring jstrDeviceName = NULL;
    jbyteArray authByteArray = NULL;
    INT32 envState;

    DLOGS("TID 0x%016" PRIx64 " deviceCertToTokenFunc called.", GETTID());

    KinesisVideoClientWrapper *pWrapper = FROM_WRAPPER_HANDLE(customData);
    CHECK(pWrapper != NULL);

    // Device name should be valid
    CHK(deviceName != 0, STATUS_NULL_ARG);

    // Get the ENV from the JavaVM and ensure we have a JVM thread
    envState = pWrapper->mJvm->GetEnv((PVOID*) &env, JNI_VERSION_1_6);
    if (envState == JNI_EDETACHED) {
        ATTACH_CURRENT_THREAD_TO_JVM(env);

        // Store the detached so we can detach the thread after the call
        detached = TRUE;
    }

    // Call the Java func to create a new string
    jstrDeviceName = env->NewStringUTF(deviceName);

    // Allocate a new byte array
    authByteArray = env->NewByteArray(pCallbackContext->pAuthInfo->size);

    if (jstrDeviceName == NULL || authByteArray == NULL) {
        retStatus = STATUS_NOT_ENOUGH_MEMORY;
        goto CleanUp;
    }

    // Copy the bits into the managed array
    env->SetByteArrayRegion(authByteArray,
                            0,
                            pCallbackContext->pAuthInfo->size,
                            (const jbyte*) pCallbackContext->pAuthInfo->data);

    // Invoke the callback
    retStatus = env->CallIntMethod(pWrapper->mGlobalJniObjRef,
                                   pWrapper->mDeviceCertToTokenMethodId,
                                   jstrDeviceName,
                                   pCallbackContext->callAfter,
                                   pCallbackContext->timeout,
                                   authByteArray,
                                   pCallbackContext->pAuthInfo->type,
                                   pCallbackContext->customData
    );

    CHK_JVM_EXCEPTION(env);

CleanUp:

    if (jstrDeviceName != NULL) {
        env->DeleteLocalRef(jstrDeviceName);
    }

    // Detach the thread if we have attached it to JVM
    if (detached) {
        pWrapper->mJvm->DetachCurrentThread();
    }

    return retStatus;
}

AUTH_INFO_TYPE KinesisVideoClientWrapper::authInfoTypeFromInt(UINT32 authInfoType)
{
    switch (authInfoType) {
        case 1: return AUTH_INFO_TYPE_CERT;
        case 2: return AUTH_INFO_TYPE_STS;
        case 3: return AUTH_INFO_NONE;
        default: return AUTH_INFO_UNDEFINED;
    }
}