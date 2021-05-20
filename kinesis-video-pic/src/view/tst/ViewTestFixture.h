#if 0
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

#include "gtest/gtest.h"
#include <com/amazonaws/kinesis/video/utils/Include.h>
#include <com/amazonaws/kinesis/video/view/Include.h>

#include "src/view/src/Include_i.h"

#define MAX_VIEW_ITEM_COUNT                    100
#define MAX_VIEW_BUFFER_DURATION               1000
#define VIEW_NOTIFICATION_CALLBACK_CUSTOM_DATA 1122
#define CHECK_AGAINST_ACKTIMESTAMP             TRUE

#define MAX_VIEW_ITERATION_COUNT 50
#define VIEW_ITEM_DURATION       10
#define VIEW_ITEM_DURATION_LARGE 20

#define VIEW_ITEM_ALLOCAITON_SIZE 100

class ViewTestBase : public ::testing::Test {
  public:
    ViewTestBase() : mContentView(NULL)
    {
    }

  protected:
    PContentView mContentView;

    PContentView gContentView;
    UINT64 gCustomData;
    ViewItem gViewItem;
    BOOL gFrameDropped;
    UINT32 gCallCount;

    /*
     * Callback function
     */
    static VOID removeNotificationCallback(PContentView pContentView, UINT64 customData, PViewItem pViewItem, BOOL consumed);

    STATUS CreateContentView(CONTENT_VIEW_OVERFLOW_POLICY contentViewOverflowStrategy = CONTENT_VIEW_OVERFLOW_POLICY_DROP_TAIL_VIEW_ITEM,
                             UINT32 maxViewItemCount = MAX_VIEW_ITEM_COUNT, UINT64 maxViewBufferDuration = MAX_VIEW_BUFFER_DURATION)
    {
        // Create the content view
        EXPECT_TRUE(STATUS_SUCCEEDED(createContentView(maxViewItemCount, maxViewBufferDuration, removeNotificationCallback, (UINT64) this,
                                                       contentViewOverflowStrategy, &mContentView)));
        return STATUS_SUCCESS;
    };

    virtual void SetUp()
    {
        // Reset the values
        gContentView = NULL;
        gCustomData = 0;
        MEMSET(&gViewItem, 0x00, SIZEOF(ViewItem));
        gFrameDropped = FALSE;
        gCallCount = 0;

        UINT32 logLevel = 0;
        auto logLevelStr = GETENV("AWS_KVS_LOG_LEVEL");
        if (logLevelStr != NULL) {
            assert(STRTOUI32(logLevelStr, NULL, 10, &logLevel) == STATUS_SUCCESS);
            SET_LOGGER_LOG_LEVEL(logLevel);
        }

        DLOGI("\nSetting up test: %s\n", GetTestName());
    };

    virtual void TearDown()
    {
        DLOGI("\nTearing down test: %s\n", GetTestName());

        // Proactively free the allocation
        if (mContentView) {
            freeContentView(mContentView);
            mContentView = NULL;
        }
    };

    PCHAR GetTestName()
    {
        return (PCHAR)::testing::UnitTest::GetInstance()->current_test_info()->test_case_name();
    };

  private:
};
