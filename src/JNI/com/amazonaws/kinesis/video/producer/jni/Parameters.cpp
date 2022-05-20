/**
 * Implementation of Kinesis Video parameters conversion
 */
#define LOG_CLASS "KinesisVideoParametersConversion"

#include "com/amazonaws/kinesis/video/producer/jni/KinesisVideoClientWrapper.h"

BOOL setDeviceInfo(JNIEnv *env, jobject deviceInfo, PDeviceInfo pDeviceInfo)
{
    STATUS retStatus = STATUS_SUCCESS;
    jmethodID methodId = NULL;
    const char *retChars;

    CHECK(env != NULL && deviceInfo != NULL && pDeviceInfo != NULL);

    // Load DeviceInfo
    jclass cls = env->GetObjectClass(deviceInfo);
    if (cls == NULL) {
        DLOGE("Failed to create DeviceInfo class.");
        CHK(FALSE, STATUS_INVALID_OPERATION);
    }

    // Retrieve the methods and call it
    methodId = env->GetMethodID(cls, "getVersion", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getVersion");
    } else {
        pDeviceInfo->version = env->CallIntMethod(deviceInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getName", "()Ljava/lang/String;");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getName");
    } else {
        jstring retString = (jstring) env->CallObjectMethod(deviceInfo, methodId);
        CHK_JVM_EXCEPTION(env);

        if (retString != NULL) {
            retChars = env->GetStringUTFChars(retString, NULL);
            STRNCPY(pDeviceInfo->name, retChars, MAX_DEVICE_NAME_LEN + 1);

            // Ensure we null terminate it
            pDeviceInfo->name[MAX_DEVICE_NAME_LEN] = '\0';
            env->ReleaseStringUTFChars(retString, retChars);
        } else {
            pDeviceInfo->name[0] = '\0';
        }
    }

    methodId = env->GetMethodID(cls, "getStreamCount", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getStreamCount");
    } else {
        pDeviceInfo->streamCount = env->CallIntMethod(deviceInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getStorageInfoVersion", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getStorageInfoVersion");
    } else {
        pDeviceInfo->storageInfo.version = env->CallIntMethod(deviceInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getDeviceStorageType", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getDeviceStorageType");
    } else {
        pDeviceInfo->storageInfo.storageType = (DEVICE_STORAGE_TYPE) env->CallIntMethod(deviceInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getSpillRatio", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getSpillRatio");
    } else {
        pDeviceInfo->storageInfo.spillRatio = env->CallIntMethod(deviceInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getStorageSize", "()J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getStorageSize");
    } else {
        pDeviceInfo->storageInfo.storageSize = env->CallLongMethod(deviceInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getRootDirectory", "()Ljava/lang/String;");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getRootDirectory");
    } else {
        jstring retString = (jstring) env->CallObjectMethod(deviceInfo, methodId);
        CHK_JVM_EXCEPTION(env);

        if (retString != NULL) {
            retChars = env->GetStringUTFChars(retString, NULL);
            STRNCPY(pDeviceInfo->storageInfo.rootDirectory, retChars, MAX_PATH_LEN + 1);
            pDeviceInfo->storageInfo.rootDirectory[MAX_PATH_LEN] = '\0';
            env->ReleaseStringUTFChars(retString, retChars);
        } else {
            pDeviceInfo->storageInfo.rootDirectory[0] = '\0';
        }
    }

    // Set the tags to empty first
    pDeviceInfo->tagCount = 0;
    pDeviceInfo->tags = NULL;
    methodId = env->GetMethodID(cls, "getTags", "()[Lcom/amazonaws/kinesisvideo/producer/Tag;");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getTags");
    } else {
        jobjectArray array = (jobjectArray) env->CallObjectMethod(deviceInfo, methodId);
        CHK_JVM_EXCEPTION(env);

        if (!setTags(env, array, &pDeviceInfo->tags, &pDeviceInfo->tagCount)) {
            DLOGW("Failed getting/setting tags.");
        }
    }

    methodId = env->GetMethodID(cls, "getClientId", "()Ljava/lang/String;");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getClientId");
    } else {
        jstring retString = (jstring) env->CallObjectMethod(deviceInfo, methodId);
        CHK_JVM_EXCEPTION(env);

        if (retString != NULL) {
            retChars = env->GetStringUTFChars(retString, NULL);
            STRNCPY(pDeviceInfo->clientId, retChars, MAX_CLIENT_ID_STRING_LENGTH + 1);
            pDeviceInfo->clientId[MAX_CLIENT_ID_STRING_LENGTH] = '\0';
            env->ReleaseStringUTFChars(retString, retChars);
        } else {
            STRNCPY(pDeviceInfo->clientId, (PCHAR) "JNI", MAX_CLIENT_ID_STRING_LENGTH + 1);
        }
    }

    // Set the client info to empty first
    methodId = env->GetMethodID(cls, "getClientInfo", "()Lcom/amazonaws/kinesisvideo/producer/ClientInfo;");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getClientInfo");
    } else {
        jobject clientInfo = (jobject) env->CallObjectMethod(deviceInfo, methodId);
        CHK_JVM_EXCEPTION(env);

        if (!setClientInfo(env, clientInfo, &pDeviceInfo->clientInfo)) {
            DLOGW("Failed getting/setting client info.");
        }
    }

CleanUp:
    return STATUS_FAILED(retStatus) ? FALSE : TRUE;
}

BOOL setClientInfo(JNIEnv *env, jobject clientInfo, PClientInfo pClientInfo) {
    STATUS retStatus = STATUS_SUCCESS;
    jmethodID methodId = NULL;
    const char *retChars;

    CHECK(env != NULL && clientInfo != NULL && pClientInfo != NULL);

    // Load ClientInfo
    jclass cls = env->GetObjectClass(clientInfo);
    if (cls == NULL) {
        DLOGE("Failed to create ClientInfo class.");
        CHK(FALSE, STATUS_INVALID_OPERATION);
    }

    // Retrieve the methods and call it
    methodId = env->GetMethodID(cls, "getVersion", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getVersion");
    } else {
        pClientInfo->version = env->CallIntMethod(clientInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getCreateClientTimeout", "()J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getCreateClientTimeout");
    } else {
        pClientInfo->createClientTimeout = env->CallLongMethod(clientInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getCreateStreamTimeout", "()J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getCreateStreamTimeout");
    } else {
        pClientInfo->createStreamTimeout = env->CallLongMethod(clientInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getStopStreamTimeout", "()J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getStopStreamTimeout");
    } else {
        pClientInfo->stopStreamTimeout = env->CallLongMethod(clientInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getOfflineBufferAvailabilityTimeout", "()J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getOfflineBufferAvailabilityTimeout");
    } else {
        pClientInfo->offlineBufferAvailabilityTimeout = env->CallLongMethod(clientInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getLoggerLogLevel", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getLoggerLogLevel");
    } else {
        pClientInfo->loggerLogLevel = env->CallIntMethod(clientInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getLogMetric", "()Z");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getLogMetric");
    } else {
        pClientInfo->logMetric = env->CallBooleanMethod(clientInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getAutomaticStreamingFlags", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getAutomaticStreamingFlags");
    } else {
        pClientInfo->automaticStreamingFlags = (AUTOMATIC_STREAMING_FLAGS) env->CallIntMethod(clientInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

CleanUp:
    return STATUS_FAILED(retStatus) ? FALSE : TRUE;
}


BOOL setTags(JNIEnv *env, jobjectArray tagArray, PTag* ppTags, PUINT32 pTagCount)
{
    STATUS retStatus = STATUS_SUCCESS;
    const char *retChars;
    jclass tagClass = NULL;
    jobject tagObj = NULL;
    jstring retString = NULL;
    jmethodID nameMethodId = NULL;
    jmethodID valueMethodId = NULL;
    PTag pTags = NULL;
    BOOL retValue = TRUE;
    UINT32 tagCount = 0, i = 0;
    PCHAR pCurPtr = NULL;

    // Early exit in case of an optional array of tags
    CHK(tagArray != NULL, STATUS_SUCCESS);

    // Get the length of the array
    tagCount = (UINT32) env->GetArrayLength(tagArray);
    CHK_JVM_EXCEPTION(env);

    // Allocate enough memory.
    // NOTE: We need to add two NULL terminators for tag name and tag value
    CHK(NULL != (pTags = (PTag) MEMCALLOC(tagCount, SIZEOF(Tag) + (MAX_TAG_NAME_LEN +  MAX_TAG_VALUE_LEN + 2) * SIZEOF(CHAR))), STATUS_NOT_ENOUGH_MEMORY);

    // Iterate over and set the values. NOTE: the actual storage for the strings will follow the array
    pCurPtr = (PCHAR) (pTags + tagCount);
    for (;i < tagCount; i++) {
        CHK(NULL != (tagObj = env->GetObjectArrayElement(tagArray, (jsize) i)), STATUS_INVALID_ARG);
        CHK_JVM_EXCEPTION(env);

        // Get the Tag class object and the method IDs first time
        if (tagClass == NULL) {
            CHK(NULL != (tagClass = env->GetObjectClass(tagObj)), STATUS_INVALID_OPERATION);
            CHK_JVM_EXCEPTION(env);

            CHK(NULL != (nameMethodId = env->GetMethodID(tagClass, "getName", "()Ljava/lang/String;")), STATUS_INVALID_OPERATION);
            CHK_JVM_EXCEPTION(env);

            CHK(NULL != (valueMethodId = env->GetMethodID(tagClass, "getValue", "()Ljava/lang/String;")), STATUS_INVALID_OPERATION);
            CHK_JVM_EXCEPTION(env);
        }

        // Get the name of the tag
        CHK(NULL != (retString = (jstring) env->CallObjectMethod(tagObj, nameMethodId)), STATUS_INVALID_ARG);
        CHK_JVM_EXCEPTION(env);

        // Extract the chars and copy
        retChars = env->GetStringUTFChars(retString, NULL);
        STRNCPY(pCurPtr, retChars, MAX_TAG_NAME_LEN + 1);
        pCurPtr[MAX_TAG_NAME_LEN] = '\0';
        env->ReleaseStringUTFChars(retString, retChars);

        // Set the tag pointer and increment the current pointer
        pTags[i].name = pCurPtr;
        pCurPtr += MAX_TAG_NAME_LEN;

        // Get the value of the tag
        CHK(NULL != (retString = (jstring) env->CallObjectMethod(tagObj, valueMethodId)), STATUS_INVALID_ARG);
        CHK_JVM_EXCEPTION(env);

        // Extract the chars and copy
        retChars = env->GetStringUTFChars(retString, NULL);
        STRNCPY(pCurPtr, retChars, MAX_TAG_VALUE_LEN + 1);
        pCurPtr[MAX_TAG_VALUE_LEN] = '\0';
        env->ReleaseStringUTFChars(retString, retChars);

        // Set the tag pointer and increment the current pointer
        pTags[i].value = pCurPtr;
        pCurPtr += MAX_TAG_VALUE_LEN;
    }

    // Set the tag count and the tags
    *pTagCount = tagCount;
    *ppTags = pTags;

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        retValue = FALSE;
        releaseTags(pTags);
    }

    return retValue;
}

VOID releaseTags(PTag tags)
{
    if (tags != NULL) {
        MEMFREE(tags);
    }
}

BOOL setStreamInfo(JNIEnv* env, jobject streamInfo, PStreamInfo pStreamInfo)
{
    STATUS retStatus = STATUS_SUCCESS;
    jmethodID methodId = NULL;
    jbyteArray byteArray = NULL;
    jbyte* bufferPtr = NULL;
    jsize arrayLen = 0;
    UINT32 trackInfoCount = 0;
    const char *retChars;

    CHECK(env != NULL && streamInfo != NULL && pStreamInfo != NULL);

    // Load StreamInfo
    jclass cls = env->GetObjectClass(streamInfo);
    if (cls == NULL) {
        DLOGE("Failed to create StreamInfo class.");
        CHK(FALSE, STATUS_INVALID_OPERATION);
    }

    // Retrieve the methods and call it
    methodId = env->GetMethodID(cls, "getVersion", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getVersion");
    } else {
        pStreamInfo->version = env->CallIntMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getName", "()Ljava/lang/String;");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getName");
    } else {
        jstring retString = (jstring) env->CallObjectMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);

        if (retString != NULL) {
            retChars = env->GetStringUTFChars(retString, NULL);
            STRNCPY(pStreamInfo->name, retChars, MAX_STREAM_NAME_LEN + 1);

            // Just in case - null terminate the copied string
            pStreamInfo->name[MAX_STREAM_NAME_LEN] = '\0';

            env->ReleaseStringUTFChars(retString, retChars);
        } else {
            pStreamInfo->name[0] = '\0';
        }
    }

    methodId = env->GetMethodID(cls, "getStreamingType", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getStreamingType");
    } else {
        pStreamInfo->streamCaps.streamingType = (STREAMING_TYPE) env->CallIntMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getContentType", "()Ljava/lang/String;");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getContentType");
    } else {
        jstring retString = (jstring) env->CallObjectMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);

        if (retString != NULL) {
            retChars = env->GetStringUTFChars(retString, NULL);
            STRNCPY(pStreamInfo->streamCaps.contentType, retChars, MAX_CONTENT_TYPE_LEN + 1);
            pStreamInfo->streamCaps.contentType[MAX_CONTENT_TYPE_LEN] = '\0';
            env->ReleaseStringUTFChars(retString, retChars);
        } else {
            pStreamInfo->streamCaps.contentType[0] = '\0';
        }
    }

    methodId = env->GetMethodID(cls, "getKmsKeyId", "()Ljava/lang/String;");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getKmsKeyId");
    } else {
        jstring retString = (jstring) env->CallObjectMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);

        if (retString != NULL) {
            retChars = env->GetStringUTFChars(retString, NULL);
            STRNCPY(pStreamInfo->kmsKeyId, retChars, MAX_ARN_LEN + 1);
            pStreamInfo->kmsKeyId[MAX_ARN_LEN] = '\0';
            env->ReleaseStringUTFChars(retString, retChars);
        } else {
            pStreamInfo->kmsKeyId[0] = '\0';
        }
    }

    methodId = env->GetMethodID(cls, "getRetentionPeriod", "()J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getRetentionPeriod");
    } else {
        pStreamInfo->retention = env->CallLongMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "isAdaptive", "()Z");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id isAdaptive");
    } else {
        pStreamInfo->streamCaps.adaptive = env->CallBooleanMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getMaxLatency", "()J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getMaxLatency");
    } else {
        pStreamInfo->streamCaps.maxLatency = env->CallLongMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getFragmentDuration", "()J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getFragmentDuration");
    } else {
        pStreamInfo->streamCaps.fragmentDuration = env->CallLongMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "isKeyFrameFragmentation", "()Z");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id isKeyFrameFragmentation");
    } else {
        pStreamInfo->streamCaps.keyFrameFragmentation = env->CallBooleanMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "isFrameTimecodes", "()Z");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id isFrameTimecodes");
    } else {
        pStreamInfo->streamCaps.frameTimecodes = env->CallBooleanMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "isAbsoluteFragmentTimes", "()Z");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id isAbsoluteFragmentTimes");
    } else {
        pStreamInfo->streamCaps.absoluteFragmentTimes = env->CallBooleanMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "isFragmentAcks", "()Z");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id isFragmentAcks");
    } else {
        pStreamInfo->streamCaps.fragmentAcks = env->CallBooleanMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "isRecoverOnError", "()Z");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id isRecoverOnError");
    } else {
        pStreamInfo->streamCaps.recoverOnError = env->CallBooleanMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "isRecalculateMetrics", "()Z");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id isRecalculateMetrics");
    } else {
        pStreamInfo->streamCaps.recalculateMetrics = env->CallBooleanMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getNalAdaptationFlags", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getNalAdaptationFlags");
    } else {
        pStreamInfo->streamCaps.nalAdaptationFlags = env->CallIntMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    // Initialize to NULL first in case of failures
    pStreamInfo->streamCaps.segmentUuid = NULL;
    methodId = env->GetMethodID(cls, "getSegmentUuidBytes", "()[B");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getSegmentUuidBytes");
    } else {
        byteArray = (jbyteArray) env->CallObjectMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);

        if (byteArray != NULL) {
            // Extract the bits from the byte buffer
            bufferPtr = env->GetByteArrayElements(byteArray, NULL);
            arrayLen = env->GetArrayLength(byteArray);

            // Ensure we have at least UUID size
            CHK(arrayLen == MKV_SEGMENT_UUID_LEN, STATUS_INVALID_ARG_LEN);

            // Allocate a temp storage
            pStreamInfo->streamCaps.segmentUuid = (PBYTE) MEMALLOC(MKV_SEGMENT_UUID_LEN);
            CHK(pStreamInfo->streamCaps.segmentUuid != NULL, STATUS_NOT_ENOUGH_MEMORY);

            // Copy the bits
            MEMCPY(pStreamInfo->streamCaps.segmentUuid, bufferPtr, MKV_SEGMENT_UUID_LEN);

            // Release the buffer
            env->ReleaseByteArrayElements(byteArray, bufferPtr, JNI_ABORT);
        }
    }

    methodId = env->GetMethodID(cls, "getTrackInfoCount", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getTrackInfoCount");
    } else {
        trackInfoCount = (UINT32) env->CallIntMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    CHECK_EXT(trackInfoCount > 0, "TrackInfo count should be greater than 0");
    pStreamInfo->streamCaps.trackInfoCount = trackInfoCount;
    pStreamInfo->streamCaps.trackInfoList = (PTrackInfo) MEMALLOC(trackInfoCount * SIZEOF(TrackInfo));
    MEMSET(pStreamInfo->streamCaps.trackInfoList, 0, SIZEOF(TrackInfo) * trackInfoCount);
    CHK(pStreamInfo->streamCaps.trackInfoList != NULL, STATUS_NOT_ENOUGH_MEMORY);

    methodId = env->GetMethodID(cls, "getTrackInfoVersion", "(I)I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getTrackInfoVersion");
    } else {
        for(UINT32 i = 0; i < trackInfoCount; ++i) {
            pStreamInfo->streamCaps.trackInfoList[i].version = (UINT64) env->CallIntMethod(streamInfo, methodId, i);
            CHK_JVM_EXCEPTION(env);
        }
    }

    methodId = env->GetMethodID(cls, "getTrackName", "(I)Ljava/lang/String;");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getTrackName");
    } else {
        for(UINT32 i = 0; i < trackInfoCount; ++i) {
            jstring retString = (jstring) env->CallObjectMethod(streamInfo, methodId, i);
            CHK_JVM_EXCEPTION(env);

            if (retString != NULL) {
                retChars = env->GetStringUTFChars(retString, NULL);
                STRNCPY(pStreamInfo->streamCaps.trackInfoList[i].trackName, retChars, MKV_MAX_TRACK_NAME_LEN + 1);
                pStreamInfo->streamCaps.trackInfoList[i].trackName[MKV_MAX_TRACK_NAME_LEN] = '\0';
                env->ReleaseStringUTFChars(retString, retChars);
            } else {
                pStreamInfo->streamCaps.trackInfoList[i].trackName[0] = '\0';
            }
        }
    }

    methodId = env->GetMethodID(cls, "getCodecId", "(I)Ljava/lang/String;");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getCodecId");
    } else {
        for(UINT32 i = 0; i < trackInfoCount; ++i) {
            jstring retString = (jstring) env->CallObjectMethod(streamInfo, methodId, i);
            CHK_JVM_EXCEPTION(env);

            if (retString != NULL) {
                retChars = env->GetStringUTFChars(retString, NULL);
                STRNCPY(pStreamInfo->streamCaps.trackInfoList[i].codecId, retChars, MKV_MAX_CODEC_ID_LEN + 1);
                pStreamInfo->streamCaps.trackInfoList[i].codecId[MKV_MAX_CODEC_ID_LEN] = '\0';
                env->ReleaseStringUTFChars(retString, retChars);
            } else {
                pStreamInfo->streamCaps.trackInfoList[i].codecId[0] = '\0';
            }
        }
    }

    methodId = env->GetMethodID(cls, "getCodecPrivateData", "(I)[B");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getCodecPrivateData");
    } else {
        for(UINT32 i = 0; i < trackInfoCount; ++i) {
            byteArray = (jbyteArray) env->CallObjectMethod(streamInfo, methodId, i);
            CHK_JVM_EXCEPTION(env);

            if (byteArray != NULL) {
                // Extract the bits from the byte buffer
                bufferPtr = env->GetByteArrayElements(byteArray, NULL);
                arrayLen = env->GetArrayLength(byteArray);

                // Allocate a temp storage
                pStreamInfo->streamCaps.trackInfoList[i].codecPrivateDataSize = (UINT32) arrayLen;
                pStreamInfo->streamCaps.trackInfoList[i].codecPrivateData = (PBYTE) MEMALLOC(arrayLen);
                CHK(pStreamInfo->streamCaps.trackInfoList[i].codecPrivateData != NULL, STATUS_NOT_ENOUGH_MEMORY);

                // Copy the bits
                MEMCPY(pStreamInfo->streamCaps.trackInfoList[i].codecPrivateData, bufferPtr, arrayLen);

                // Release the buffer
                env->ReleaseByteArrayElements(byteArray, bufferPtr, JNI_ABORT);
            } else {
                pStreamInfo->streamCaps.trackInfoList[i].codecPrivateDataSize = 0;
                pStreamInfo->streamCaps.trackInfoList[i].codecPrivateData = NULL;
            }
        }
    }

    methodId = env->GetMethodID(cls, "getTrackInfoType", "(I)I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getTrackInfoType");
    } else {
        for(UINT32 i = 0; i < trackInfoCount; ++i) {
            pStreamInfo->streamCaps.trackInfoList[i].trackType = (MKV_TRACK_INFO_TYPE) env->CallIntMethod(streamInfo, methodId, i);
            CHK_JVM_EXCEPTION(env);
        }
    }

    methodId = env->GetMethodID(cls, "getTrackId", "(I)J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getTrackId");
    } else {
        for(UINT32 i = 0; i < trackInfoCount; ++i) {
            pStreamInfo->streamCaps.trackInfoList[i].trackId = (UINT64) env->CallLongMethod(streamInfo, methodId, i);
            CHK_JVM_EXCEPTION(env);
        }
    }

    methodId = env->GetMethodID(cls, "getAvgBandwidthBps", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getAvgBandwidthBps");
    } else {
        pStreamInfo->streamCaps.avgBandwidthBps = env->CallIntMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getFrameRate", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getFrameRate");
    } else {
        pStreamInfo->streamCaps.frameRate = env->CallIntMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getBufferDuration", "()J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getBufferDuration");
    } else {
        pStreamInfo->streamCaps.bufferDuration = env->CallLongMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getReplayDuration", "()J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getReplayDuration");
    } else {
        pStreamInfo->streamCaps.replayDuration = env->CallLongMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getConnectionStalenessDuration", "()J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getConnectionStalenessDuration");
    } else {
        pStreamInfo->streamCaps.connectionStalenessDuration = env->CallLongMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getTimecodeScale", "()J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getTimecodeScale");
    } else {
        pStreamInfo->streamCaps.timecodeScale = env->CallLongMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    // Set the tags to empty first
    pStreamInfo->tagCount = 0;
    pStreamInfo->tags = NULL;
    methodId = env->GetMethodID(cls, "getTags", "()[Lcom/amazonaws/kinesisvideo/producer/Tag;");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getTags");
    } else {
        jobjectArray array = (jobjectArray) env->CallObjectMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);

        if (!setTags(env, array, &pStreamInfo->tags, &pStreamInfo->tagCount)) {
            DLOGW("Failed getting/setting tags.");
        }
    }

    methodId = env->GetMethodID(cls, "getFrameOrderMode", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getFrameOrderMode");
    } else {
        pStreamInfo->streamCaps.frameOrderingMode = (FRAME_ORDER_MODE) env->CallIntMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getStorePressurePolicy", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getStorePressurePolicy");
    } else {
        pStreamInfo->streamCaps.storePressurePolicy = (CONTENT_STORE_PRESSURE_POLICY) env->CallIntMethod(streamInfo, methodId);
        CHK_JVM_EXCEPTION(env);
    }

CleanUp:
    return STATUS_FAILED(retStatus) ? FALSE : TRUE;
}

BOOL setFrame(JNIEnv* env, jobject kinesisVideoFrame, PFrame pFrame)
{
    STATUS retStatus = STATUS_SUCCESS;
    jmethodID methodId = NULL;
    CHECK(env != NULL && kinesisVideoFrame != NULL && pFrame != NULL);

    // Load KinesisVideoFrame
    jclass cls = env->GetObjectClass(kinesisVideoFrame);
    if (cls == NULL) {
        DLOGE("Failed to create KinesisVideoFrame class.");
        CHK(FALSE, STATUS_INVALID_OPERATION);
    }

    // Retrieve the methods and call it
    methodId = env->GetMethodID(cls, "getVersion", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getVersion");
    } else {
        pFrame->version = env->CallIntMethod(kinesisVideoFrame, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getIndex", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getIndex");
    } else {
        pFrame->index = env->CallIntMethod(kinesisVideoFrame, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getTrackId", "()J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getTrackId");
    } else {
        pFrame->trackId = env->CallLongMethod(kinesisVideoFrame, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getFlags", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getFlags");
    } else {
        pFrame->flags = (FRAME_FLAGS) env->CallIntMethod(kinesisVideoFrame, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getDecodingTs", "()J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getDecodingTs");
    } else {
        pFrame->decodingTs = env->CallLongMethod(kinesisVideoFrame, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getPresentationTs", "()J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getPresentationTs");
    } else {
        pFrame->presentationTs = env->CallLongMethod(kinesisVideoFrame, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getDuration", "()J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getDuration");
    } else {
        pFrame->duration = env->CallLongMethod(kinesisVideoFrame, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getSize", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getSize");
    } else {
        pFrame->size = (FRAME_FLAGS) env->CallIntMethod(kinesisVideoFrame, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getData", "()Ljava/nio/ByteBuffer;");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getData");
    } else {
        jobject byteBuffer = (jstring) env->CallObjectMethod(kinesisVideoFrame, methodId);
        CHK_JVM_EXCEPTION(env);
        pFrame->frameData = (PBYTE) env->GetDirectBufferAddress(byteBuffer);
    }

CleanUp:
    return STATUS_FAILED(retStatus) ? FALSE : TRUE;
}

BOOL setFragmentAck(JNIEnv* env, jobject fragmentAck, PFragmentAck pFragmentAck)
{
    STATUS retStatus = STATUS_SUCCESS;
    jmethodID methodId = NULL;
    CHECK(env != NULL && fragmentAck != NULL && pFragmentAck != NULL);

    // Load KinesisVideoFragmentAck
    jclass cls = env->GetObjectClass(fragmentAck);
    if (cls == NULL) {
        DLOGE("Failed to create KinesisVideoFragmentAck class.");
        CHK(FALSE, STATUS_INVALID_OPERATION);
    }

    // Retrieve the methods and call it
    methodId = env->GetMethodID(cls, "getVersion", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getVersion");
    } else {
        pFragmentAck->version = env->CallIntMethod(fragmentAck, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getAckType", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getAckType");
    } else {
        pFragmentAck->ackType = (FRAGMENT_ACK_TYPE) env->CallIntMethod(fragmentAck, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getTimestamp", "()J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getTimestamp");
    } else {
        pFragmentAck->timestamp = env->CallLongMethod(fragmentAck, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getSequenceNumber", "()Ljava/lang/String;");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getSequenceNumber");
    } else {
        jstring retString = (jstring) env->CallObjectMethod(fragmentAck, methodId);
        CHK_JVM_EXCEPTION(env);

        if (retString != NULL) {
            const char* retChars = env->GetStringUTFChars(retString, NULL);
            STRNCPY(pFragmentAck->sequenceNumber, retChars, MAX_FRAGMENT_SEQUENCE_NUMBER + 1);
            pFragmentAck->sequenceNumber[MAX_FRAGMENT_SEQUENCE_NUMBER] = '\0';
            env->ReleaseStringUTFChars(retString, retChars);
        } else {
            pFragmentAck->sequenceNumber[0] = '\0';
        }
    }

    methodId = env->GetMethodID(cls, "getResult", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getResult");
    } else {
        pFragmentAck->result = (SERVICE_CALL_RESULT) env->CallIntMethod(fragmentAck, methodId);
        CHK_JVM_EXCEPTION(env);
    }

CleanUp:
    return STATUS_FAILED(retStatus) ? FALSE : TRUE;
}

BOOL setStreamDescription(JNIEnv* env, jobject streamDescription, PStreamDescription pStreamDesc)
{
    STATUS retStatus = STATUS_SUCCESS;
    jmethodID methodId = NULL;
    const char *retChars;

    CHECK(env != NULL && streamDescription != NULL && pStreamDesc != NULL);

    // Load StreamDescription
    jclass cls = env->GetObjectClass(streamDescription);
    if (cls == NULL) {
        DLOGE("Failed to create StreamDescription class.");
        CHK(FALSE, STATUS_INVALID_OPERATION);
    }

    // Retrieve the methods and call it
    methodId = env->GetMethodID(cls, "getVersion", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getVersion");
    } else {
        pStreamDesc->version = env->CallIntMethod(streamDescription, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getDeviceName", "()Ljava/lang/String;");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getDeviceName");
    } else {
        jstring retString = (jstring) env->CallObjectMethod(streamDescription, methodId);
        CHK_JVM_EXCEPTION(env);

        if (retString != NULL) {
            retChars = env->GetStringUTFChars(retString, NULL);
            STRNCPY(pStreamDesc->deviceName, retChars, MAX_DEVICE_NAME_LEN + 1);
            pStreamDesc->deviceName[MAX_DEVICE_NAME_LEN] = '\0';
            env->ReleaseStringUTFChars(retString, retChars);
        } else {
            pStreamDesc->deviceName[0] = '\0';
        }
    }

    methodId = env->GetMethodID(cls, "getStreamName", "()Ljava/lang/String;");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getStreamName");
    } else {
        jstring retString = (jstring) env->CallObjectMethod(streamDescription, methodId);
        CHK_JVM_EXCEPTION(env);

        if (retString != NULL) {
            retChars = env->GetStringUTFChars(retString, NULL);
            STRNCPY(pStreamDesc->streamName, retChars, MAX_STREAM_NAME_LEN + 1);
            pStreamDesc->streamName[MAX_STREAM_NAME_LEN] = '\0';
            env->ReleaseStringUTFChars(retString, retChars);
        } else {
            pStreamDesc->streamName[0] = '\0';
        }
    }

    methodId = env->GetMethodID(cls, "getContentType", "()Ljava/lang/String;");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getContentType");
    } else {
        jstring retString = (jstring) env->CallObjectMethod(streamDescription, methodId);
        CHK_JVM_EXCEPTION(env);

        if (retString != NULL) {
            retChars = env->GetStringUTFChars(retString, NULL);
            STRNCPY(pStreamDesc->contentType, retChars, MAX_CONTENT_TYPE_LEN + 1);
            pStreamDesc->contentType[MAX_CONTENT_TYPE_LEN] = '\0';
            env->ReleaseStringUTFChars(retString, retChars);
        } else {
            pStreamDesc->contentType[0] = '\0';
        }
    }

    methodId = env->GetMethodID(cls, "getUpdateVersion", "()Ljava/lang/String;");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getUpdateVersion");
    } else {
        jstring retString = (jstring) env->CallObjectMethod(streamDescription, methodId);
        CHK_JVM_EXCEPTION(env);

        if (retString != NULL) {
            retChars = env->GetStringUTFChars(retString, NULL);
            STRNCPY(pStreamDesc->updateVersion, retChars, MAX_UPDATE_VERSION_LEN + 1);
            pStreamDesc->updateVersion[MAX_UPDATE_VERSION_LEN] = '\0';
            env->ReleaseStringUTFChars(retString, retChars);
        } else {
            pStreamDesc->updateVersion[0] = '\0';
        }
    }

    methodId = env->GetMethodID(cls, "getStreamArn", "()Ljava/lang/String;");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getStreamArn");
    } else {
        jstring retString = (jstring) env->CallObjectMethod(streamDescription, methodId);
        CHK_JVM_EXCEPTION(env);

        if (retString != NULL) {
            retChars = env->GetStringUTFChars(retString, NULL);
            STRNCPY(pStreamDesc->streamArn, retChars, MAX_ARN_LEN + 1);
            pStreamDesc->streamArn[MAX_ARN_LEN] = '\0';
            env->ReleaseStringUTFChars(retString, retChars);
        } else {
            pStreamDesc->streamArn[0] = '\0';
        }
    }

    methodId = env->GetMethodID(cls, "getCreationTime", "()J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getCreationTime");
    } else {
        pStreamDesc->creationTime = env->CallLongMethod(streamDescription, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getStreamStatus", "()I");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getStreamStatus");
    } else {
        pStreamDesc->streamStatus = (STREAM_STATUS) env->CallIntMethod(streamDescription, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getRetention", "()J");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getRetention");
    } else {
        pStreamDesc->retention = env->CallLongMethod(streamDescription, methodId);
        CHK_JVM_EXCEPTION(env);
    }

    methodId = env->GetMethodID(cls, "getKmsKeyId", "()Ljava/lang/String;");
    if (methodId == NULL) {
        DLOGW("Couldn't find method id getKmsKeyId");
    } else {
        jstring retString = (jstring) env->CallObjectMethod(streamDescription, methodId);
        CHK_JVM_EXCEPTION(env);

        if (retString != NULL) {
            retChars = env->GetStringUTFChars(retString, NULL);
            STRNCPY(pStreamDesc->kmsKeyId, retChars, MAX_ARN_LEN + 1);
            pStreamDesc->kmsKeyId[MAX_ARN_LEN] = '\0';
            env->ReleaseStringUTFChars(retString, retChars);
        } else {
            pStreamDesc->kmsKeyId[0] = '\0';
        }
    }

CleanUp:
    return STATUS_FAILED(retStatus) ? FALSE : TRUE;
}

BOOL setStreamingEndpoint(JNIEnv* env, jstring streamingEndpoint, PCHAR pEndpoint)
{
    CHECK(env != NULL && streamingEndpoint != NULL && pEndpoint != NULL);

    const char *endpointChars = env->GetStringUTFChars(streamingEndpoint, NULL);
    STRNCPY(pEndpoint, endpointChars, MAX_URI_CHAR_LEN + 1);
    pEndpoint[MAX_URI_CHAR_LEN] = '\0';
    env->ReleaseStringUTFChars(streamingEndpoint, endpointChars);

    return TRUE;
}

BOOL setStreamDataBuffer(JNIEnv* env, jobject dataBuffer, UINT32 offset, PBYTE* ppBuffer)
{
    PBYTE pBuffer = NULL;

    CHECK(env != NULL && ppBuffer != NULL);

    if (dataBuffer == NULL) {
        return FALSE;
    }

    // Get the byte pointer from the GC byte[] by pinning or copying.
    pBuffer = (PBYTE) env->GetByteArrayElements((jbyteArray) dataBuffer, NULL);

    if (pBuffer == NULL) {
        return FALSE;
    }

    *ppBuffer = pBuffer + offset;

    return TRUE;
}

BOOL releaseStreamDataBuffer(JNIEnv* env, jobject dataBuffer, UINT32 offset, PBYTE pBuffer)
{
    CHECK(env != NULL);

    if (dataBuffer == NULL || pBuffer == NULL) {
        return TRUE;
    }

    // Release/commit the changes and free.
    env->ReleaseByteArrayElements((jbyteArray) dataBuffer, (jbyte*) (pBuffer - offset), 0);

    return TRUE;
}
