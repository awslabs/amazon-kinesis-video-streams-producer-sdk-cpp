/**
 * Simple kinesis video producer client wrapper for JNI
 */
#ifndef __KINESIS_VIDEO_PRODUCER_CLIENT_WRAPPER_H__
#define __KINESIS_VIDEO_PRODUCER_CLIENT_WRAPPER_H__

#pragma once

#define HAVE_PTHREADS 1           // Makes threads.h use pthreads

#include "SyncMutex.h"
#include "TimedSemaphore.h"
#include "JNICommon.h"
#include "Parameters.h"

#define TO_WRAPPER_HANDLE(p)                    ((jlong) (p))
#define FROM_WRAPPER_HANDLE(h)                  ((KinesisVideoClientWrapper*) (h))

#define JNI_VER JNI_VERSION_1_6

#define CHK_JVM_EXCEPTION(env) \
    do { \
        /* If there was an exception we need */ \
        if (env->ExceptionCheck() == JNI_TRUE) { \
            /* Print out the message to the stderr */ \
            env->ExceptionDescribe(); \
            /* Clear the exception */ \
            env->ExceptionClear(); \
            /* Terminate the process as we didn't expect any exceptions */ \
            DLOGE("JVM threw an unexpected exception."); \
            retStatus = STATUS_INVALID_OPERATION; \
            goto CleanUp; \
        } \
    } while (FALSE)

#ifdef ANDROID_BUILD
#define ATTACH_CURRENT_THREAD_TO_JVM(env) \
    do { \
        if (pWrapper->mJvm->AttachCurrentThread(&env, NULL) != 0) { \
            DLOGE("Fail to attache to JVM!");\
            return STATUS_INVALID_OPERATION; \
        } \
    } while (FALSE)
#else
#define ATTACH_CURRENT_THREAD_TO_JVM(env) \
    do { \
        if (pWrapper->mJvm->AttachCurrentThread((PVOID*) &env, NULL) != 0) { \
            DLOGE("Fail to attache to JVM!");\
            return STATUS_INVALID_OPERATION; \
        } \
    } while (FALSE)
#endif

class KinesisVideoClientWrapper
{
    CLIENT_HANDLE mClientHandle;
    static JavaVM *mJvm; // scope revised to static to make it accessible from static function- logPrintFunc 
    static jobject mGlobalJniObjRef; // scope revised to static to make it accessible from static function- logPrintFunc
    ClientCallbacks mClientCallbacks;
    DeviceInfo mDeviceInfo;
    AuthInfo mAuthInfo;
    SyncMutex mSyncLock;

    // Extracted method IDs
    jmethodID mGetDeviceCertificateMethodId;
    jmethodID mGetSecurityTokenMethodId;
    jmethodID mGetDeviceFingerprintMethodId;
    jmethodID mStreamUnderflowReportMethodId;
    jmethodID mStorageOverflowPressureMethodId;
    jmethodID mStreamLatencyPressureMethodId;
    jmethodID mStreamConnectionStaleMethodId;
    jmethodID mFragmentAckReceivedMethodId;
    jmethodID mDroppedFrameReportMethodId;
    jmethodID mBufferDurationOverflowPressureMethodId;
    jmethodID mDroppedFragmentReportMethodId;
    jmethodID mStreamErrorReportMethodId;
    jmethodID mStreamDataAvailableMethodId;
    jmethodID mStreamReadyMethodId;
    jmethodID mStreamClosedMethodId;
    jmethodID mCreateStreamMethodId;
    jmethodID mDescribeStreamMethodId;
    jmethodID mGetStreamingEndpointMethodId;
    jmethodID mGetStreamingTokenMethodId;
    jmethodID mPutStreamMethodId;
    jmethodID mTagResourceMethodId;
    jmethodID mClientReadyMethodId;
    jmethodID mCreateDeviceMethodId;
    jmethodID mDeviceCertToTokenMethodId;
    static jmethodID mLogPrintMethodId;

    //////////////////////////////////////////////////////////////////////////////////////
    // Internal private methods
    //////////////////////////////////////////////////////////////////////////////////////
    STATUS getAuthInfo(jmethodID, PBYTE*, PUINT32, PUINT64);
    static AUTH_INFO_TYPE authInfoTypeFromInt(UINT32);

    //////////////////////////////////////////////////////////////////////////////////////
    // Static callbacks definitions
    //////////////////////////////////////////////////////////////////////////////////////
    static UINT64 getCurrentTimeFunc(UINT64);
    static UINT32 getRandomNumberFunc(UINT64);
    static STATUS getDeviceCertificateFunc(UINT64, PBYTE*, PUINT32, PUINT64);
    static STATUS getSecurityTokenFunc(UINT64, PBYTE*, PUINT32, PUINT64);
    static STATUS getDeviceFingerprintFunc(UINT64, PCHAR*);
    static STATUS streamUnderflowReportFunc(UINT64, STREAM_HANDLE);
    static STATUS storageOverflowPressureFunc(UINT64, UINT64);
    static STATUS streamLatencyPressureFunc(UINT64, STREAM_HANDLE, UINT64);
    static STATUS streamConnectionStaleFunc(UINT64, STREAM_HANDLE, UINT64);
    static STATUS fragmentAckReceivedFunc(UINT64, STREAM_HANDLE, UPLOAD_HANDLE, PFragmentAck);
    static STATUS droppedFrameReportFunc(UINT64, STREAM_HANDLE, UINT64);
    static STATUS bufferDurationOverflowPressureFunc(UINT64, STREAM_HANDLE, UINT64);
    static STATUS droppedFragmentReportFunc(UINT64, STREAM_HANDLE, UINT64);
    static STATUS streamErrorReportFunc(UINT64, STREAM_HANDLE, UPLOAD_HANDLE, UINT64, STATUS);
    static STATUS streamDataAvailableFunc(UINT64, STREAM_HANDLE, PCHAR, UINT64, UINT64, UINT64);
    static STATUS streamReadyFunc(UINT64, STREAM_HANDLE);
    static STATUS streamClosedFunc(UINT64, STREAM_HANDLE, UINT64);
    static MUTEX createMutexFunc(UINT64, BOOL);
    static VOID lockMutexFunc(UINT64, MUTEX);
    static VOID unlockMutexFunc(UINT64, MUTEX);
    static BOOL tryLockMutexFunc(UINT64, MUTEX);
    static VOID freeMutexFunc(UINT64, MUTEX);
    static CVAR createConditionVariableFunc(UINT64);
    static STATUS signalConditionVariableFunc(UINT64, CVAR);
    static STATUS broadcastConditionVariableFunc(UINT64, CVAR);
    static STATUS waitConditionVariableFunc(UINT64, CVAR, MUTEX, UINT64);
    static VOID freeConditionVariableFunc(UINT64, CVAR);
    static STATUS createStreamFunc(UINT64,
                                   PCHAR,
                                   PCHAR,
                                   PCHAR,
                                   PCHAR,
                                   UINT64,
                                   PServiceCallContext);
    static STATUS describeStreamFunc(UINT64,
                                     PCHAR,
                                     PServiceCallContext);
    static STATUS getStreamingEndpointFunc(UINT64,
                                           PCHAR,
                                           PCHAR,
                                           PServiceCallContext);
    static STATUS getStreamingTokenFunc(UINT64,
                                        PCHAR,
                                        STREAM_ACCESS_MODE,
                                        PServiceCallContext);
    static STATUS putStreamFunc(UINT64,
                                PCHAR,
                                PCHAR,
                                UINT64,
                                BOOL,
                                BOOL,
                                PCHAR,
                                PServiceCallContext);

    static STATUS tagResourceFunc(UINT64,
                                  PCHAR,
                                  UINT32,
                                  PTag,
                                  PServiceCallContext);

    static STATUS clientReadyFunc(UINT64,
                                  CLIENT_HANDLE);

    static STATUS createDeviceFunc(UINT64,
                                   PCHAR,
                                   PServiceCallContext);

    static STATUS deviceCertToTokenFunc(UINT64,
                                        PCHAR,
                                        PServiceCallContext);
    static VOID logPrintFunc(UINT32, PCHAR, PCHAR, ...);

public:
    KinesisVideoClientWrapper(JNIEnv* env,
                        jobject thiz,
                        jobject deviceInfo);

    ~KinesisVideoClientWrapper();
    SyncMutex& getSyncLock();
    void deleteGlobalRef(JNIEnv* env);
    jobject getGlobalRef();
    void stopKinesisVideoStreams();
    STREAM_HANDLE createKinesisVideoStream(jobject streamInfo);
    void getKinesisVideoMetrics(jobject kinesisVideoMetrics);
    void getKinesisVideoStreamMetrics(jlong streamHandle, jobject kinesisVideoStreamMetrics);
    void stopKinesisVideoStream(jlong streamHandle);
    void freeKinesisVideoStream(jlong streamHandle);
    void putKinesisVideoFrame(jlong streamHandle, jobject kinesisVideoFrame);
    void putKinesisVideoFragmentMetadata(jlong streamHandle, jstring metadataName, jstring metadataValue, jboolean persistent);
    void describeStreamResult(jlong streamHandle, jint httpStatusCode, jobject streamDescription);
    void kinesisVideoStreamTerminated(jlong streamHandle, jlong uploadHandle, jint httpStatusCode);
    void getStreamingEndpointResult(jlong streamHandle, jint httpStatusCode, jstring streamingEndpoint);
    void getStreamingTokenResult(jlong streamHandle, jint httpStatusCode, jbyteArray token, jint tokenSize, jlong expiration);
    void createStreamResult(jlong streamHandle, jint httpStatusCode, jstring streamArn);
    void putStreamResult(jlong streamHandle, jint httpStatusCode, jlong clientStreamHandle);
    void tagResourceResult(jlong customData, jint httpStatusCode);
    void getKinesisVideoStreamData(jlong streamHandle, jlong uploadHandle, jobject dataBuffer, jint offset, jint length, jobject readResult);
    void streamFormatChanged(jlong streamHandle, jobject codecPrivateData, jlong trackId);
    void createDeviceResult(jlong clientHandle, jint httpStatusCode, jstring deviceArn);
    void deviceCertToTokenResult(jlong clientHandle, jint httpStatusCode, jbyteArray token, jint tokenSize, jlong expiration);
    void kinesisVideoStreamFragmentAck(jlong streamHandle, jlong uploadHandle, jobject fragmentAck);
    void kinesisVideoStreamParseFragmentAck(jlong streamHandle, jlong uploadHandle, jstring ack);
private:
    BOOL setCallbacks(JNIEnv* env, jobject thiz);
};

#endif // __KINESIS_VIDEO_PRODUCER_CLIENT_WRAPPER_H__
