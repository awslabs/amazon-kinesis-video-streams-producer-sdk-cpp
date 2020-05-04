/**
 * C part of the JNI interface to the native producer client functionality.
 */

#define LOG_CLASS "KinesisVideoProducerJNI"
#include "com/amazonaws/kinesis/video/producer/jni/com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni.h"
#include "com/amazonaws/kinesis/video/producer/jni/KinesisVideoClientWrapper.h"

// Used to detect version mismatches at runtime between the native interface library and the Java code that uses it.
// This must match the value returned by EXPECTED_LIBRARY_VERSION in java JNI counterpart.
//
// IMPORTANT: This version number *must* be incremented every time a new library build is checked in.
// We are seeing a very high incidence of runtime failures due to APKs being deployed with the wrong native libraries,
// and they are much easier to diagnose when the numeric version check fails.
#define NATIVE_LIBRARY_VERSION "2.0"

//
// JNI entry points
//
#ifdef __cplusplus
extern "C" {
#endif
    /**
     * Returns a hardcoded library version string.
     */
    PUBLIC_API jstring JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_getNativeLibraryVersion(JNIEnv* env, jobject thiz)
    {
        return env->NewStringUTF(NATIVE_LIBRARY_VERSION);
    }

    /**
     * Returns a string representing the date and time when this code was compiled.
     */
    PUBLIC_API jstring JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_getNativeCodeCompileTime(JNIEnv* env, jobject thiz)
    {
        return env->NewStringUTF(__DATE__ " " __TIME__);
    }

    /**
     * Releases the kinesis video client object. All operations will fail from this moment on
     */
    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_freeKinesisVideoClient(JNIEnv* env, jobject thiz, jlong handle)
    {
        ENTER();


        DLOGI("Freeing Kinesis Video client.");
        CHECK(env != NULL && thiz != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            // Cache the globalRef for later deletion
            jobject globalRef = pWrapper->getGlobalRef();

            // Free the existing engine
            delete pWrapper;

            // Free the global reference
            if (globalRef != NULL) {
                env->DeleteGlobalRef(globalRef);
            }
        }

        LEAVE();
    }

    /**
     * Creates and initializes the kinesis video client
     */
    PUBLIC_API jlong JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_createKinesisVideoClient(JNIEnv* env, jobject thiz, jobject deviceInfo)
    {
        ENTER();

        KinesisVideoClientWrapper* pWrapper = NULL;
        jlong retValue = (jlong) NULL;

        DLOGI("Creating Kinesis Video client.");
        CHECK(env != NULL && thiz != NULL);

        if (deviceInfo == NULL) {
            DLOGE("DeviceInfo is NULL.");
            throwNativeException(env, EXCEPTION_NAME, "DeviceInfo is NULL.", STATUS_NULL_ARG);
            goto CleanUp;
        }

        // Create the wrapper engine
        pWrapper = new KinesisVideoClientWrapper(env, thiz, deviceInfo);

        // Returning the pointer as a handle
        retValue = (jlong) TO_WRAPPER_HANDLE(pWrapper);

CleanUp:

        LEAVE();
        return retValue;
    }

    /**
     * Stops KinesisVideo streams
     */
    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_stopKinesisVideoStreams(JNIEnv* env, jobject thiz, jlong handle)
    {
        ENTER();

        DLOGI("Stopping Kinesis Video streams.");
        CHECK(env != NULL && thiz != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->stopKinesisVideoStreams();
        }

        LEAVE();
    }

    /**
     * Stops a KinesisVideo stream
     */
    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_stopKinesisVideoStream(JNIEnv* env, jobject thiz, jlong handle, jlong streamHandle)
    {
        ENTER();

        DLOGI("Stopping Kinesis Video stream.");
        CHECK(env != NULL && thiz != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->stopKinesisVideoStream(streamHandle);
        }

        LEAVE();
    }

    /**
     * Frees a KinesisVideo stream.
     */
    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_freeKinesisVideoStream(JNIEnv* env, jobject thiz, jlong handle, jlong streamHandle)
    {
        ENTER();

        DLOGI("Stopping Kinesis Video stream.");
        CHECK(env != NULL && thiz != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->freeKinesisVideoStream(streamHandle);
        }

        LEAVE();
    }

    /**
     * Extracts the KinesisVideo client object metrics
     */
    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_getKinesisVideoMetrics(JNIEnv* env, jobject thiz, jlong handle, jobject kinesisVideoMetrics)
    {
        ENTERS();

        DLOGS("Getting Kinesis Video metrics.");
        CHECK(env != NULL && thiz != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->getKinesisVideoMetrics(kinesisVideoMetrics);
        }

        LEAVES();
    }

    /**
     * Extracts the KinesisVideo client object metrics
     */
    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_getKinesisVideoStreamMetrics(JNIEnv* env, jobject thiz, jlong handle, jlong streamHandle, jobject kinesisVideoStreamMetrics)
    {
        ENTERS();

        DLOGS("Getting Kinesis Video stream metrics.");
        CHECK(env != NULL && thiz != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->getKinesisVideoStreamMetrics(streamHandle, kinesisVideoStreamMetrics);
        }

        LEAVES();
    }

     /**
     * Creates and initializes the kinesis video client
     */
    PUBLIC_API jlong JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_createKinesisVideoStream(JNIEnv* env, jobject thiz, jlong handle, jobject streamInfo)
    {
        ENTER();
        jlong streamHandle = INVALID_STREAM_HANDLE_VALUE;

        DLOGI("Creating Kinesis Video stream.");
        CHECK(env != NULL && thiz != NULL && streamInfo != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            streamHandle = pWrapper->createKinesisVideoStream(streamInfo);
        }

        LEAVE();

        return streamHandle;
    }

    /**
     * Puts a frame in to the frame buffer
     */
    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_putKinesisVideoFrame(JNIEnv* env, jobject thiz, jlong handle, jlong streamHandle, jobject kinesisVideoFrame)
    {
        ENTERS();

        DLOGS("Putting Kinesis Video frame for stream 0x%016" PRIx64 ".", streamHandle);
        CHECK(env != NULL && thiz != NULL && kinesisVideoFrame != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->putKinesisVideoFrame(streamHandle, kinesisVideoFrame);
        }

        LEAVES();
    }

    /**
     * Puts a metadata in to the frame buffer
     */
    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_putKinesisVideoFragmentMetadata(JNIEnv* env, jobject thiz, jlong handle, jlong streamHandle, jstring metadataName, jstring metadataValue, jboolean persistent)
    {
        ENTERS();

        DLOGS("Putting Kinesis Video metadata for stream 0x%016" PRIx64 ".", streamHandle);
        CHECK(env != NULL && thiz != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->putKinesisVideoFragmentMetadata(streamHandle, metadataName, metadataValue, persistent);
        }

        LEAVES();
    }

    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_kinesisVideoStreamFragmentAck(JNIEnv* env, jobject thiz, jlong handle, jlong streamHandle, jlong uploadHandle, jobject fragmentAck)
    {
        ENTERS();

        DLOGS("Reporting Kinesis Video fragment ack for stream 0x%016" PRIx64 ".", streamHandle);
        CHECK(env != NULL && thiz != NULL && fragmentAck != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->kinesisVideoStreamFragmentAck(streamHandle, uploadHandle, fragmentAck);
        }

        LEAVES();
    }

    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_kinesisVideoStreamParseFragmentAck(JNIEnv* env, jobject thiz, jlong handle, jlong streamHandle, jlong uploadHandle, jstring ack)
    {
        ENTERS();

        DLOGS("Parsing Kinesis Video fragment ack for stream 0x%016" PRIx64 ".", streamHandle);
        CHECK(env != NULL && thiz != NULL && ack != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->kinesisVideoStreamParseFragmentAck(streamHandle, uploadHandle, ack);
        }

        LEAVES();
    }

    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_describeStreamResultEvent(JNIEnv* env, jobject thiz, jlong handle, jlong streamHandle, jint httpStatusCode, jobject streamDescription)
    {
        ENTER();

        DLOGI("Describe stream event for handle 0x%016" PRIx64 ".", (UINT64) handle);
        CHECK(env != NULL && thiz != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->describeStreamResult(streamHandle, httpStatusCode, streamDescription);
        }

        LEAVE();
    }

    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_getStreamingEndpointResultEvent(JNIEnv* env, jobject thiz, jlong handle, jlong streamHandle, jint httpStatusCode, jstring streamingEndpoint)
    {
        ENTER();

        DLOGI("get streaming endpoint event for handle 0x%016" PRIx64 ".", (UINT64) handle);
        CHECK(env != NULL && thiz != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->getStreamingEndpointResult(streamHandle, httpStatusCode, streamingEndpoint);
        }

        LEAVE();
    }

    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_getStreamingTokenResultEvent(JNIEnv* env, jobject thiz, jlong handle, jlong streamHandle, jint httpStatusCode, jobject streamingToken, jint tokenSize, jlong tokenExpiration)
    {
        ENTER();

        DLOGI("get streaming token event for handle 0x%016" PRIx64 ".", (UINT64) handle);
        CHECK(env != NULL && thiz != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->getStreamingTokenResult(streamHandle, httpStatusCode, (jbyteArray) streamingToken, tokenSize, tokenExpiration);
        }

        LEAVE();
    }

    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_createStreamResultEvent(JNIEnv* env, jobject thiz, jlong handle, jlong streamHandle, jint httpStatusCode, jstring streamArn)
    {
        ENTERS();

        DLOGI("create stream event for handle 0x%016" PRIx64 ".", (UINT64) handle);
        CHECK(env != NULL && thiz != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->createStreamResult(streamHandle, httpStatusCode, streamArn);
        }

        LEAVES();
    }

    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_putStreamResultEvent(JNIEnv* env, jobject thiz, jlong handle, jlong streamHandle, jint httpStatusCode, jlong clientStreamHandle)
    {
        ENTER();

        DLOGI("put stream event for handle 0x%016" PRIx64 ".", (UINT64) handle);
        CHECK(env != NULL && thiz != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->putStreamResult(streamHandle, httpStatusCode, clientStreamHandle);
        }

        LEAVE();
    }

    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_tagResourceResultEvent(JNIEnv* env, jobject thiz, jlong handle, jlong customData, jint httpStatusCode)
    {
        ENTER();

        DLOGI("tag resource event for handle 0x%016" PRIx64 ".", (UINT64) handle);
        CHECK(env != NULL && thiz != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->tagResourceResult(customData, httpStatusCode);
        }

        LEAVE();
    }

    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_getKinesisVideoStreamData(JNIEnv* env, jobject thiz, jlong handle, jlong streamHandle, jlong uploadHandle, jobject dataBuffer, jint offset, jint length, jobject readResult)
    {
        ENTERS();
        jint retStatus = STATUS_SUCCESS;

        DLOGS("get kinesis video stream data event for handle 0x%016" PRIx64 ".", (UINT64) handle);
        CHECK(env != NULL && thiz != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->getKinesisVideoStreamData(streamHandle, uploadHandle, dataBuffer, offset, length, readResult);
        }

        LEAVES();
    }

    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_kinesisVideoStreamFormatChanged(JNIEnv* env, jobject thiz, jlong handle, jlong streamHandle, jobject codecPrivateData, jlong trackId)
    {
        ENTER();

        DLOGI("stream format changed event for handle 0x%016" PRIx64 ".", (UINT64) handle);
        CHECK(env != NULL && thiz != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->streamFormatChanged(streamHandle, codecPrivateData, trackId);
        }

        LEAVE();
    }

    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_createDeviceResultEvent(JNIEnv* env, jobject thiz, jlong handle, jlong deviceHandle, jint httpStatusCode, jstring deviceArn)
    {
        ENTER();

        DLOGI("create device event for handle 0x%016" PRIx64 ".", (UINT64) handle);
        CHECK(env != NULL && thiz != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->createDeviceResult(deviceHandle, httpStatusCode, deviceArn);
        }

        LEAVE();
    }

    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_deviceCertToTokenResultEvent(JNIEnv* env, jobject thiz, jlong handle, jlong deviceHandle, jint httpStatusCode, jobject token, jint tokenSize, jlong tokenExpiration)
    {
        ENTER();

        DLOGI("device cert to token event for handle 0x%016" PRIx64 ".", (UINT64) handle);
        CHECK(env != NULL && thiz != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->deviceCertToTokenResult(deviceHandle, httpStatusCode, (jbyteArray) token, tokenSize, tokenExpiration);
        }

        LEAVE();
    }

    PUBLIC_API void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_kinesisVideoStreamTerminated(JNIEnv* env, jobject thiz, jlong handle, jlong streamHandle, jlong uploadHandle, jint httpStatusCode)
    {
        ENTER();

        DLOGI("Stream terminated event for handle 0x%016" PRIx64 ".", (UINT64) handle);
        CHECK(env != NULL && thiz != NULL);

        KinesisVideoClientWrapper* pWrapper = FROM_WRAPPER_HANDLE(handle);
        if (pWrapper != NULL) {
            SyncMutex::Autolock l(pWrapper->getSyncLock(), __FUNCTION__);
            pWrapper->kinesisVideoStreamTerminated(streamHandle, uploadHandle, httpStatusCode);
        }

        LEAVE();
    }

#ifdef __cplusplus
} // End extern "C"
#endif
