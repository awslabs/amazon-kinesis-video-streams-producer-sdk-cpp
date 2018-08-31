#include "UtilTestFixture.h"

class ThreadFunctionalityTest : public UtilTestBase {
};

#define TEST_THREAD_COUNT       500

UINT64 gThreadCurrentCount = 0;
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
    THREAD_SLEEP(index * 1000);

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
    for (index = 0; index < TEST_THREAD_COUNT; index++) {
        gThreadVisited[index] = FALSE;
        gThreadCleared[index] = FALSE;
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
