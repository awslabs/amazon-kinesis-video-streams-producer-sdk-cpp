/**
 * Parameter conversion for JNI calls
 */
#ifndef __KINESIS_VIDEO_PARAMETERS_CONVERSION_H__
#define __KINESIS_VIDEO_PARAMETERS_CONVERSION_H__

#pragma once

BOOL setDeviceInfo(JNIEnv* env, jobject deviceInfo, PDeviceInfo pDeviceInfo);
BOOL setClientInfo(JNIEnv* env, jobject clientInfo, PClientInfo pClientInfo);
BOOL setStreamInfo(JNIEnv* env, jobject streamInfo, PStreamInfo pStreamInfo);
BOOL setFrame(JNIEnv* env, jobject kinesisVideoFrame, PFrame pFrame);
BOOL setFragmentAck(JNIEnv* env, jobject fragmentAck, PFragmentAck pFragmentAck);
BOOL setStreamDescription(JNIEnv* env, jobject streamDescription, PStreamDescription pStreamDesc);
BOOL setStreamingEndpoint(JNIEnv* env, jstring streamingEndpoint, PCHAR pEndpoint);
BOOL setStreamDataBuffer(JNIEnv* env, jobject dataBuffer, UINT32 offset, PBYTE* ppBuffer);
BOOL releaseStreamDataBuffer(JNIEnv* env, jobject dataBuffer, UINT32 offset, PBYTE pBuffer);
BOOL setTags(JNIEnv *env, jobjectArray tagArray, PTag* ppTags, PUINT32 pTagCount);
VOID releaseTags(PTag tags);
#endif // __KINESIS_VIDEO_PARAMETERS_CONVERSION_H__
