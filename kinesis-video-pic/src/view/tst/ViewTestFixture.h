#if 0
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

#include "gtest/gtest.h"
#include <com/amazonaws/kinesis/video/utils/Include.h>
#include <com/amazonaws/kinesis/video/view/Include.h>

#include "src/view/src/Include_i.h"

#define MAX_VIEW_ITEM_COUNT                         100
#define MAX_VIEW_BUFFER_DURATION                    1000
#define VIEW_NOTIFICATION_CALLBACK_CUSTOM_DATA      1122

extern PContentView gContentView;
extern UINT64 gCustomData;
extern ViewItem gViewItem;
extern BOOL gCurrent;
extern UINT32 gCallCount;

/**
 * Callback function
 */
VOID removeNotificationCallback(PContentView pContentView, UINT64 customData, PViewItem pViewItem, BOOL consumed);

class ViewTestBase : public ::testing::Test {
public:
    ViewTestBase(): mContentView(NULL) {}

protected:
    PContentView mContentView;

    STATUS CreateContentView()
    {
        // Reset the globals
        gContentView = NULL;
        gCustomData = 0;
        MEMSET(&gViewItem, 0x00, SIZEOF(ViewItem));
        gCurrent = FALSE;
        gCallCount = 0;

        // Create the content view
        EXPECT_TRUE(STATUS_SUCCEEDED(createContentView(MAX_VIEW_ITEM_COUNT, MAX_VIEW_BUFFER_DURATION, removeNotificationCallback, VIEW_NOTIFICATION_CALLBACK_CUSTOM_DATA, &mContentView)));

        return STATUS_SUCCESS;
    };

    virtual void SetUp()
    {
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
        return (PCHAR) ::testing::UnitTest::GetInstance()->current_test_info()->test_case_name();
    };


private:

};
