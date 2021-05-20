#include "UtilTestFixture.h"

class ThreadFunctionalityTest : public UtilTestBase {
};

#define TEST_THREAD_COUNT       500

UINT64 gThreadCurrentCount = 0;
UINT64 gThreadSleepTimes[TEST_THREAD_COUNT];
BOOL gThreadVisited[TEST_THREAD_COUNT];
BOOL gThreadCleared[TEST_THREAD_COUNT];
MUTEX gThreadMutex;

PVOID testThreadRoutine(PVOID arg)
{
    MUTEX_LOCK(gThreadMutex);
    gThreadCurrentCount++;
    MUTEX_UNLOCK(gThreadMutex);

    UINT64 index = (UINT64) arg;

    // Mark as visited
    gThreadVisited[index] = TRUE;

    // Just sleep for some time
    THREAD_SLEEP(gThreadSleepTimes[index]);

    // Mark as cleared
    gThreadCleared[index] = TRUE;

    MUTEX_LOCK(gThreadMutex);
    gThreadCurrentCount--;
    MUTEX_UNLOCK(gThreadMutex);

    return NULL;
}

TEST_F(ThreadFunctionalityTest, ThreadCreateAndReleaseSimpleCheck)
{
    UINT64 index;
    TID threads[TEST_THREAD_COUNT];
    gThreadMutex = MUTEX_CREATE(FALSE);

    // Create the threads
    for (index = 0; index < TEST_THREAD_COUNT; index++) {
        gThreadVisited[index] = FALSE;
        gThreadCleared[index] = FALSE;
        gThreadSleepTimes[index] = index * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
        EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&threads[index], testThreadRoutine, (PVOID)index));
    }

    // Await for the threads to finish
    for (index = 0; index < TEST_THREAD_COUNT; index++) {
        EXPECT_EQ(STATUS_SUCCESS, THREAD_JOIN(threads[index], NULL));
    }

    EXPECT_EQ(0, gThreadCurrentCount);

    for (index = 0; index < TEST_THREAD_COUNT; index++) {
        EXPECT_TRUE(gThreadVisited[index]) << "Thread didn't visit index " << index;
        EXPECT_TRUE(gThreadCleared[index]) << "Thread didn't clear index " << index;
    }

    MUTEX_FREE(gThreadMutex);
}

TEST_F(ThreadFunctionalityTest, ThreadCreateAndCancel)
{
    UINT64 index;
    TID threads[TEST_THREAD_COUNT];
    gThreadMutex = MUTEX_CREATE(FALSE);

    // Create the threads
    for (index = 0; index < TEST_THREAD_COUNT; index++) {
        gThreadVisited[index] = FALSE;
        gThreadCleared[index] = FALSE;
        // Long sleep
        gThreadSleepTimes[index] = 20 * HUNDREDS_OF_NANOS_IN_A_SECOND;
        EXPECT_EQ(STATUS_SUCCESS, THREAD_CREATE(&threads[index], testThreadRoutine, (PVOID)index));
#if !(defined _WIN32 || defined _WIN64 || defined __CYGWIN__)
        // We should detach thread for non-windows platforms only
        // Windows implementation would cancel the handle and the
        // consequent thread cancel would fail
        EXPECT_EQ(STATUS_SUCCESS, THREAD_DETACH(threads[index]));
#endif
    }

    // wait 2 seconds to make sure all threads are executed
    THREAD_SLEEP(2 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    // Cancel all the threads
    for (index = 0; index < TEST_THREAD_COUNT; index++) {
        EXPECT_EQ(STATUS_SUCCESS, THREAD_CANCEL(threads[index]));
    }

    // Validate that threads have been killed and didn't finish successfully
    EXPECT_EQ(TEST_THREAD_COUNT, gThreadCurrentCount);

    for (index = 0; index < TEST_THREAD_COUNT; index++) {
        EXPECT_TRUE(gThreadVisited[index]) << "Thread didn't visit index " << index;
        EXPECT_FALSE(gThreadCleared[index]) << "Thread shouldn't have cleared index " << index;
    }

    gThreadCurrentCount = 0;
    MUTEX_FREE(gThreadMutex);
}
