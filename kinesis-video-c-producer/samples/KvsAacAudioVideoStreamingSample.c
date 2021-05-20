#include <com/amazonaws/kinesis/video/cproducer/Include.h>

#define DEFAULT_RETENTION_PERIOD     2 * HUNDREDS_OF_NANOS_IN_AN_HOUR
#define DEFAULT_BUFFER_DURATION      120 * HUNDREDS_OF_NANOS_IN_A_SECOND
#define DEFAULT_CALLBACK_CHAIN_COUNT 5
#define DEFAULT_KEY_FRAME_INTERVAL   45
#define DEFAULT_FPS_VALUE            25
#define DEFAULT_STREAM_DURATION      20 * HUNDREDS_OF_NANOS_IN_A_SECOND
#define SAMPLE_AUDIO_FRAME_DURATION  (20 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)
#define SAMPLE_VIDEO_FRAME_DURATION  (HUNDREDS_OF_NANOS_IN_A_SECOND / DEFAULT_FPS_VALUE)
#define AUDIO_TRACK_SAMPLING_RATE    48000
#define AUDIO_TRACK_CHANNEL_CONFIG   2

#define NUMBER_OF_H264_FRAME_FILES 403
#define NUMBER_OF_AAC_FRAME_FILES  582

#define FILE_LOGGING_BUFFER_SIZE (100 * 1024)
#define MAX_NUMBER_OF_LOG_FILES  5

typedef struct {
    PBYTE buffer;
    UINT32 size;
} FrameData, *PFrameData;

typedef struct {
    volatile ATOMIC_BOOL firstVideoFramePut;
    UINT64 streamStopTime;
    UINT64 streamStartTime;
    STREAM_HANDLE streamHandle;
    CHAR sampleDir[MAX_PATH_LEN + 1];
    FrameData audioFrames[NUMBER_OF_AAC_FRAME_FILES];
    FrameData videoFrames[NUMBER_OF_H264_FRAME_FILES];
    BOOL firstFrame;
    UINT64 startTime;
} SampleCustomData, *PSampleCustomData;

PVOID putVideoFrameRoutine(PVOID args)
{
    STATUS retStatus = STATUS_SUCCESS;
    PSampleCustomData data = (PSampleCustomData) args;
    Frame frame;
    UINT32 fileIndex = 0;
    STATUS status;
    UINT64 runningTime;
    DOUBLE startUpLatency;

    CHK(data != NULL, STATUS_NULL_ARG);

    frame.frameData = data->videoFrames[fileIndex].buffer;
    frame.size = data->videoFrames[fileIndex].size;
    frame.version = FRAME_CURRENT_VERSION;
    frame.trackId = DEFAULT_VIDEO_TRACK_ID;
    frame.duration = 0;
    frame.decodingTs = 0;
    frame.presentationTs = 0;
    frame.index = 0;

    // video track is used to mark new fragment. A new fragment is generated for every frame with FRAME_FLAG_KEY_FRAME
    frame.flags = fileIndex % DEFAULT_KEY_FRAME_INTERVAL == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

    while (defaultGetTime() < data->streamStopTime) {
        status = putKinesisVideoFrame(data->streamHandle, &frame);
        if (data->firstFrame) {
            startUpLatency = (DOUBLE)(GETTIME() - data->startTime) / (DOUBLE) HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
            DLOGD("Start up latency: %lf ms", startUpLatency);
            data->firstFrame = FALSE;
        }

        ATOMIC_STORE_BOOL(&data->firstVideoFramePut, TRUE);
        if (STATUS_FAILED(status)) {
            printf("putKinesisVideoFrame failed with 0x%08x\n", status);
            status = STATUS_SUCCESS;
        }

        frame.presentationTs += SAMPLE_VIDEO_FRAME_DURATION;
        frame.decodingTs = frame.presentationTs;
        frame.index++;

        fileIndex = (fileIndex + 1) % NUMBER_OF_H264_FRAME_FILES;
        frame.flags = fileIndex % DEFAULT_KEY_FRAME_INTERVAL == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        frame.frameData = data->videoFrames[fileIndex].buffer;
        frame.size = data->videoFrames[fileIndex].size;

        // synchronize putKinesisVideoFrame to running time
        runningTime = defaultGetTime() - data->streamStartTime;
        if (runningTime < frame.presentationTs) {
            // reduce sleep time a little for smoother video
            THREAD_SLEEP((frame.presentationTs - runningTime) * 0.9);
        }
    }

CleanUp:

    if (retStatus != STATUS_SUCCESS) {
        printf("putVideoFrameRoutine failed with 0x%08x", retStatus);
    }

    return (PVOID)(ULONG_PTR) retStatus;
}

PVOID putAudioFrameRoutine(PVOID args)
{
    STATUS retStatus = STATUS_SUCCESS;
    PSampleCustomData data = (PSampleCustomData) args;
    Frame frame;
    UINT32 fileIndex = 0;
    STATUS status;
    UINT64 runningTime;

    CHK(data != NULL, STATUS_NULL_ARG);

    frame.frameData = data->audioFrames[fileIndex].buffer;
    frame.size = data->audioFrames[fileIndex].size;
    frame.version = FRAME_CURRENT_VERSION;
    frame.trackId = DEFAULT_AUDIO_TRACK_ID;
    frame.duration = 0;
    frame.decodingTs = 0;     // relative time mode
    frame.presentationTs = 0; // relative time mode
    frame.index = 0;
    frame.flags = FRAME_FLAG_NONE; // audio track is not used to cut fragment

    while (defaultGetTime() < data->streamStopTime) {
        // no audio can be put until first video frame is put
        if (ATOMIC_LOAD_BOOL(&data->firstVideoFramePut)) {
            status = putKinesisVideoFrame(data->streamHandle, &frame);
            if (STATUS_FAILED(status)) {
                printf("putKinesisVideoFrame for audio failed with 0x%08x\n", status);
                status = STATUS_SUCCESS;
            }

            frame.presentationTs += SAMPLE_AUDIO_FRAME_DURATION;
            frame.decodingTs = frame.presentationTs;
            frame.index++;

            fileIndex = (fileIndex + 1) % NUMBER_OF_AAC_FRAME_FILES;
            frame.frameData = data->audioFrames[fileIndex].buffer;
            frame.size = data->audioFrames[fileIndex].size;

            // synchronize putKinesisVideoFrame to running time
            runningTime = defaultGetTime() - data->streamStartTime;
            if (runningTime < frame.presentationTs) {
                THREAD_SLEEP(frame.presentationTs - runningTime);
            }
        }
    }

CleanUp:

    if (retStatus != STATUS_SUCCESS) {
        printf("putAudioFrameRoutine failed with 0x%08x", retStatus);
    }

    return (PVOID)(ULONG_PTR) retStatus;
}

INT32 main(INT32 argc, CHAR* argv[])
{
    PDeviceInfo pDeviceInfo = NULL;
    PStreamInfo pStreamInfo = NULL;
    PClientCallbacks pClientCallbacks = NULL;
    PStreamCallbacks pStreamCallbacks = NULL;
    CLIENT_HANDLE clientHandle = INVALID_CLIENT_HANDLE_VALUE;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;
    STATUS retStatus = STATUS_SUCCESS;
    PCHAR accessKey = NULL, secretKey = NULL, sessionToken = NULL, streamName = NULL, region = NULL, cacertPath = NULL;
    UINT64 streamStopTime, streamingDuration = DEFAULT_STREAM_DURATION, fileSize = 0;
    TID audioSendTid, videoSendTid;
    SampleCustomData data;
    UINT32 i;
    CHAR filePath[MAX_PATH_LEN + 1];
    PTrackInfo pAudioTrack = NULL;
    BYTE audioCpd[KVS_AAC_CPD_SIZE_BYTE];

    MEMSET(&data, 0x00, SIZEOF(SampleCustomData));

    if (argc < 2) {
        printf("Usage: AWS_ACCESS_KEY_ID=SAMPLEKEY AWS_SECRET_ACCESS_KEY=SAMPLESECRET %s <stream_name> <duration_in_seconds> <frame_files_path>\n",
               argv[0]);
        CHK(FALSE, STATUS_INVALID_ARG);
    }

    if ((accessKey = getenv(ACCESS_KEY_ENV_VAR)) == NULL || (secretKey = getenv(SECRET_KEY_ENV_VAR)) == NULL) {
        printf("Error missing credentials\n");
        CHK(FALSE, STATUS_INVALID_ARG);
    }

    MEMSET(data.sampleDir, 0x00, MAX_PATH_LEN + 1);
    if (argc < 4) {
        STRCPY(data.sampleDir, (PCHAR) "../samples");
    } else {
        STRNCPY(data.sampleDir, argv[3], MAX_PATH_LEN);
        if (data.sampleDir[STRLEN(data.sampleDir) - 1] == '/') {
            data.sampleDir[STRLEN(data.sampleDir) - 1] = '\0';
        }
    }

    printf("Loading audio frames...\n");
    for (i = 0; i < NUMBER_OF_AAC_FRAME_FILES; ++i) {
        SNPRINTF(filePath, MAX_PATH_LEN, "%s/aacSampleFrames/sample-%03d.aac", data.sampleDir, i + 1);
        CHK_STATUS(readFile(filePath, TRUE, NULL, &fileSize));
        data.audioFrames[i].buffer = (PBYTE) MEMALLOC(fileSize);
        data.audioFrames[i].size = fileSize;
        CHK_STATUS(readFile(filePath, TRUE, data.audioFrames[i].buffer, &fileSize));
    }
    printf("Done loading audio frames.\n");

    printf("Loading video frames...\n");
    for (i = 0; i < NUMBER_OF_H264_FRAME_FILES; ++i) {
        SNPRINTF(filePath, MAX_PATH_LEN, "%s/h264SampleFrames/frame-%03d.h264", data.sampleDir, i + 1);
        CHK_STATUS(readFile(filePath, TRUE, NULL, &fileSize));
        data.videoFrames[i].buffer = (PBYTE) MEMALLOC(fileSize);
        data.videoFrames[i].size = fileSize;
        CHK_STATUS(readFile(filePath, TRUE, data.videoFrames[i].buffer, &fileSize));
    }
    printf("Done loading video frames.\n");

    cacertPath = getenv(CACERT_PATH_ENV_VAR);
    sessionToken = getenv(SESSION_TOKEN_ENV_VAR);
    streamName = argv[1];
    if ((region = getenv(DEFAULT_REGION_ENV_VAR)) == NULL) {
        region = (PCHAR) DEFAULT_AWS_REGION;
    }

    if (argc >= 3) {
        // Get the duration and convert to an integer
        CHK_STATUS(STRTOUI64(argv[2], NULL, 10, &streamingDuration));
        printf("streaming for %" PRIu64 " seconds\n", streamingDuration);
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
    pAudioTrack = pStreamInfo->streamCaps.trackInfoList[0].trackId == DEFAULT_AUDIO_TRACK_ID ? &pStreamInfo->streamCaps.trackInfoList[0]
                                                                                             : &pStreamInfo->streamCaps.trackInfoList[1];
    // generate audio cpd
    pAudioTrack->codecPrivateData = audioCpd;
    pAudioTrack->codecPrivateDataSize = KVS_AAC_CPD_SIZE_BYTE;
    CHK_STATUS(mkvgenGenerateAacCpd(AAC_LC, AUDIO_TRACK_SAMPLING_RATE, AUDIO_TRACK_CHANNEL_CONFIG, pAudioTrack->codecPrivateData,
                                    pAudioTrack->codecPrivateDataSize));

    // use relative time mode. Buffer timestamps start from 0
    pStreamInfo->streamCaps.absoluteFragmentTimes = FALSE;

    data.startTime = GETTIME();
    data.firstFrame = TRUE;
    CHK_STATUS(createDefaultCallbacksProviderWithAwsCredentials(accessKey, secretKey, sessionToken, MAX_UINT64, region, cacertPath, NULL, NULL,
                                                                &pClientCallbacks));

    if (NULL != getenv(ENABLE_FILE_LOGGING)) {
        if ((retStatus = addFileLoggerPlatformCallbacksProvider(pClientCallbacks, FILE_LOGGING_BUFFER_SIZE, MAX_NUMBER_OF_LOG_FILES,
                                                                (PCHAR) FILE_LOGGER_LOG_FILE_DIRECTORY_PATH, TRUE) != STATUS_SUCCESS)) {
            printf("File logging enable option failed with 0x%08x error code\n", retStatus);
        }
    }

    CHK_STATUS(createStreamCallbacks(&pStreamCallbacks));
    CHK_STATUS(addStreamCallbacks(pClientCallbacks, pStreamCallbacks));

    CHK_STATUS(createKinesisVideoClient(pDeviceInfo, pClientCallbacks, &clientHandle));
    CHK_STATUS(createKinesisVideoStreamSync(clientHandle, pStreamInfo, &streamHandle));

    data.streamStopTime = streamStopTime;
    data.streamHandle = streamHandle;
    data.streamStartTime = defaultGetTime();
    ATOMIC_STORE_BOOL(&data.firstVideoFramePut, FALSE);

    THREAD_CREATE(&videoSendTid, putVideoFrameRoutine, (PVOID) &data);
    THREAD_CREATE(&audioSendTid, putAudioFrameRoutine, (PVOID) &data);

    THREAD_JOIN(videoSendTid, NULL);
    THREAD_JOIN(audioSendTid, NULL);

    CHK_STATUS(stopKinesisVideoStreamSync(streamHandle));
    CHK_STATUS(freeKinesisVideoStream(&streamHandle));
    CHK_STATUS(freeKinesisVideoClient(&clientHandle));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        printf("Failed with status 0x%08x\n", retStatus);
    }

    for (i = 0; i < NUMBER_OF_AAC_FRAME_FILES; ++i) {
        SAFE_MEMFREE(data.audioFrames[i].buffer);
    }

    for (i = 0; i < NUMBER_OF_H264_FRAME_FILES; ++i) {
        SAFE_MEMFREE(data.videoFrames[i].buffer);
    }
    freeDeviceInfo(&pDeviceInfo);
    freeStreamInfoProvider(&pStreamInfo);
    freeKinesisVideoStream(&streamHandle);
    freeKinesisVideoClient(&clientHandle);
    freeCallbacksProvider(&pClientCallbacks);

    return (INT32) retStatus;
}
