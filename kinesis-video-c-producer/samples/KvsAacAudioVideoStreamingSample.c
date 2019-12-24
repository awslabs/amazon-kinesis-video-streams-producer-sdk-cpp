#include <com/amazonaws/kinesis/video/cproducer/Include.h>

#define DEFAULT_RETENTION_PERIOD            2 * HUNDREDS_OF_NANOS_IN_AN_HOUR
#define DEFAULT_BUFFER_DURATION             120 * HUNDREDS_OF_NANOS_IN_A_SECOND
#define DEFAULT_CALLBACK_CHAIN_COUNT        5
#define DEFAULT_KEY_FRAME_INTERVAL          45
#define DEFAULT_FPS_VALUE                   25
#define DEFAULT_STREAM_DURATION             20 * HUNDREDS_OF_NANOS_IN_A_SECOND
#define SAMPLE_AUDIO_FRAME_DURATION         (20 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)
#define SAMPLE_VIDEO_FRAME_DURATION         (HUNDREDS_OF_NANOS_IN_A_SECOND / DEFAULT_FPS_VALUE)

#define NUMBER_OF_H264_FRAME_FILES          403
#define NUMBER_OF_AAC_FRAME_FILES           582

typedef struct {
    volatile ATOMIC_BOOL firstVideoFramePut;
    UINT64 streamStopTime;
    UINT64 streamStartTime;
    STREAM_HANDLE streamHandle;
    CHAR sampleDir[MAX_PATH_LEN + 1];
} SampleCustomData, *PSampleCustomData;

STATUS readFrameData(PFrame pFrame, PCHAR frameFilePath)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 size;

    CHK(pFrame != NULL && frameFilePath != NULL, STATUS_NULL_ARG);

    // Get the size and read into frame
    CHK_STATUS(readFile(frameFilePath, TRUE, NULL, &size));
    CHK_STATUS(readFile(frameFilePath, TRUE, pFrame->frameData, &size));

    pFrame->size = (UINT32) size;

CleanUp:

    return retStatus;
}

PVOID putVideoFrameRoutine(PVOID args)
{
    STATUS retStatus = STATUS_SUCCESS;
    PSampleCustomData data = (PSampleCustomData) args;
    Frame frame;
    UINT32 fileIndex = 0, frameSize;
    CHAR filePath[MAX_PATH_LEN + 1];
    STATUS status;
    BYTE frameBuffer[200000]; // Assuming this is enough
    UINT64 runningTime;

    CHK(data != NULL, STATUS_NULL_ARG);

    frame.frameData = frameBuffer;
    frame.version = FRAME_CURRENT_VERSION;
    frame.trackId = DEFAULT_VIDEO_TRACK_ID;
    frame.duration = 0;
    frame.decodingTs = 0; // relative time mode
    frame.presentationTs = 0;
    frame.index = 0;

    // video track is used to mark new fragment. A new fragment is generated for every frame with FRAME_FLAG_KEY_FRAME
    frame.flags = fileIndex % DEFAULT_KEY_FRAME_INTERVAL == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
    SNPRINTF(filePath, MAX_PATH_LEN, "%s/h264SampleFrames/frame-%03d.h264", data->sampleDir, fileIndex + 1);
    CHK_STATUS(readFrameData(&frame, filePath));

    while (defaultGetTime() < data->streamStopTime) {
        status = putKinesisVideoFrame(data->streamHandle, &frame);
        ATOMIC_STORE_BOOL(&data->firstVideoFramePut, TRUE);
        if (STATUS_FAILED(status)) {
            DLOGD("putKinesisVideoFrame failed with 0x%08x", status);
            status = STATUS_SUCCESS;
        }

        frame.presentationTs += SAMPLE_VIDEO_FRAME_DURATION;
        frame.decodingTs = frame.presentationTs;
        frame.index++;

        fileIndex = (fileIndex + 1) % NUMBER_OF_H264_FRAME_FILES;
        frame.flags = fileIndex % DEFAULT_KEY_FRAME_INTERVAL == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        SNPRINTF(filePath, MAX_PATH_LEN, "%s/h264SampleFrames/frame-%03d.h264", data->sampleDir, fileIndex + 1);
        CHK_STATUS(readFrameData(&frame, filePath));

        // synchronize putKinesisVideoFrame to running time
        runningTime = defaultGetTime() - data->streamStartTime;
        if (runningTime < frame.presentationTs) {
            // reduce sleep time a little for smoother video
            THREAD_SLEEP((frame.presentationTs - runningTime) * 0.9);
        }
    }

CleanUp:

    CHK_LOG_ERR(retStatus);
    return (PVOID) (ULONG_PTR) retStatus;
}

PVOID putAudioFrameRoutine(PVOID args)
{
    STATUS retStatus = STATUS_SUCCESS;
    PSampleCustomData data = (PSampleCustomData) args;
    Frame frame;
    UINT32 fileIndex = 0, frameSize;
    CHAR filePath[MAX_PATH_LEN + 1];
    STATUS status;
    BYTE frameBuffer[200000]; // Assuming this is enough
    UINT64 runningTime;

    CHK(data != NULL, STATUS_NULL_ARG);

    frame.frameData = frameBuffer;
    frame.version = FRAME_CURRENT_VERSION;
    frame.trackId = DEFAULT_AUDIO_TRACK_ID;
    frame.duration = 0;
    frame.decodingTs = 0; // relative time mode
    frame.presentationTs = 0; // relative time mode
    frame.index = 0;
    frame.flags = FRAME_FLAG_NONE; // audio track is not used to cut fragment

    SNPRINTF(filePath, MAX_PATH_LEN, "%s/aacSampleFrames/sample-%03d.aac", data->sampleDir, fileIndex + 1);
    CHK_STATUS(readFrameData(&frame, filePath));

    while (defaultGetTime() < data->streamStopTime) {
        // no audio can be put until first video frame is put
        if (ATOMIC_LOAD_BOOL(&data->firstVideoFramePut)) {
            status = putKinesisVideoFrame(data->streamHandle, &frame);
            if (STATUS_FAILED(status)) {
                DLOGD("putKinesisVideoFrame for audio failed with 0x%08x", status);
                status = STATUS_SUCCESS;
            }

            frame.presentationTs += SAMPLE_AUDIO_FRAME_DURATION;
            frame.decodingTs = frame.presentationTs;
            frame.index++;

            fileIndex = (fileIndex + 1) % NUMBER_OF_AAC_FRAME_FILES;
            SNPRINTF(filePath, MAX_PATH_LEN, "%s/aacSampleFrames/sample-%03d.aac", data->sampleDir, fileIndex + 1);
            CHK_STATUS(readFrameData(&frame, filePath));

            // synchronize putKinesisVideoFrame to running time
            runningTime = defaultGetTime() - data->streamStartTime;
            if (runningTime < frame.presentationTs) {
                THREAD_SLEEP(frame.presentationTs - runningTime);
            }
        }
    }

CleanUp:

    CHK_LOG_ERR(retStatus);
    return (PVOID) (ULONG_PTR) retStatus;
}

INT32 main(INT32 argc, CHAR *argv[])
{
    PDeviceInfo pDeviceInfo = NULL;
    PStreamInfo pStreamInfo = NULL;
    PClientCallbacks pClientCallbacks = NULL;
    CLIENT_HANDLE clientHandle = INVALID_CLIENT_HANDLE_VALUE;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    STATUS retStatus = STATUS_SUCCESS;
    PCHAR accessKey = NULL, secretKey = NULL, sessionToken = NULL, streamName = NULL, region = NULL, cacertPath = NULL;
    UINT64 streamStopTime, streamingDuration = DEFAULT_STREAM_DURATION;
    TID audioSendTid, videoSendTid;
    SampleCustomData data;
    /*
     * user is expected to set audio cpd for their stream.
     * 5 bits (Audio Object Type) | 4 bits (frequency index) | 4 bits (channel configuration) | 3 bits (not used)
     */
    BYTE audioCpd[] = {0x11, 0x90, 0x56, 0xe5, 0x00};

    if (argc < 2) {
        defaultLogPrint(LOG_LEVEL_ERROR, "", "Usage: AWS_ACCESS_KEY_ID=SAMPLEKEY AWS_SECRET_ACCESS_KEY=SAMPLESECRET %s <stream_name> <duration_in_seconds> <frame_files_path>\n", argv[0]);
        CHK(FALSE, STATUS_INVALID_ARG);
    }

    if ((accessKey = getenv(ACCESS_KEY_ENV_VAR)) == NULL || (secretKey = getenv(SECRET_KEY_ENV_VAR)) == NULL) {
        defaultLogPrint(LOG_LEVEL_ERROR, "", "Error missing credentials");
        CHK(FALSE, STATUS_INVALID_ARG);
    }

    MEMSET(data.sampleDir, 0x00, MAX_PATH_LEN + 1);
    if (argc < 4) {
        STRCPY(data.sampleDir, (PCHAR) "../kinesis-video-c-producer/samples");
    } else {
        STRNCPY(data.sampleDir, argv[3], MAX_PATH_LEN);
        if (data.sampleDir[STRLEN(data.sampleDir) - 1] == '/') {
            data.sampleDir[STRLEN(data.sampleDir) - 1] = '\0';
        }
    }

    cacertPath = getenv(CACERT_PATH_ENV_VAR);
    sessionToken = getenv(SESSION_TOKEN_ENV_VAR);
    streamName = argv[1];
    if ((region = getenv(DEFAULT_REGION_ENV_VAR)) == NULL) {
        region = (PCHAR) DEFAULT_AWS_REGION;
    }

    if (argc >= 3) {
        // Get the duration and convert to an integer
        CHK_STATUS(STRTOUI64(argv[2], NULL, 10, &streamingDuration));
        DLOGD("streaming for %u seconds", streamingDuration);
        streamingDuration *= HUNDREDS_OF_NANOS_IN_A_SECOND;
    }

    streamStopTime = defaultGetTime() + streamingDuration;

    // default storage size is 128MB. Use setDeviceInfoStorageSize after create to change storage size.
    CHK_STATUS(createDefaultDeviceInfo(&pDeviceInfo));
    // adjust members of pDeviceInfo here if needed
    pDeviceInfo->clientInfo.loggerLogLevel = LOG_LEVEL_DEBUG;

    CHK_STATUS(createRealtimeAudioVideoStreamInfoProvider(streamName, DEFAULT_RETENTION_PERIOD, DEFAULT_BUFFER_DURATION, &pStreamInfo));

    // adjust members of pStreamInfo here if needed

    // set up audio cpd.
    for(UINT32 i = 0; i < 2; ++i) {
        if (pStreamInfo->streamCaps.trackInfoList[i].trackId == DEFAULT_AUDIO_TRACK_ID) {
            pStreamInfo->streamCaps.trackInfoList[i].codecPrivateData = audioCpd;
            pStreamInfo->streamCaps.trackInfoList[i].codecPrivateDataSize = ARRAY_SIZE(audioCpd);
        }
    }
    // use relative time mode. Buffer timestamps start from 0
    pStreamInfo->streamCaps.absoluteFragmentTimes = FALSE;

    CHK_STATUS(createDefaultCallbacksProviderWithAwsCredentials(accessKey,
                                                                secretKey,
                                                                sessionToken,
                                                                MAX_UINT64,
                                                                region,
                                                                cacertPath,
                                                                NULL,
                                                                NULL,
                                                                FALSE,
                                                                &pClientCallbacks));

    CHK_STATUS(createKinesisVideoClient(pDeviceInfo, pClientCallbacks, &clientHandle));
    CHK_STATUS(createKinesisVideoStreamSync(clientHandle, pStreamInfo, &streamHandle));

    data.streamStopTime = streamStopTime;
    data.streamHandle = streamHandle;
    data.streamStartTime = defaultGetTime();
    ATOMIC_STORE_BOOL(&data.firstVideoFramePut, FALSE);

    THREAD_CREATE(&videoSendTid, putVideoFrameRoutine,
                          (PVOID) &data);
    THREAD_CREATE(&audioSendTid, putAudioFrameRoutine,
                          (PVOID) &data);

    THREAD_JOIN(videoSendTid, NULL);
    THREAD_JOIN(audioSendTid, NULL);

    CHK_STATUS(stopKinesisVideoStreamSync(streamHandle));
    CHK_STATUS(freeKinesisVideoStream(&streamHandle));
    CHK_STATUS(freeKinesisVideoClient(&clientHandle));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        defaultLogPrint(LOG_LEVEL_ERROR, "", "Failed with status 0x%08x\n", retStatus);
    }

    if (pDeviceInfo != NULL) {
        freeDeviceInfo(&pDeviceInfo);
    }

    if (pStreamInfo != NULL) {
        freeStreamInfoProvider(&pStreamInfo);
    }

    if (IS_VALID_STREAM_HANDLE(streamHandle)) {
        freeKinesisVideoStream(&streamHandle);
    }

    if (IS_VALID_CLIENT_HANDLE(clientHandle)) {
        freeKinesisVideoClient(&clientHandle);
    }

    if (pClientCallbacks != NULL) {
        freeCallbacksProvider(&pClientCallbacks);
    }

    return (INT32) retStatus;
}
