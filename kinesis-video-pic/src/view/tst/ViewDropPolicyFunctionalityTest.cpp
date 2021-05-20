#include "ViewTestFixture.h"

using ::testing::WithParamInterface;
using ::testing::Bool;
using ::testing::Values;
using ::testing::Combine;

class ViewDropPolicyFunctionalityTest : public ViewTestBase,
                                        public WithParamInterface< ::std::tuple<CONTENT_VIEW_OVERFLOW_POLICY> >{

    protected:
    CONTENT_VIEW_OVERFLOW_POLICY mOverflowPolicy;
    void SetUp() {
        ViewTestBase::SetUp();
        std::tie(mOverflowPolicy) = GetParam();
    }
};

TEST_P(ViewDropPolicyFunctionalityTest, retainedItemTrimedByContentViewTrimTail)
{
    const UINT64 KEY_FRAME_INTERVAL = 25;
    const UINT64 FPS = 25;
    const UINT64 TEST_VIEW_ITEM_DURATION = HUNDREDS_OF_NANOS_IN_A_SECOND / FPS;
    const UINT64 TEST_MAX_BUFFER_DURATION = 3 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    const UINT64 FIRST_FRAME_ALLOCATION_HANDLE = 1;
    UINT64 timestamp = 0;
    UINT32 i;
    PViewItem pViewItem = NULL;
    ALLOCATION_HANDLE allocHandle = INVALID_ALLOCATION_HANDLE_VALUE;

    // buffer duration will run out first
    CreateContentView(mOverflowPolicy, 10000, TEST_MAX_BUFFER_DURATION);

    for(i = 0; i < 3 * FPS; ++i) {
        allocHandle = i == 0 ? FIRST_FRAME_ALLOCATION_HANDLE : INVALID_ALLOCATION_HANDLE_VALUE;
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView,
                                                     timestamp,
                                                     timestamp,
                                                     TEST_VIEW_ITEM_DURATION,
                                                     allocHandle,
                                                     0,
                                                     VIEW_ITEM_ALLOCAITON_SIZE,
                                                     i % KEY_FRAME_INTERVAL == 0 ? ITEM_FLAG_FRAGMENT_START : ITEM_FLAG_NONE));

        if (i == 0) {
            // get next view item, simulating upper layer start consuming first view item, and it will be retained
            EXPECT_EQ(STATUS_SUCCESS, contentViewGetNext(mContentView, &pViewItem));
        }

        timestamp += TEST_VIEW_ITEM_DURATION;
    }

    // content view should be full now, no frame should be dropped yet.
    EXPECT_EQ(0, gCallCount);

    // should trigger a fragment getting dropped
    EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView,
                                                 timestamp,
                                                 timestamp,
                                                 TEST_VIEW_ITEM_DURATION,
                                                 INVALID_ALLOCATION_HANDLE_VALUE,
                                                 0,
                                                 VIEW_ITEM_ALLOCAITON_SIZE,
                                                 i % KEY_FRAME_INTERVAL == 0 ? ITEM_FLAG_FRAGMENT_START : ITEM_FLAG_NONE));
    // more than one frame dropped
    EXPECT_GT(gCallCount, 0);

    EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pViewItem));
    // first view item is retained because it's the currently streaming item
    EXPECT_EQ(FIRST_FRAME_ALLOCATION_HANDLE, pViewItem->handle);

    // get first frame of third fragment
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, 2 * FPS * TEST_VIEW_ITEM_DURATION, TRUE, &pViewItem));
    EXPECT_EQ(STATUS_SUCCESS, contentViewTrimTail(mContentView, pViewItem->index));

    EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pViewItem));
    // first view item is dropped
    EXPECT_NE(FIRST_FRAME_ALLOCATION_HANDLE, pViewItem->handle);
}

TEST_P(ViewDropPolicyFunctionalityTest, retainedItemTrimedByDropFrameAfterConsumed)
{
    const UINT64 KEY_FRAME_INTERVAL = 25;
    const UINT64 FPS = 25;
    const UINT64 TEST_VIEW_ITEM_DURATION = HUNDREDS_OF_NANOS_IN_A_SECOND / FPS;
    const UINT64 TEST_MAX_BUFFER_DURATION = 3 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    const UINT64 FIRST_FRAME_ALLOCATION_HANDLE = 1;
    UINT64 timestamp = 0;
    UINT32 i;
    PViewItem pViewItem = NULL;
    ALLOCATION_HANDLE allocHandle = INVALID_ALLOCATION_HANDLE_VALUE;

    // buffer duration will run out first
    CreateContentView(mOverflowPolicy, 10000, TEST_MAX_BUFFER_DURATION);

    for(i = 0; i < 3 * FPS; ++i) {
        allocHandle = i == 0 ? FIRST_FRAME_ALLOCATION_HANDLE : INVALID_ALLOCATION_HANDLE_VALUE;
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView,
                                                     timestamp,
                                                     timestamp,
                                                     TEST_VIEW_ITEM_DURATION,
                                                     allocHandle,
                                                     0,
                                                     VIEW_ITEM_ALLOCAITON_SIZE,
                                                     i % KEY_FRAME_INTERVAL == 0 ? ITEM_FLAG_FRAGMENT_START : ITEM_FLAG_NONE));
        if (i == 0) {
            // get next view item, simulating upper layer start consuming first view item, and it will be retained
            EXPECT_EQ(STATUS_SUCCESS, contentViewGetNext(mContentView, &pViewItem));
        }

        timestamp += TEST_VIEW_ITEM_DURATION;
    }

    // content view should be full now, no frame should be dropped yet.
    EXPECT_EQ(0, gCallCount);

    // should trigger a fragment getting dropped
    EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView,
                                                 timestamp,
                                                 timestamp,
                                                 TEST_VIEW_ITEM_DURATION,
                                                 INVALID_ALLOCATION_HANDLE_VALUE,
                                                 0,
                                                 VIEW_ITEM_ALLOCAITON_SIZE,
                                                 i % KEY_FRAME_INTERVAL == 0 ? ITEM_FLAG_FRAGMENT_START : ITEM_FLAG_NONE));
    timestamp += TEST_VIEW_ITEM_DURATION;

    // more than one frame dropped
    EXPECT_GT(gCallCount, 0);

    EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pViewItem));
    // first view item is retained because it's the current streaming item
    EXPECT_EQ(FIRST_FRAME_ALLOCATION_HANDLE, pViewItem->handle);

    // get next view item, simulating upper layer start consuming first view item
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetNext(mContentView, &pViewItem));
    // get next view item, simulating upper layer finished consuming first view item
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetNext(mContentView, &pViewItem));

    // reset drop frame count
    gCallCount = 0;

    // put 2 more fragment. should cause frame drop
    for(i = 0; i < 2 * FPS; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView,
                                                     timestamp,
                                                     timestamp,
                                                     TEST_VIEW_ITEM_DURATION,
                                                     INVALID_ALLOCATION_HANDLE_VALUE,
                                                     0,
                                                     VIEW_ITEM_ALLOCAITON_SIZE,
                                                     i % KEY_FRAME_INTERVAL == 0 ? ITEM_FLAG_FRAGMENT_START : ITEM_FLAG_NONE));

        timestamp += TEST_VIEW_ITEM_DURATION;
    }

    // more than one frame dropped
    EXPECT_GT(gCallCount, 0);

    EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pViewItem));
    // first view item is dropped
    EXPECT_NE(FIRST_FRAME_ALLOCATION_HANDLE, pViewItem->handle);
}

TEST_P(ViewDropPolicyFunctionalityTest, retainingViewItemWhileDroppingStillReduceWindowsDuration)
{
    const UINT64 KEY_FRAME_INTERVAL = 25;
    const UINT64 FPS = 25;
    const UINT64 TEST_VIEW_ITEM_DURATION = HUNDREDS_OF_NANOS_IN_A_SECOND / FPS;
    const UINT64 TEST_MAX_BUFFER_DURATION = 3 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    const UINT64 FIRST_FRAME_ALLOCATION_HANDLE = 1;
    UINT64 timestamp = 0;
    UINT32 i;
    UINT64 windowDurationBeforeDrop, windowDurationAfterDrop, currentDuration;
    PViewItem pViewItem = NULL;

    // buffer duration will run out first
    CreateContentView(mOverflowPolicy, 10000, TEST_MAX_BUFFER_DURATION);

    for(i = 0; i < 3 * FPS; ++i) {
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView,
                                                     timestamp,
                                                     timestamp,
                                                     TEST_VIEW_ITEM_DURATION,
                                                     INVALID_ALLOCATION_HANDLE_VALUE,
                                                     0,
                                                     VIEW_ITEM_ALLOCAITON_SIZE,
                                                     i % KEY_FRAME_INTERVAL == 0 ? ITEM_FLAG_FRAGMENT_START : ITEM_FLAG_NONE));

        if (i == 0) {
            // get next view item, simulating upper layer start consuming first view item, and it will be retained
            EXPECT_EQ(STATUS_SUCCESS, contentViewGetNext(mContentView, &pViewItem));
        }

        timestamp += TEST_VIEW_ITEM_DURATION;
    }

    // content view should be full now, no frame should be dropped yet.
    EXPECT_EQ(0, gCallCount);
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetWindowDuration(mContentView, &currentDuration, &windowDurationBeforeDrop));


    // should trigger a fragment getting dropped
    EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView,
                                                 timestamp,
                                                 timestamp,
                                                 TEST_VIEW_ITEM_DURATION,
                                                 INVALID_ALLOCATION_HANDLE_VALUE,
                                                 0,
                                                 VIEW_ITEM_ALLOCAITON_SIZE,
                                                 i % KEY_FRAME_INTERVAL == 0 ? ITEM_FLAG_FRAGMENT_START : ITEM_FLAG_NONE));
    timestamp += TEST_VIEW_ITEM_DURATION;

    // more than one frame dropped
    EXPECT_GT(gCallCount, 0);
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetWindowDuration(mContentView, &currentDuration, &windowDurationAfterDrop));
    EXPECT_LE(windowDurationAfterDrop, windowDurationBeforeDrop);
}

TEST_P(ViewDropPolicyFunctionalityTest, consumedViewItemStayRetained)
{
    const UINT64 KEY_FRAME_INTERVAL = 25;
    const UINT64 FPS = 25;
    const UINT64 TEST_VIEW_ITEM_DURATION = HUNDREDS_OF_NANOS_IN_A_SECOND / FPS;
    const UINT64 TEST_MAX_BUFFER_DURATION = 2 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    const UINT64 FIRST_FRAME_ALLOCATION_HANDLE = 1;
    UINT64 timestamp = 0;
    UINT32 i = 0;
    PViewItem pViewItem = NULL;
    ALLOCATION_HANDLE allocHandle = INVALID_ALLOCATION_HANDLE_VALUE;

    // buffer duration will run out first
    CreateContentView(mOverflowPolicy, 10000, TEST_MAX_BUFFER_DURATION);

    // keep putting view item until at least 100 view items have been dropped
    while (gCallCount < 100) {
        allocHandle = i == 0 ? FIRST_FRAME_ALLOCATION_HANDLE : INVALID_ALLOCATION_HANDLE_VALUE;
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView,
                                                     timestamp,
                                                     timestamp,
                                                     TEST_VIEW_ITEM_DURATION,
                                                     allocHandle,
                                                     0,
                                                     VIEW_ITEM_ALLOCAITON_SIZE,
                                                     i % KEY_FRAME_INTERVAL == 0 ? ITEM_FLAG_FRAGMENT_START : ITEM_FLAG_NONE));
        if (i == 0) {
            // get next view item, simulating upper layer start consuming first view item, and it will be retained
            EXPECT_EQ(STATUS_SUCCESS, contentViewGetNext(mContentView, &pViewItem));
        }

        timestamp += TEST_VIEW_ITEM_DURATION;
        i++;
    }

    EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pViewItem));
    // first view item is still retained because it's the current streaming item
    EXPECT_EQ(FIRST_FRAME_ALLOCATION_HANDLE, pViewItem->handle);

    // get next view item, simulating upper layer finished consuming first view item
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetNext(mContentView, &pViewItem));

    // reset drop frame count
    gCallCount = 0;

    // put until new drop frame
    while (gCallCount == 0) {
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView,
                                                     timestamp,
                                                     timestamp,
                                                     TEST_VIEW_ITEM_DURATION,
                                                     INVALID_ALLOCATION_HANDLE_VALUE,
                                                     0,
                                                     VIEW_ITEM_ALLOCAITON_SIZE,
                                                     i % KEY_FRAME_INTERVAL == 0 ? ITEM_FLAG_FRAGMENT_START : ITEM_FLAG_NONE));

        timestamp += TEST_VIEW_ITEM_DURATION;
        i++;
    }

    EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pViewItem));
    // first view item is dropped
    EXPECT_NE(FIRST_FRAME_ALLOCATION_HANDLE, pViewItem->handle);
}

INSTANTIATE_TEST_CASE_P(PermutatedDropPolicy, ViewDropPolicyFunctionalityTest,
                        Values(CONTENT_VIEW_OVERFLOW_POLICY_DROP_TAIL_VIEW_ITEM, CONTENT_VIEW_OVERFLOW_POLICY_DROP_UNTIL_FRAGMENT_START));