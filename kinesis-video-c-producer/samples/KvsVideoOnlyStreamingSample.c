#include <com/amazonaws/kinesis/video/cproducer/Include.h>

#define DEFAULT_RETENTION_PERIOD          2 * HUNDREDS_OF_NANOS_IN_AN_HOUR
#define DEFAULT_BUFFER_DURATION           120 * HUNDREDS_OF_NANOS_IN_A_SECOND
#define DEFAULT_CALLBACK_CHAIN_COUNT      5
#define DEFAULT_KEY_FRAME_INTERVAL        45
#define DEFAULT_FPS_VALUE                 25
#define DEFAULT_STREAM_DURATION           20 * HUNDREDS_OF_NANOS_IN_A_SECOND
#define DEFAULT_STORAGE_SIZE              20 * 1024 * 1024
#define RECORDED_FRAME_AVG_BITRATE_BIT_PS 3800000

#define NUMBER_OF_FRAME_FILES 403

#define FILE_LOGGING_BUFFER_SIZE (100 * 1024)
#define MAX_NUMBER_OF_LOG_FILES  5
STATUS readFrameData(PFrame pFrame, PCHAR frameFilePath)
{
    STATUS retStatus = STATUS_SUCCESS;
    CHAR filePath[MAX_PATH_LEN + 1];
    UINT32 index;
    UINT64 size;

    CHK(pFrame != NULL, STATUS_NULL_ARG);

    index = pFrame->index % NUMBER_OF_FRAME_FILES + 1;
    SNPRINTF(filePath, MAX_PATH_LEN, "%s/frame-%03d.h264", frameFilePath, index);
    size = pFrame->size;

    // Get the size and read into frame
    CHK_STATUS(readFile(filePath, TRUE, NULL, &size));
    CHK_STATUS(readFile(filePath, TRUE, pFrame->frameData, &size));

    pFrame->size = (UINT32) size;

    if (pFrame->flags == FRAME_FLAG_KEY_FRAME) {
        defaultLogPrint(LOG_LEVEL_DEBUG, "", "Key frame file %s, size %" PRIu64, filePath, pFrame->size);
    }

CleanUp:

    return retStatus;
}

// Forward declaration of the default thread sleep function
VOID defaultThreadSleep(UINT64);

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
    CHAR frameFilePath[MAX_PATH_LEN + 1];
    Frame frame;
    BYTE frameBuffer[200000]; // Assuming this is enough
    UINT32 frameSize = SIZEOF(frameBuffer), frameIndex = 0, fileIndex = 0;
    UINT64 streamStopTime, streamingDuration = DEFAULT_STREAM_DURATION;
    DOUBLE startUpLatency;
    BOOL firstFrame = TRUE;
    UINT64 startTime;

    if (argc < 2) {
        defaultLogPrint(
            LOG_LEVEL_ERROR, "",
            "Usage: AWS_ACCESS_KEY_ID=SAMPLEKEY AWS_SECRET_ACCESS_KEY=SAMPLESECRET %s <stream_name> <duration_in_seconds> <frame_files_path>\n",
            argv[0]);
        CHK(FALSE, STATUS_INVALID_ARG);
    }

    if ((accessKey = getenv(ACCESS_KEY_ENV_VAR)) == NULL || (secretKey = getenv(SECRET_KEY_ENV_VAR)) == NULL) {
        defaultLogPrint(LOG_LEVEL_ERROR, "", "Error missing credentials");
        CHK(FALSE, STATUS_INVALID_ARG);
    }

    MEMSET(frameFilePath, 0x00, MAX_PATH_LEN + 1);
    if (argc < 4) {
        STRCPY(frameFilePath, (PCHAR) "../samples/h264SampleFrames");
    } else {
        STRNCPY(frameFilePath, argv[3], MAX_PATH_LEN);
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
        streamingDuration *= HUNDREDS_OF_NANOS_IN_A_SECOND;
    }

    streamStopTime = defaultGetTime() + streamingDuration;

    // default storage size is 128MB. Use setDeviceInfoStorageSize after create to change storage size.
    CHK_STATUS(createDefaultDeviceInfo(&pDeviceInfo));
    // adjust members of pDeviceInfo here if needed
    pDeviceInfo->clientInfo.loggerLogLevel = LOG_LEVEL_DEBUG;
    pDeviceInfo->storageInfo.storageSize = DEFAULT_STORAGE_SIZE;

    CHK_STATUS(createRealtimeVideoStreamInfoProvider(streamName, DEFAULT_RETENTION_PERIOD, DEFAULT_BUFFER_DURATION, &pStreamInfo));
    CHK_STATUS(setStreamInfoBasedOnStorageSize(DEFAULT_STORAGE_SIZE, RECORDED_FRAME_AVG_BITRATE_BIT_PS, 1, pStreamInfo));
    // adjust members of pStreamInfo here if needed

    startTime = GETTIME();
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

    // setup dummy frame
    MEMSET(frameBuffer, 0x00, frameSize);
    frame.frameData = frameBuffer;
    frame.version = FRAME_CURRENT_VERSION;
    frame.trackId = DEFAULT_VIDEO_TRACK_ID;
    frame.duration = HUNDREDS_OF_NANOS_IN_A_SECOND / DEFAULT_FPS_VALUE;
    frame.decodingTs = defaultGetTime(); // current time
    frame.presentationTs = frame.decodingTs;

    while (defaultGetTime() < streamStopTime) {
        frame.index = frameIndex;
        frame.flags = fileIndex % DEFAULT_KEY_FRAME_INTERVAL == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        frame.size = SIZEOF(frameBuffer);

        CHK_STATUS(readFrameData(&frame, frameFilePath));

        CHK_STATUS(putKinesisVideoFrame(streamHandle, &frame));
        if (firstFrame) {
            startUpLatency = (DOUBLE)(GETTIME() - startTime) / (DOUBLE) HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
            DLOGD("Start up latency: %lf ms", startUpLatency);
            firstFrame = FALSE;
        }
        defaultThreadSleep(frame.duration);

        frame.decodingTs += frame.duration;
        frame.presentationTs = frame.decodingTs;
        frameIndex++;
        fileIndex++;
        fileIndex = fileIndex % NUMBER_OF_FRAME_FILES;
    }

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
