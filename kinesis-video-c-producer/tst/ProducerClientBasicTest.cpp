#include "ProducerTestFixture.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

class ProducerClientBasicTest : public ProducerClientTestBase {
public:
    VOID initialize()
    {
        mSignalingLock = MUTEX_CREATE(FALSE);
        mStreamsCreated = CVAR_CREATE();
        MEMSET(mClients, 0x00, SIZEOF(mClients));
        MEMSET(mClientCallbacks, 0x00, SIZEOF(mClientCallbacks));
        mActiveStreamCount = 0;
        mActiveClientCount = 0;
    }

    VOID deinitialize()
    {
        if (IS_VALID_MUTEX_VALUE(mSignalingLock)) {
            MUTEX_FREE(mSignalingLock);
            mSignalingLock = INVALID_MUTEX_VALUE;
        }

        if (IS_VALID_CVAR_VALUE(mStreamsCreated)) {
            CVAR_FREE(mStreamsCreated);
            mStreamsCreated = INVALID_CVAR_VALUE;
        }
    }

    static PVOID staticCreateProducerRoutine(PVOID);
    static PVOID staticCreateProducerClientRoutine(PVOID);
    static PVOID staticProducerClientStartRoutine(PVOID);

    MUTEX mSignalingLock;
    CVAR mStreamsCreated;
    CLIENT_HANDLE mClients[TEST_STREAM_COUNT];
    PClientCallbacks mClientCallbacks[TEST_STREAM_COUNT];
    volatile UINT32 mActiveStreamCount;
    volatile UINT32 mActiveClientCount;
};

extern ProducerClientTestBase* gProducerClientTestBase;

PVOID ProducerClientBasicTest::staticCreateProducerClientRoutine(PVOID arg)
{
    UINT64 index = (UINT64) arg;
    StreamInfo streamInfo;
    TrackInfo trackInfo;
    STATUS retStatus;
    ProducerClientBasicTest* pTest = (ProducerClientBasicTest*) gProducerClientTestBase;
    PAuthCallbacks pAuthCallbacks;
    DeviceInfo deviceInfo;

    MEMSET(&streamInfo, 0x00, SIZEOF(StreamInfo));
    MEMSET(&trackInfo, 0x00, SIZEOF(TrackInfo));
    MEMSET(&deviceInfo, 0x00, SIZEOF(DeviceInfo));

    deviceInfo.version = DEVICE_INFO_CURRENT_VERSION;
    STRCPY(deviceInfo.name, TEST_DEVICE_INFO_NAME);
    STRCPY(deviceInfo.clientId, TEST_CLIENT_ID);
    deviceInfo.streamCount = 1;
    deviceInfo.tagCount = 0;
    deviceInfo.tags = NULL;
    deviceInfo.storageInfo.version = STORAGE_INFO_CURRENT_VERSION;
    deviceInfo.storageInfo.storageType = DEVICE_STORAGE_TYPE_IN_MEM;
    deviceInfo.storageInfo.spillRatio = 0;
    deviceInfo.storageInfo.storageSize = MIN_STORAGE_ALLOCATION_SIZE;
    deviceInfo.storageInfo.rootDirectory[0] = '\0';
    deviceInfo.clientInfo.version = CLIENT_INFO_CURRENT_VERSION;
    deviceInfo.clientInfo.stopStreamTimeout = 0;
    deviceInfo.clientInfo.createStreamTimeout = 0;
    deviceInfo.clientInfo.createClientTimeout = 0;
    deviceInfo.clientInfo.offlineBufferAvailabilityTimeout = 0;
    deviceInfo.clientInfo.loggerLogLevel = GET_LOGGER_LOG_LEVEL();
    deviceInfo.clientInfo.logMetric = TRUE;

    EXPECT_EQ(STATUS_SUCCESS, createAbstractDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
            API_CALL_CACHE_TYPE_NONE,
            TEST_CACHING_ENDPOINT_PERIOD,
            pTest->mRegion,
            TEST_CONTROL_PLANE_URI,
            pTest->mCaCertPath,
            NULL,
            TEST_USER_AGENT,
            &pTest->mClientCallbacks[index]));

    EXPECT_EQ(STATUS_SUCCESS, createStaticAuthCallbacks(pTest->mClientCallbacks[index],
            pTest->mAccessKey,
            pTest->mSecretKey,
            pTest->mSessionToken,
            MAX_UINT64,
            &pAuthCallbacks));

    // Create the producer client
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(&deviceInfo,
            pTest->mClientCallbacks[index],
            &pTest->mClients[index]));

    trackInfo.version = TRACK_INFO_CURRENT_VERSION;
    trackInfo.trackId = DEFAULT_VIDEO_TRACK_ID;
    trackInfo.codecPrivateData = NULL;
    trackInfo.codecPrivateDataSize = 0;
    STRCPY(trackInfo.codecId, MKV_H264_AVC_CODEC_ID);
    STRCPY(trackInfo.trackName, (PCHAR) "kinesis_video");
    trackInfo.trackType = MKV_TRACK_INFO_TYPE_VIDEO;
    trackInfo.trackCustomData.trackVideoConfig.videoHeight = 1080;
    trackInfo.trackCustomData.trackVideoConfig.videoWidth = 1920;

    streamInfo.version = STORAGE_INFO_CURRENT_VERSION;
    streamInfo.kmsKeyId[0] = '\0';
    streamInfo.retention = 2 * HUNDREDS_OF_NANOS_IN_AN_HOUR;
    streamInfo.streamCaps.streamingType = STREAMING_TYPE_REALTIME;
    streamInfo.tagCount = 0;
    streamInfo.tags = NULL;
    STRCPY(streamInfo.streamCaps.contentType, MKV_H264_CONTENT_TYPE);
    streamInfo.streamCaps.fragmentAcks = TRUE;
    streamInfo.streamCaps.nalAdaptationFlags = NAL_ADAPTATION_FLAG_NONE;
    streamInfo.streamCaps.segmentUuid = NULL;
    streamInfo.streamCaps.absoluteFragmentTimes = TRUE;
    streamInfo.streamCaps.keyFrameFragmentation = TRUE;
    streamInfo.streamCaps.adaptive = TRUE;
    streamInfo.streamCaps.frameTimecodes = TRUE;
    streamInfo.streamCaps.recalculateMetrics = TRUE;
    streamInfo.streamCaps.recoverOnError = TRUE;
    streamInfo.streamCaps.avgBandwidthBps = 10 * 1024;
    streamInfo.streamCaps.bufferDuration = 20 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    streamInfo.streamCaps.replayDuration = 10 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    streamInfo.streamCaps.fragmentDuration = 2 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    streamInfo.streamCaps.connectionStalenessDuration = 20 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    streamInfo.streamCaps.maxLatency = TEST_MAX_STREAM_LATENCY;
    streamInfo.streamCaps.timecodeScale = 1 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    streamInfo.streamCaps.frameRate = 25;
    streamInfo.streamCaps.trackInfoCount = 1;
    streamInfo.streamCaps.trackInfoList = &trackInfo;
    streamInfo.streamCaps.frameOrderingMode = FRAME_ORDER_MODE_PASS_THROUGH;
    SPRINTF(streamInfo.name, "ScaryTestStream_%u", (UINT32) index);
	
    EXPECT_EQ(STATUS_SUCCESS, retStatus = createKinesisVideoStreamSync(pTest->mClients[index], &streamInfo, &pTest->mStreams[index]));

    if (++pTest->mActiveStreamCount == TEST_STREAM_COUNT) {
        CVAR_SIGNAL(pTest->mStreamsCreated);
    }

    return (PVOID) (UINT64) retStatus;
}

PVOID ProducerClientBasicTest::staticCreateProducerRoutine(PVOID arg)
{
    UINT64 index = (UINT64) arg;
    StreamInfo streamInfo;
    TrackInfo trackInfo;
    STATUS retStatus;
    ProducerClientBasicTest* pTest = (ProducerClientBasicTest*) gProducerClientTestBase;

    MEMSET(&streamInfo, 0x00, SIZEOF(StreamInfo));
    MEMSET(&trackInfo, 0x00, SIZEOF(TrackInfo));

    trackInfo.version = TRACK_INFO_CURRENT_VERSION;
    trackInfo.trackId = DEFAULT_VIDEO_TRACK_ID;
    trackInfo.codecPrivateData = NULL;
    trackInfo.codecPrivateDataSize = 0;
    STRCPY(trackInfo.codecId, MKV_H264_AVC_CODEC_ID);
    STRCPY(trackInfo.trackName, (PCHAR) "kinesis_video");
    trackInfo.trackType = MKV_TRACK_INFO_TYPE_VIDEO;
    trackInfo.trackCustomData.trackVideoConfig.videoHeight = 1080;
    trackInfo.trackCustomData.trackVideoConfig.videoWidth = 1920;

    streamInfo.version = STORAGE_INFO_CURRENT_VERSION;
    streamInfo.kmsKeyId[0] = '\0';
    streamInfo.retention = 2 * HUNDREDS_OF_NANOS_IN_AN_HOUR;
    streamInfo.streamCaps.streamingType = STREAMING_TYPE_REALTIME;
    streamInfo.tagCount = 0;
    streamInfo.tags = NULL;
    STRCPY(streamInfo.streamCaps.contentType, MKV_H264_CONTENT_TYPE);
    streamInfo.streamCaps.fragmentAcks = TRUE;
    streamInfo.streamCaps.nalAdaptationFlags = NAL_ADAPTATION_FLAG_NONE;
    streamInfo.streamCaps.segmentUuid = NULL;
    streamInfo.streamCaps.absoluteFragmentTimes = TRUE;
    streamInfo.streamCaps.keyFrameFragmentation = TRUE;
    streamInfo.streamCaps.adaptive = TRUE;
    streamInfo.streamCaps.frameTimecodes = TRUE;
    streamInfo.streamCaps.recalculateMetrics = TRUE;
    streamInfo.streamCaps.recoverOnError = TRUE;
    streamInfo.streamCaps.avgBandwidthBps = 10 * 1024;
    streamInfo.streamCaps.bufferDuration = TEST_STREAM_BUFFER_DURATION;
    streamInfo.streamCaps.replayDuration = 10 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    streamInfo.streamCaps.fragmentDuration = 2 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    streamInfo.streamCaps.connectionStalenessDuration = 20 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    streamInfo.streamCaps.maxLatency = TEST_MAX_STREAM_LATENCY;
    streamInfo.streamCaps.timecodeScale = 1 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    streamInfo.streamCaps.frameRate = 25;
    streamInfo.streamCaps.trackInfoCount = 1;
    streamInfo.streamCaps.trackInfoList = &trackInfo;
    streamInfo.streamCaps.frameOrderingMode = FRAME_ORDER_MODE_PASS_THROUGH;
    SPRINTF(streamInfo.name, "ScaryTestStream_%u", (UINT32) index);

    retStatus = createKinesisVideoStreamSync(pTest->mClientHandle, &streamInfo, &pTest->mStreams[index]);

    if (++pTest->mActiveStreamCount == TEST_STREAM_COUNT) {
        CVAR_SIGNAL(pTest->mStreamsCreated);
    }

    return (PVOID) (UINT64) retStatus;
}

PVOID ProducerClientBasicTest::staticProducerClientStartRoutine(PVOID arg)
{
    UINT64 streamIndex = (UINT64) arg;
    ProducerClientBasicTest* pTest = (ProducerClientBasicTest*) gProducerClientTestBase;
    STREAM_HANDLE streamHandle = pTest->mStreams[streamIndex];
    if (!IS_VALID_STREAM_HANDLE(streamHandle)) {
        DLOGE("Invalid stream handle");
        return NULL;
    }

    UINT32 index = 0, persistentMetadataIndex = 0;
    UINT64 timestamp = GETTIME();
    Frame frame;
    std::string persistentMetadataName;
    TID tid = GETTID();

    PStreamInfo pStreamInfo;
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamGetStreamInfo(streamHandle, &pStreamInfo));

    // Set an indicator that the producer is not stopped
    pTest->mProducerStopped = FALSE;

    // Increment the active stream/producer count
    pTest->mActiveClientCount++;

    // Loop until cancelled
    frame.version = FRAME_CURRENT_VERSION;
    frame.duration = TEST_FRAME_DURATION;
    frame.frameData = pTest->mFrameBuffer;
    frame.trackId = DEFAULT_VIDEO_TRACK_ID;

    MEMSET(frame.frameData, 0x55, pTest->mFrameSize);

    BYTE cpd[] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x34,
                  0xAC, 0x2B, 0x40, 0x1E, 0x00, 0x78, 0xD8, 0x08,
                  0x80, 0x00, 0x01, 0xF4, 0x00, 0x00, 0xEA, 0x60,
                  0x47, 0xA5, 0x50, 0x00, 0x00, 0x00, 0x01, 0x68,
                  0xEE, 0x3C, 0xB0};
    UINT32 cpdSize = SIZEOF(cpd);

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(streamHandle, cpdSize, cpd, DEFAULT_VIDEO_TRACK_ID));

    while (!pTest->mStopProducer) {
        // Produce frames
        timestamp = GETTIME();

        frame.index = index++;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.size = pTest->mFrameSize;

        // Add small variation to the frame size (if larger frames
        if (frame.size > 100) {
            frame.size -= RAND() % 100;
        }

        // Key frame every 50th
        frame.flags = (frame.index % TEST_KEY_FRAME_INTERVAL == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

        DLOGD("Putting frame for stream: %s, TID: 0x%" PRIx64 ", Id: %u, Key Frame: %s, Size: %u, Dts: %" PRIu64 ", Pts: %" PRIu64,
              pStreamInfo->name,
              tid,
              frame.index,
              (((frame.flags & FRAME_FLAG_KEY_FRAME) == FRAME_FLAG_KEY_FRAME) ? "true" : "false"),
              frame.size,
              frame.decodingTs,
              frame.presentationTs);

        // Apply some non-persistent metadata every few frames
        if (frame.index % 20 == 0) {
            std::ostringstream metadataName;
            std::ostringstream metadataValue;

            metadataName << "MetadataNameForFrame_" << frame.index;
            metadataValue << "MetadataValueForFrame_" << frame.index;
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(streamHandle,
                                                                      (PCHAR) metadataName.str().c_str(),
                                                                      (PCHAR) metadataValue.str().c_str(),
                                                                      FALSE));
        }

        // Apply some persistent metadata on a larger intervals to span fragments
        if (frame.index % 60 == 0) {
            std::ostringstream metadataName;
            std::ostringstream metadataValue;

            metadataName << "PersistentMetadataName_" << persistentMetadataIndex;

            // Set or clear persistent metadata every other time.
            if (persistentMetadataIndex % 2 == 0) {
                persistentMetadataName = metadataName.str();
                metadataValue << "PersistentMetadataValue_" << persistentMetadataIndex;
            }

            persistentMetadataIndex++;
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(streamHandle,
                                                                      (PCHAR) persistentMetadataName.c_str(),
                                                                      (PCHAR) metadataValue.str().c_str(),
                                                                      TRUE));
        }

        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &frame));

        THREAD_SLEEP(TEST_FRAME_DURATION);
    }

    DLOGD("Stopping the stream: %s", pStreamInfo->name);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle)) << "Timed out awaiting for the stream stop notification";

    freeKinesisVideoClient(&pTest->mClients[streamIndex]);
    freeCallbacksProvider(&pTest->mClientCallbacks[streamIndex]);

    pTest->mClients[streamIndex] = INVALID_CLIENT_HANDLE_VALUE;
    pTest->mStreams[streamIndex] = INVALID_STREAM_HANDLE_VALUE;

    // Indicate that the producer routine had stopped
    if (--pTest->mActiveClientCount == 0) {
        pTest->mProducerStopped = true;
    }

    return NULL;
}

PVOID staticProducerRoutine(PVOID arg)
{
    STREAM_HANDLE streamHandle = (STREAM_HANDLE) arg;
    return gProducerClientTestBase->basicProducerRoutine(streamHandle);
}

PVOID staticProducerRoutineOffline(PVOID arg)
{
    STREAM_HANDLE streamHandle = (STREAM_HANDLE) arg;
    return gProducerClientTestBase->basicProducerRoutine(streamHandle, STREAMING_TYPE_OFFLINE);
}

PVOID ProducerClientTestBase::basicProducerRoutine(STREAM_HANDLE streamHandle, STREAMING_TYPE streamingType)
{
    if (!IS_VALID_STREAM_HANDLE(streamHandle)) {
        DLOGE("Invalid stream handle");
        return NULL;
    }

    UINT32 index = 0, persistentMetadataIndex = 0;
    UINT64 timestamp = GETTIME();
    Frame frame;
    std::string persistentMetadataName;
    TID tid = GETTID();

    PStreamInfo pStreamInfo;
    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamGetStreamInfo(streamHandle, &pStreamInfo));

    // Set an indicator that the producer is not stopped
    mProducerStopped = FALSE;

    // Loop until cancelled
    frame.version = FRAME_CURRENT_VERSION;
    frame.duration = TEST_FRAME_DURATION;
    frame.frameData = mFrameBuffer;
    frame.trackId = DEFAULT_VIDEO_TRACK_ID;

    MEMSET(frame.frameData, 0x55, mFrameSize);

    BYTE cpd[] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x34,
                  0xAC, 0x2B, 0x40, 0x1E, 0x00, 0x78, 0xD8, 0x08,
                  0x80, 0x00, 0x01, 0xF4, 0x00, 0x00, 0xEA, 0x60,
                  0x47, 0xA5, 0x50, 0x00, 0x00, 0x00, 0x01, 0x68,
                  0xEE, 0x3C, 0xB0};
    UINT32 cpdSize = SIZEOF(cpd);

    EXPECT_EQ(STATUS_SUCCESS, kinesisVideoStreamFormatChanged(streamHandle, cpdSize, cpd, DEFAULT_VIDEO_TRACK_ID));

    while (!mStopProducer) {
        // Produce frames
        if (IS_OFFLINE_STREAMING_MODE(streamingType)) {
            timestamp += frame.duration;
        } else {
            timestamp = GETTIME();
        }

        frame.index = index++;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;
        frame.size = mFrameSize;

        // Add small variation to the frame size (if larger frames
        if (frame.size > 100) {
            frame.size -= RAND() % 100;
        }

        // Key frame every 50th
        frame.flags = (frame.index % TEST_KEY_FRAME_INTERVAL == 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

        DLOGD("Putting frame for stream: %s, TID: 0x%" PRIx64 ", Id: %u, Key Frame: %s, Size: %u, Dts: %" PRIu64 ", Pts: %" PRIu64,
              pStreamInfo->name,
              tid,
              frame.index,
              (((frame.flags & FRAME_FLAG_KEY_FRAME) == FRAME_FLAG_KEY_FRAME) ? "true" : "false"),
              frame.size,
              frame.decodingTs,
              frame.presentationTs);

        // Apply some non-persistent metadata every few frames
        if (frame.index % 20 == 0) {
            std::ostringstream metadataName;
            std::ostringstream metadataValue;

            metadataName << "MetadataNameForFrame_" << frame.index;
            metadataValue << "MetadataValueForFrame_" << frame.index;
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(streamHandle,
                                                                      (PCHAR) metadataName.str().c_str(),
                                                                      (PCHAR) metadataValue.str().c_str(),
                                                                      FALSE));
        }

        // Apply some persistent metadata on a larger intervals to span fragments
        if (frame.index % 60 == 0) {
            std::ostringstream metadataName;
            std::ostringstream metadataValue;

            metadataName << "PersistentMetadataName_" << persistentMetadataIndex;

            // Set or clear persistent metadata every other time.
            if (persistentMetadataIndex % 2 == 0) {
                persistentMetadataName = metadataName.str();
                metadataValue << "PersistentMetadataValue_" << persistentMetadataIndex;
            }

            persistentMetadataIndex++;
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(streamHandle,
                                                                      (PCHAR) persistentMetadataName.c_str(),
                                                                      (PCHAR) metadataValue.str().c_str(),
                                                                      TRUE));
        }

#if 0
            // Simulate EoFr first
if (frame.index % 50 == 0 && frame.index != 0) {
Frame eofr = EOFR_FRAME_INITIALIZER;
EXPECT_TRUE(kinesis_video_stream->putFrame(eofr));
}
#endif

        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &frame));

        // Sleep a while for non-offline modes
        if (streamingType != STREAMING_TYPE_OFFLINE) {
            THREAD_SLEEP(TEST_FRAME_DURATION);
        }
    }

    DLOGD("Stopping the stream: %s", pStreamInfo->name);
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle)) << "Timed out awaiting for the stream stop notification";

    // Indicate that the producer routine had stopped
    mProducerStopped = true;

    return NULL;
}

TEST_F(ProducerClientBasicTest, create_produce_stream)
{
    // Check if it's run with the env vars set if not bail out
    if (!mAccessKeyIdSet) {
        return;
    }

    createDefaultProducerClient(FALSE, 5 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    for (UINT32 i = 0; i < TEST_STREAM_COUNT; i++) {
        // Create the stream
        ASSERT_EQ(STATUS_SUCCESS, createTestStream(i, STREAMING_TYPE_REALTIME, 20 * HUNDREDS_OF_NANOS_IN_A_SECOND, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND));

        // Spin off the producer
        EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&mProducerThread, staticProducerRoutine, (PVOID) mStreams[i]));
        THREAD_DETACH(mProducerThread);
    }

#if 0
    // This section will simply demonstrate a stream termination, waiting for the termination and then cleanup and
    // a consequent re-creation and restart. Normally, after stopping, the customers application will have to
    // await until the stream stopped event is called.
    for (UINT32 iter = 0; iter < 10; iter++) {
        THREAD_SLEEP(10 * HUNDREDS_OF_NANOS_IN_A_SECOND);
        DLOGD("Stopping the streams");
        mStopProducer = TRUE;
        DLOGD("Waiting for the streams to finish and close...");
        THREAD_SLEEP(10 * HUNDREDS_OF_NANOS_IN_A_SECOND);

        DLOGD("Freeing the streams.");

        // Free the streams
        for (UINT32 i = 0; i < TEST_STREAM_COUNT; i++) {
            freeKinesisVideoStream(&mStreams[i]);
        }

        DLOGD("Starting the streams again");
        mStopProducer = FALSE;

        // Create new streams
        for (UINT32 i = 0; i < TEST_STREAM_COUNT; i++) {
            // Create the stream
            createTestStream(i);

            DLOGD("Stream has been created");

            // Spin off the producer
            EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&mProducerThread, staticProducerRoutine, (PVOID) mStreams[i]));
            THREAD_DETACH(mProducerThread);
        }
    }
#endif

    // Wait for some time to produce
    THREAD_SLEEP(TEST_EXECUTION_DURATION);

    // Indicate the cancel for the threads
    mStopProducer = TRUE;

    // Join the thread and wait to exit.
    // NOTE: This is not a right way of doing it as for the multiple stream scenario
    // it will have a potential race condition. This is for demo purposes only and the
    // real implementations should use proper signalling.
    // We will wait for 30 seconds for the thread to terminate
    UINT32 index = 0;
    do {
        THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    } while (index++ < 300 && !mProducerStopped);

    EXPECT_TRUE(mProducerStopped) << "Producer thread failed to stop cleanly";

    // We will block for some time due to an incorrect implementation of the awaiting code
    // NOTE: The proper implementation should use synchronization primitives to await for the
    // producer threads to finish properly - here we just simulate a media pipeline.
    THREAD_SLEEP(10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    freeStreams();
}

TEST_F(ProducerClientBasicTest, create_produce_stream_parallel)
{
    // Check if it's run with the env vars set if not bail out
    if (!mAccessKeyIdSet) {
        return;
    }

    initialize();

    createDefaultProducerClient(FALSE, 5 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    for (UINT64 i = 0; i < TEST_STREAM_COUNT; i++) {
        // Create the streams
        THREAD_SLEEP(300 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
        THREAD_CREATE(&mProducerThread, staticCreateProducerRoutine, (PVOID) i);
        THREAD_DETACH(mProducerThread);
    }

    // Await for all the streams to finish creating
    MUTEX_LOCK(mSignalingLock);
    ASSERT_EQ(STATUS_SUCCESS, CVAR_WAIT(mStreamsCreated, mSignalingLock, 10 * HUNDREDS_OF_NANOS_IN_A_SECOND));
    MUTEX_UNLOCK(mSignalingLock);

    for (UINT32 i = 0; i < TEST_STREAM_COUNT; i++) {
        // Spin off the producer
        EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&mProducerThread, staticProducerRoutine, (PVOID) mStreams[i]));
        THREAD_DETACH(mProducerThread);
    }

    // Wait for some time to produce
    THREAD_SLEEP(TEST_EXECUTION_DURATION);

    // Indicate the cancel for the threads
    mStopProducer = TRUE;

    UINT32 index = 0;
    do {
        THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    } while (index++ < 300 && !mProducerStopped);

    EXPECT_TRUE(mProducerStopped) << "Producer thread failed to stop cleanly";

    THREAD_SLEEP(10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    freeStreams();

    deinitialize();
}

TEST_F(ProducerClientBasicTest, create_produce_client_parallel)
{
    // Check if it's run with the env vars set if not bail out
    if (!mAccessKeyIdSet) {
        return;
    }

    initialize();

    for (UINT64 i = 0; i < TEST_STREAM_COUNT; i++) {
        // Create the streams
        THREAD_SLEEP(300 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
        THREAD_CREATE(&mProducerThread, staticCreateProducerClientRoutine, (PVOID) i);
        THREAD_DETACH(mProducerThread);
    }

    // Await for all the streams to finish creating
    MUTEX_LOCK(mSignalingLock);
    ASSERT_EQ(STATUS_SUCCESS, CVAR_WAIT(mStreamsCreated, mSignalingLock, 10 * HUNDREDS_OF_NANOS_IN_A_SECOND));
    MUTEX_UNLOCK(mSignalingLock);

    for (UINT64 i = 0; i < TEST_STREAM_COUNT; i++) {
        EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&mProducerThread, staticProducerClientStartRoutine, (PVOID) i));
        THREAD_DETACH(mProducerThread);
    }

    // Wait for some time to produce
    THREAD_SLEEP(TEST_EXECUTION_DURATION);

    // Indicate the cancel for the threads
    mStopProducer = TRUE;

    UINT32 index = 0;
    do {
        THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    } while (index++ < 300 && !mProducerStopped);

    EXPECT_TRUE(mProducerStopped) << "Producer thread failed to stop cleanly";

    THREAD_SLEEP(10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    freeStreams();

    deinitialize();
}

TEST_F(ProducerClientBasicTest, cachingEndpointProvider_Returns_EndpointFromCache_OnTokenRotation)
{
    const UINT32 ITERATION_COUNT = 3;

    // Check if it's run with the env vars set if not bail out
    if (!mAccessKeyIdSet) {
        return;
    }

    createDefaultProducerClient(TRUE);

    for (UINT32 i = 0; i < TEST_STREAM_COUNT; i++) {
        // Create the stream
        ASSERT_EQ(STATUS_SUCCESS, createTestStream(i, STREAMING_TYPE_REALTIME, 20 * HUNDREDS_OF_NANOS_IN_A_SECOND, 60 * HUNDREDS_OF_NANOS_IN_A_SECOND));

        // Spin off the producer
        EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&mProducerThread, staticProducerRoutine, (PVOID) mStreams[i]));
        THREAD_DETACH(mProducerThread);
    }

    // Wait for a couple of rotations and ensure we didn't call the endpoint
    THREAD_SLEEP(TEST_STREAMING_TOKEN_DURATION * ITERATION_COUNT);

    // Indicate the cancel for the threads
    mStopProducer = TRUE;

    // Join the thread and wait to exit.
    // NOTE: This is not a right way of doing it as for the multiple stream scenario
    // it will have a potential race condition. This is for demo purposes only and the
    // real implementations should use proper signalling.
    // We will wait for 30 seconds for the thread to terminate
    UINT32 index = 0;
    do {
        THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    } while (index++ < 300 && !mProducerStopped);

    EXPECT_TRUE(mProducerStopped) << "Producer thread failed to stop cleanly";

    // Expect the number of calls
    EXPECT_EQ((ITERATION_COUNT + 1) * TEST_STREAM_COUNT, mPutStreamFnCount);
    EXPECT_EQ((ITERATION_COUNT + 1) * TEST_STREAM_COUNT, mGetStreamingEndpointFnCount);
    EXPECT_EQ(0, mCurlCreateStreamCount);
    EXPECT_EQ(0, mCurlDescribeStreamCount);
    EXPECT_EQ(0, mCurlTagResourceCount);
    EXPECT_EQ(1 * TEST_STREAM_COUNT, mCurlGetDataEndpointCount);
    EXPECT_EQ((ITERATION_COUNT + 1) * TEST_STREAM_COUNT, mCurlPutMediaCount);

    // We will block for some time due to an incorrect implementation of the awaiting code
    // NOTE: The proper implementation should use synchronization primitives to await for the
    // producer threads to finish properly - here we just simulate a media pipeline.
    THREAD_SLEEP(10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    freeStreams();
}

TEST_F(ProducerClientBasicTest, createStreamStopSyncFree)
{
    PDeviceInfo pDeviceInfo;
    PClientCallbacks pClientCallbacks;
    CLIENT_HANDLE clientHandle;
    STREAM_HANDLE streamHandle;
    PStreamInfo pStreamInfo;
    CHAR streamName[MAX_STREAM_NAME_LEN + 1];

    STRNCPY(streamName, (PCHAR) TEST_STREAM_NAME, MAX_STREAM_NAME_LEN);
    streamName[MAX_STREAM_NAME_LEN] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, createDefaultDeviceInfo(&pDeviceInfo));
    pDeviceInfo->clientInfo.loggerLogLevel = GET_LOGGER_LOG_LEVEL();
    EXPECT_EQ(STATUS_SUCCESS, createRealtimeVideoStreamInfoProvider(streamName, TEST_RETENTION_PERIOD, TEST_STREAM_BUFFER_DURATION, &pStreamInfo));


    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProvider(5, mAccessKey, mSecretKey,
                                                             mSessionToken, GETTIME() + TEST_STREAMING_TOKEN_DURATION,
                                                             mRegion, TEST_CONTROL_PLANE_URI,
                                                             mCaCertPath, NULL, NULL,
                                                             API_CALL_CACHE_TYPE_NONE, TEST_CACHING_ENDPOINT_PERIOD,
                                                             FALSE, &pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(pDeviceInfo, pClientCallbacks, &clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStreamSync(clientHandle, pStreamInfo, &streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeDeviceInfo(&pDeviceInfo));
    EXPECT_EQ(STATUS_SUCCESS, freeStreamInfoProvider(&pStreamInfo));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
}

TEST_F(ProducerClientBasicTest, createStreamPutOneFrameStopSyncFree)
{
    PDeviceInfo pDeviceInfo;
    PClientCallbacks pClientCallbacks;
    CLIENT_HANDLE clientHandle;
    STREAM_HANDLE streamHandle;
    PStreamInfo pStreamInfo;
    CHAR streamName[MAX_STREAM_NAME_LEN + 1];
    Frame frame;
    frame.version = FRAME_CURRENT_VERSION;
    frame.duration = TEST_FRAME_DURATION;
    frame.frameData = mFrameBuffer;
    frame.trackId = DEFAULT_VIDEO_TRACK_ID;
    frame.index = 0;
    frame.decodingTs = 0;
    frame.presentationTs = 0;
    frame.size = mFrameSize;
    frame.flags = FRAME_FLAG_KEY_FRAME;


    STRNCPY(streamName, (PCHAR) TEST_STREAM_NAME, MAX_STREAM_NAME_LEN);
    streamName[MAX_STREAM_NAME_LEN] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, createDefaultDeviceInfo(&pDeviceInfo));
    pDeviceInfo->clientInfo.loggerLogLevel = GET_LOGGER_LOG_LEVEL();
    EXPECT_EQ(STATUS_SUCCESS, createRealtimeVideoStreamInfoProvider(streamName, TEST_RETENTION_PERIOD, TEST_STREAM_BUFFER_DURATION, &pStreamInfo));
    pStreamInfo->streamCaps.nalAdaptationFlags = NAL_ADAPTATION_FLAG_NONE;
    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProvider(5, mAccessKey, mSecretKey,
                                                             mSessionToken, GETTIME() + TEST_STREAMING_TOKEN_DURATION,
                                                             mRegion, TEST_CONTROL_PLANE_URI,
                                                             mCaCertPath, NULL, NULL,
                                                             API_CALL_CACHE_TYPE_NONE, TEST_CACHING_ENDPOINT_PERIOD,
                                                             FALSE, &pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(pDeviceInfo, pClientCallbacks, &clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStreamSync(clientHandle, pStreamInfo, &streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &frame));
    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeDeviceInfo(&pDeviceInfo));
    EXPECT_EQ(STATUS_SUCCESS, freeStreamInfoProvider(&pStreamInfo));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
}

TEST_F(ProducerClientBasicTest, createStreamPutMultipleFrameStopSyncFree)
{
    PDeviceInfo pDeviceInfo;
    PClientCallbacks pClientCallbacks;
    CLIENT_HANDLE clientHandle;
    STREAM_HANDLE streamHandle;
    PStreamInfo pStreamInfo;
    CHAR streamName[MAX_STREAM_NAME_LEN + 1];
    Frame frame;
    UINT32 i;

    frame.version = FRAME_CURRENT_VERSION;
    frame.duration = TEST_FRAME_DURATION;
    frame.frameData = mFrameBuffer;
    frame.trackId = DEFAULT_VIDEO_TRACK_ID;
    frame.index = 0;
    frame.decodingTs = 0;
    frame.presentationTs = 0;
    frame.size = mFrameSize;
    frame.flags = FRAME_FLAG_KEY_FRAME;


    STRNCPY(streamName, (PCHAR) TEST_STREAM_NAME, MAX_STREAM_NAME_LEN);
    streamName[MAX_STREAM_NAME_LEN] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, createDefaultDeviceInfo(&pDeviceInfo));
    pDeviceInfo->clientInfo.loggerLogLevel = GET_LOGGER_LOG_LEVEL();
    EXPECT_EQ(STATUS_SUCCESS, createRealtimeVideoStreamInfoProvider(streamName, TEST_RETENTION_PERIOD, TEST_STREAM_BUFFER_DURATION, &pStreamInfo));
    pStreamInfo->streamCaps.nalAdaptationFlags = NAL_ADAPTATION_FLAG_NONE;
    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProvider(5, mAccessKey, mSecretKey,
                                                             mSessionToken, MAX_UINT64,
                                                             mRegion, TEST_CONTROL_PLANE_URI,
                                                             mCaCertPath, NULL, NULL,
                                                             API_CALL_CACHE_TYPE_NONE, TEST_CACHING_ENDPOINT_PERIOD,
                                                             FALSE, &pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(pDeviceInfo, pClientCallbacks, &clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStreamSync(clientHandle, pStreamInfo, &streamHandle));

    for(i = 0; i < TEST_START_STOP_ITERATION_COUNT; i++) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &frame));
        THREAD_SLEEP(TEST_FRAME_DURATION);
        frame.index++;
        frame.decodingTs += TEST_FRAME_DURATION;
        frame.presentationTs = frame.decodingTs;
        frame.flags = frame.index % TEST_FPS == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
    }

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeDeviceInfo(&pDeviceInfo));
    EXPECT_EQ(STATUS_SUCCESS, freeStreamInfoProvider(&pStreamInfo));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
}

TEST_F(ProducerClientBasicTest, createStreamStreamUntilTokenRotationStopSyncFree)
{
    PDeviceInfo pDeviceInfo;
    PClientCallbacks pClientCallbacks;
    CLIENT_HANDLE clientHandle;
    STREAM_HANDLE streamHandle;
    PStreamInfo pStreamInfo;
    PAuthCallbacks pAuthCallbacks = NULL;
    CHAR streamName[MAX_STREAM_NAME_LEN + 1];
    Frame frame;
    UINT64 currentTime = GETTIME();
    UINT64 stopTime = currentTime +
                      TEST_STREAMING_TOKEN_DURATION +
                      10 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    frame.version = FRAME_CURRENT_VERSION;
    frame.duration = TEST_FRAME_DURATION;
    frame.frameData = mFrameBuffer;
    frame.trackId = DEFAULT_VIDEO_TRACK_ID;
    frame.index = 0;
    frame.decodingTs = 0;
    frame.presentationTs = 0;
    frame.size = mFrameSize;
    frame.flags = FRAME_FLAG_KEY_FRAME;

    STRNCPY(streamName, (PCHAR) TEST_STREAM_NAME, MAX_STREAM_NAME_LEN);
    streamName[MAX_STREAM_NAME_LEN] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, createDefaultDeviceInfo(&pDeviceInfo));
    pDeviceInfo->clientInfo.loggerLogLevel = GET_LOGGER_LOG_LEVEL();
    EXPECT_EQ(STATUS_SUCCESS, createRealtimeVideoStreamInfoProvider(streamName, TEST_RETENTION_PERIOD, TEST_STREAM_BUFFER_DURATION, &pStreamInfo));
    pStreamInfo->streamCaps.nalAdaptationFlags = NAL_ADAPTATION_FLAG_NONE;
    EXPECT_EQ(STATUS_SUCCESS, createAbstractDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
            API_CALL_CACHE_TYPE_NONE,
            TEST_CACHING_ENDPOINT_PERIOD,
            mRegion,
            TEST_CONTROL_PLANE_URI,
            mCaCertPath,
            NULL,
            NULL,
            &pClientCallbacks));

    UINT64 expiration = currentTime + TEST_STREAMING_TOKEN_DURATION;

    EXPECT_EQ(STATUS_SUCCESS, createRotatingStaticAuthCallbacks(pClientCallbacks,
                                                                mAccessKey,
                                                                mSecretKey,
                                                                mSessionToken,
                                                                expiration,
                                                                TEST_STREAMING_TOKEN_DURATION,
                                                                &pAuthCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(pDeviceInfo, pClientCallbacks, &clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStreamSync(clientHandle, pStreamInfo, &streamHandle));

    while(currentTime < stopTime) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &frame));
        THREAD_SLEEP(TEST_FRAME_DURATION);
        frame.index++;
        frame.decodingTs += TEST_FRAME_DURATION;
        frame.presentationTs = frame.decodingTs;
        frame.flags = frame.index % TEST_FPS == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        currentTime = GETTIME();
    }

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeDeviceInfo(&pDeviceInfo));
    EXPECT_EQ(STATUS_SUCCESS, freeStreamInfoProvider(&pStreamInfo));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
}

TEST_F(ProducerClientBasicTest, createStreamStreamUntilTokenRotationStopSyncFreeWithFileLogger)
{
    PDeviceInfo pDeviceInfo;
    PClientCallbacks pClientCallbacks;
    CLIENT_HANDLE clientHandle;
    STREAM_HANDLE streamHandle;
    PStreamInfo pStreamInfo;
    PAuthCallbacks pAuthCallbacks = NULL;
    CHAR streamName[MAX_STREAM_NAME_LEN + 1];
    Frame frame;
    UINT64 currentTime = GETTIME();
    UINT64 stopTime = currentTime +
                      TEST_STREAMING_TOKEN_DURATION +
                      10 * HUNDREDS_OF_NANOS_IN_A_SECOND;

    frame.version = FRAME_CURRENT_VERSION;
    frame.duration = TEST_FRAME_DURATION;
    frame.frameData = mFrameBuffer;
    frame.trackId = DEFAULT_VIDEO_TRACK_ID;
    frame.index = 0;
    frame.decodingTs = 0;
    frame.presentationTs = 0;
    frame.size = mFrameSize;
    frame.flags = FRAME_FLAG_KEY_FRAME;

    STRNCPY(streamName, (PCHAR) TEST_STREAM_NAME, MAX_STREAM_NAME_LEN);
    streamName[MAX_STREAM_NAME_LEN] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, createDefaultDeviceInfo(&pDeviceInfo));
    pDeviceInfo->clientInfo.loggerLogLevel = GET_LOGGER_LOG_LEVEL();
    EXPECT_EQ(STATUS_SUCCESS, createRealtimeVideoStreamInfoProvider(streamName, TEST_RETENTION_PERIOD, TEST_STREAM_BUFFER_DURATION, &pStreamInfo));
    pStreamInfo->streamCaps.nalAdaptationFlags = NAL_ADAPTATION_FLAG_NONE;
    EXPECT_EQ(STATUS_SUCCESS, createAbstractDefaultCallbacksProvider(TEST_DEFAULT_CHAIN_COUNT,
                                                                     API_CALL_CACHE_TYPE_NONE,
                                                                     TEST_CACHING_ENDPOINT_PERIOD,
                                                                     mRegion,
                                                                     TEST_CONTROL_PLANE_URI,
                                                                     mCaCertPath,
                                                                     NULL,
                                                                     NULL,
                                                                     &pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, addFileLoggerPlatformCallbacksProvider(pClientCallbacks, 100*1024, 3, (PCHAR) "/tmp", TRUE));

    UINT64 expiration = currentTime + TEST_STREAMING_TOKEN_DURATION;

    EXPECT_EQ(STATUS_SUCCESS, createRotatingStaticAuthCallbacks(pClientCallbacks,
                                                                mAccessKey,
                                                                mSecretKey,
                                                                mSessionToken,
                                                                expiration,
                                                                TEST_STREAMING_TOKEN_DURATION,
                                                                &pAuthCallbacks));

    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(pDeviceInfo, pClientCallbacks, &clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStreamSync(clientHandle, pStreamInfo, &streamHandle));

    while(currentTime < stopTime) {
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &frame));
        THREAD_SLEEP(TEST_FRAME_DURATION);
        frame.index++;
        frame.decodingTs += TEST_FRAME_DURATION;
        frame.presentationTs = frame.decodingTs;
        frame.flags = frame.index % TEST_FPS == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        currentTime = GETTIME();
    }

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeDeviceInfo(&pDeviceInfo));
    EXPECT_EQ(STATUS_SUCCESS, freeStreamInfoProvider(&pStreamInfo));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
}

TEST_F(ProducerClientBasicTest, createStreamPutMultipleFrameTimeoutLatencyStopSyncFree)
{
    PDeviceInfo pDeviceInfo;
    PClientCallbacks pClientCallbacks;
    CLIENT_HANDLE clientHandle;
    STREAM_HANDLE streamHandle;
    PStreamInfo pStreamInfo;
    CHAR streamName[MAX_STREAM_NAME_LEN + 1];
    Frame frame;
    UINT32 i;
    BYTE frameBuf[100000]; // large frames are needed
    UINT32 fragmentCount = 3, frameRate = 30, keyFrameInterval = 60;

    frame.version = FRAME_CURRENT_VERSION;
    frame.duration = 15 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    frame.frameData = frameBuf;
    frame.trackId = DEFAULT_VIDEO_TRACK_ID;
    frame.index = 0;
    frame.decodingTs = 0;
    frame.presentationTs = 0;
    frame.size = SIZEOF(frameBuf);
    frame.flags = FRAME_FLAG_KEY_FRAME;
    Frame eofr = EOFR_FRAME_INITIALIZER;

    STRNCPY(streamName, (PCHAR) TEST_STREAM_NAME, MAX_STREAM_NAME_LEN);
    streamName[MAX_STREAM_NAME_LEN] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, createDefaultDeviceInfo(&pDeviceInfo));
    pDeviceInfo->clientInfo.loggerLogLevel = GET_LOGGER_LOG_LEVEL();
    EXPECT_EQ(STATUS_SUCCESS, createRealtimeVideoStreamInfoProvider(streamName, TEST_RETENTION_PERIOD, TEST_STREAM_BUFFER_DURATION, &pStreamInfo));
    pStreamInfo->streamCaps.nalAdaptationFlags = NAL_ADAPTATION_FLAG_NONE;
    pStreamInfo->streamCaps.maxLatency = 15 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    pStreamInfo->streamCaps.replayDuration = 30 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    pStreamInfo->streamCaps.bufferDuration = 120 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    pStreamInfo->streamCaps.connectionStalenessDuration = 40 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    pStreamInfo->streamCaps.keyFrameFragmentation = TRUE;
    pStreamInfo->streamCaps.frameRate = frameRate;
    pStreamInfo->streamCaps.frameTimecodes = TRUE;

    EXPECT_EQ(STATUS_SUCCESS, createDefaultCallbacksProvider(5, mAccessKey, mSecretKey,
                                                             mSessionToken, MAX_UINT64,
                                                             mRegion, TEST_CONTROL_PLANE_URI,
                                                             mCaCertPath, NULL, NULL,
                                                             API_CALL_CACHE_TYPE_ALL, TEST_CACHING_ENDPOINT_PERIOD,
                                                             TRUE, &pClientCallbacks));
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClientSync(pDeviceInfo, pClientCallbacks, &clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStreamSync(clientHandle, pStreamInfo, &streamHandle));

    for(i = 0; i < fragmentCount * keyFrameInterval; i++) {
        frame.flags = frame.index % keyFrameInterval == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

        if (i == keyFrameInterval && frame.flags == FRAME_FLAG_KEY_FRAME) {
            DLOGD("PAUSING to cause a timeout");
            EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &eofr));
            THREAD_SLEEP(60 * HUNDREDS_OF_NANOS_IN_A_SECOND);
            DLOGD("PAUSING After the pause");
        }

        frame.decodingTs = GETTIME();
        frame.presentationTs = frame.decodingTs;

        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(streamHandle, &frame));
        THREAD_SLEEP(30 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
        frame.index++;
    }

    EXPECT_EQ(STATUS_SUCCESS, stopKinesisVideoStreamSync(streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoStream(&streamHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, freeDeviceInfo(&pDeviceInfo));
    EXPECT_EQ(STATUS_SUCCESS, freeStreamInfoProvider(&pStreamInfo));
    EXPECT_EQ(STATUS_SUCCESS, freeCallbacksProvider(&pClientCallbacks));
}

}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com;
