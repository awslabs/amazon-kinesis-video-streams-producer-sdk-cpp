#include <jni.h>
/* Header for class com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni */

#ifndef _Included_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
#define _Included_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
#ifdef __cplusplus
extern "C" {
#endif
#undef com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_INVALID_CLIENT_HANDLE_VALUE
#define com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_INVALID_CLIENT_HANDLE_VALUE 0LL
#undef com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_INVALID_STREAM_HANDLE_VALUE
#define com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_INVALID_STREAM_HANDLE_VALUE 0LL
/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    getNativeLibraryVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_getNativeLibraryVersion
  (JNIEnv *, jobject);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    getNativeCodeCompileTime
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_getNativeCodeCompileTime
  (JNIEnv *, jobject);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    createKinesisVideoClient
 * Signature: (Lcom/amazonaws/kinesisvideo/producer/DeviceInfo;)J
 */
JNIEXPORT jlong JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_createKinesisVideoClient
  (JNIEnv *, jobject, jobject);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    freeKinesisVideoClient
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_freeKinesisVideoClient
  (JNIEnv *, jobject, jlong);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    stopKinesisVideoStreams
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_stopKinesisVideoStreams
  (JNIEnv *, jobject, jlong);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    stopKinesisVideoStream
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_stopKinesisVideoStream
(JNIEnv *, jobject, jlong, jlong);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    freeKinesisVideoStream
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_freeKinesisVideoStream
        (JNIEnv *, jobject, jlong, jlong);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    getKinesisVideoMetrics
 * Signature: (JLcom/amazonaws/kinesisvideo/internal/producer/KinesisVideoMetrics;)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_getKinesisVideoMetrics
(JNIEnv *, jobject, jlong, jobject);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    getKinesisVideoStreamMetrics
 * Signature: (JJLcom/amazonaws/kinesisvideo/internal/producer/KinesisVideoStreamMetrics;)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_getKinesisVideoStreamMetrics
(JNIEnv *, jobject, jlong, jlong, jobject);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    createKinesisVideoStream
 * Signature: (JLcom/amazonaws/kinesisvideo/producer/StreamInfo;)J
 */
JNIEXPORT jlong JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_createKinesisVideoStream
  (JNIEnv *, jobject, jlong, jobject);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    putKinesisVideoFrame
 * Signature: (JJLcom/amazonaws/kinesisvideo/producer/KinesisVideoFrame;)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_putKinesisVideoFrame
  (JNIEnv *, jobject, jlong, jlong, jobject);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    putKinesisVideoFragmentMetadata
 * Signature: (JJLjava/lang/String;Ljava/lang/String;Z)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_putKinesisVideoFragmentMetadata
  (JNIEnv*, jobject, jlong, jlong, jstring, jstring, jboolean);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    kinesisVideoStreamFragmentAck
 * Signature: (JJJLcom/amazonaws/kinesisvideo/producer/KinesisVideoFragmentAck;)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_kinesisVideoStreamFragmentAck
  (JNIEnv *, jobject, jlong, jlong, jlong, jobject);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    kinesisVideoStreamParseFragmentAck
 * Signature: (JJJLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_kinesisVideoStreamParseFragmentAck
(JNIEnv *, jobject, jlong, jlong, jlong, jstring);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    describeStreamResultEvent
 * Signature: (JJILcom/amazonaws/kinesisvideo/producer/StreamDescription;)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_describeStreamResultEvent
  (JNIEnv *, jobject, jlong, jlong, jint, jobject);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    getStreamingEndpointResultEvent
 * Signature: (JJILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_getStreamingEndpointResultEvent
  (JNIEnv *, jobject, jlong, jlong, jint, jstring);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    getStreamingTokenResultEvent
 * Signature: (JJI[BIJ)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_getStreamingTokenResultEvent
  (JNIEnv *, jobject, jlong, jlong, jint, jobject, jint, jlong);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    createStreamResultEvent
 * Signature: (JJILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_createStreamResultEvent
  (JNIEnv *, jobject, jlong, jlong, jint, jstring);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    putStreamResultEvent
 * Signature: (JJIJ)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_putStreamResultEvent
  (JNIEnv *, jobject, jlong, jlong, jint, jlong);

/*
* Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
* Method:    tagResourceResultEvent
* Signature: (JJI)V
*/
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_tagResourceResultEvent
  (JNIEnv *, jobject, jlong, jlong, jint);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    getKinesisVideoStreamData
 * Signature: (JJJ[BIILcom/amazonaws/kinesisvideo/internal/producer/ReadResult;)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_getKinesisVideoStreamData
  (JNIEnv *, jobject, jlong, jlong, jlong, jobject, jint, jint, jobject);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    kinesisVideoStreamFormatChanged
 * Signature: (JJ[BJ)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_kinesisVideoStreamFormatChanged
  (JNIEnv *, jobject, jlong, jlong, jobject, jlong);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    createDeviceResultEvent
 * Signature: (JJILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_createDeviceResultEvent
  (JNIEnv *, jobject, jlong, jlong, jint, jstring);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    deviceCertToTokenResultEvent
 * Signature: (JJI[BIJ)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_deviceCertToTokenResultEvent
  (JNIEnv *, jobject, jlong, jlong, jint, jobject, jint, jlong);

/*
 * Class:     com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni
 * Method:    kinesisVideoStreamTerminated
 * Signature: (JJJI)V
 */
JNIEXPORT void JNICALL Java_com_amazonaws_kinesisvideo_internal_producer_jni_NativeKinesisVideoProducerJni_kinesisVideoStreamTerminated
(JNIEnv *, jobject, jlong, jlong, jlong, jint);

#ifdef __cplusplus
}
#endif
#endif
