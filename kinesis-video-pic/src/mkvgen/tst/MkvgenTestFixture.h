#if 0
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

#include "gtest/gtest.h"
#include <com/amazonaws/kinesis/video/utils/Include.h>
#include <com/amazonaws/kinesis/video/mkvgen/Include.h>

#include "src/mkvgen/src/Include_i.h"

#define MKV_TEST_TIMECODE_SCALE (1 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

#define MKV_TEST_BUFFER_SIZE      MKV_MAX_CODEC_PRIVATE_LEN + 100000
#define MKV_TEST_CLUSTER_DURATION (5 * HUNDREDS_OF_NANOS_IN_A_SECOND)
#define MKV_TEST_FRAME_DURATION   (40 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

#define MKV_TEST_BEHAVIOR_FLAGS (MKV_GEN_KEY_FRAME_PROCESSING | MKV_GEN_IN_STREAM_TIME)

#define MKV_TEST_CONTENT_TYPE ((PCHAR) "video/teststream")
#define MKV_TEST_CODEC_ID     ((PCHAR) "TestCodec")
#define MKV_TEST_TRACK_NAME   ((PCHAR) "TestTrack")

#define MKV_TEST_CLIENT_ID ((PCHAR) "TestClientId")

#define MKV_TEST_CUSTOM_DATA 0x12345

#define MKV_TEST_TAG_COUNT        5
#define MKV_TEST_TRACK_INFO_COUNT 1
#define MKV_TEST_TRACKID          1

#define MKV_TEST_DEFAULT_TRACK_WIDTH  1280
#define MKV_TEST_DEFAULT_TRACK_HEIGHT 720

#define MKV_TEST_SEGMENT_UUID ((PBYTE) "0123456789abcdef")

/**
 * Callback function
 */
UINT64 getTimeCallback(UINT64);

extern BOOL gTimeCallbackCalled;

class MkvgenTestBase : public ::testing::Test {
  public:
    MkvgenTestBase() : mBuffer(NULL), mMkvGenerator(NULL)
    {
    }

  protected:
    PBYTE mBuffer;
    PMkvGenerator mMkvGenerator;
    TrackInfo mTrackInfo;
    UINT32 mTrackInfoCount;

    STATUS CreateMkvGenerator()
    {
        STATUS retStatus = STATUS_SUCCESS;
        mBuffer = (PBYTE) MEMALLOC(MKV_TEST_BUFFER_SIZE);

        // Create the MKV generator
        retStatus = createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE, MKV_TEST_CLUSTER_DURATION,
                                       MKV_TEST_SEGMENT_UUID, &mTrackInfo, mTrackInfoCount, MKV_TEST_CLIENT_ID, NULL, 0, &mMkvGenerator);
        EXPECT_EQ(STATUS_SUCCESS, retStatus);

        return retStatus;
    };

    virtual void SetUp()
    {
        UINT32 logLevel = 0;
        auto logLevelStr = GETENV("AWS_KVS_LOG_LEVEL");
        if (logLevelStr != NULL) {
            assert(STRTOUI32(logLevelStr, NULL, 10, &logLevel) == STATUS_SUCCESS);
            SET_LOGGER_LOG_LEVEL(logLevel);
        }

        mTrackInfoCount = MKV_TEST_TRACK_INFO_COUNT;
        mTrackInfo.version = TRACK_INFO_CURRENT_VERSION;
        mTrackInfo.trackId = MKV_TEST_TRACKID;
        STRCPY(mTrackInfo.codecId, MKV_TEST_CODEC_ID);
        STRCPY(mTrackInfo.trackName, MKV_TEST_TRACK_NAME);
        mTrackInfo.codecPrivateData = NULL;
        mTrackInfo.codecPrivateDataSize = 0;
        mTrackInfo.trackType = MKV_TRACK_INFO_TYPE_VIDEO;
        mTrackInfo.trackCustomData.trackVideoConfig.videoWidth = MKV_TEST_DEFAULT_TRACK_WIDTH;
        mTrackInfo.trackCustomData.trackVideoConfig.videoHeight = MKV_TEST_DEFAULT_TRACK_HEIGHT;

        DLOGI("\nSetting up test: %s\n", GetTestName());
        ASSERT_TRUE(STATUS_SUCCEEDED(CreateMkvGenerator()));

        // Reset the globals
        gTimeCallbackCalled = FALSE;
    };

    virtual void TearDown()
    {
        DLOGI("\nTearing down test: %s\n", GetTestName());

        // Free the scratch buffer
        if (NULL != mBuffer) {
            MEMFREE(mBuffer);
            mBuffer = NULL;
        }

        if (mMkvGenerator) {
            freeMkvGenerator(mMkvGenerator);
            mMkvGenerator = NULL;
        }
    };

    PCHAR GetTestName()
    {
        return (PCHAR)::testing::UnitTest::GetInstance()->current_test_info()->test_case_name();
    };

  private:
};
