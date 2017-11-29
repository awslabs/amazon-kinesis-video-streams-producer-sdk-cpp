#pragma GCC diagnostic ignored "-Wwrite-strings"

#include "gtest/gtest.h"
#include <com/amazonaws/kinesis/video/utils/Include.h>
#include <com/amazonaws/kinesis/video/mkvgen/Include.h>

#include "src/mkvgen/src/Include_i.h"

#define MKV_TEST_TIMECODE_SCALE     (1 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

#define MKV_TEST_BUFFER_SIZE        MKV_MAX_CODEC_PRIVATE_LEN + 100000
#define MKV_TEST_CLUSTER_DURATION  (5 * HUNDREDS_OF_NANOS_IN_A_SECOND)
#define MKV_TEST_FRAME_DURATION    (40 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

#define MKV_TEST_BEHAVIOR_FLAGS         (MKV_GEN_KEY_FRAME_PROCESSING | MKV_GEN_IN_STREAM_TIME)

#define MKV_TEST_CONTENT_TYPE           "video/teststream"
#define MKV_TEST_CODEC_ID               "TEST_CODEC_ID"
#define MKV_TEST_TRACK_NAME             "test track"

#define MKV_TEST_CUSTOM_DATA            0x12345

/**
 * Callback function
 */
UINT64 getTimeCallback(UINT64);

extern BOOL gTimeCallbackCalled;

class MkvgenTestBase : public ::testing::Test {
public:
    MkvgenTestBase(): mBuffer(NULL), mMkvGenerator(NULL) {}

protected:
    PBYTE mBuffer;
    PMkvGenerator mMkvGenerator;

    STATUS CreateMkvGenerator()
    {
        mBuffer = (PBYTE)MEMALLOC(MKV_TEST_BUFFER_SIZE);

        // Create the MKV generator
        EXPECT_EQ(STATUS_SUCCESS, createMkvGenerator(MKV_TEST_CONTENT_TYPE, MKV_TEST_BEHAVIOR_FLAGS, MKV_TEST_TIMECODE_SCALE,
                MKV_TEST_CLUSTER_DURATION,MKV_TEST_CODEC_ID, MKV_TEST_TRACK_NAME, NULL, 0, NULL, 0, &mMkvGenerator));

        return STATUS_SUCCESS;
    };

    virtual void SetUp()
    {
        printf("\nSetting up test: %s\n", GetTestName());
        ASSERT_TRUE(STATUS_SUCCEEDED(CreateMkvGenerator()));

        // Reset the globals
        gTimeCallbackCalled = FALSE;
    };

    virtual void TearDown()
    {
        printf("\nTearing down test: %s\n", GetTestName());

        // Free the scratch buffer
        if (mBuffer) {
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
        return (PCHAR) ::testing::UnitTest::GetInstance()->current_test_info()->test_case_name();
    };


private:

};
