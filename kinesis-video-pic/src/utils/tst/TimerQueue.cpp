#include "UtilTestFixture.h"

class TimerQueueFunctionalityTest : public UtilTestBase {
public:
    TimerQueueFunctionalityTest() : testTimerId(MAX_UINT32),
                                    invokeCount(0),
                                    cancelAfterCount(MAX_UINT32),
                                    retStatus(STATUS_SUCCESS),
                                    checkTimerId(TRUE),
                                    sleepFor(0)
    {
    }

    volatile SIZE_T testTimerId;
    volatile SIZE_T checkTimerId;
    volatile SIZE_T invokeCount;
    volatile SIZE_T cancelAfterCount;
    volatile SIZE_T retStatus;
    volatile SIZE_T sleepFor;
};

STATUS testTimerCallback(UINT32 timerId, UINT64 scheduledTime, UINT64 customData)
{
    UNUSED_PARAM(scheduledTime);
    STATUS retStatus = STATUS_SUCCESS;
    SIZE_T invokeCount, cancelAfterCount, sleep;
    BOOL checkId;
    UINT32 testTimerId;
    TimerQueueFunctionalityTest* pTest = (TimerQueueFunctionalityTest*) customData;

    CHK(pTest != NULL, STATUS_INVALID_ARG);

    testTimerId = (UINT32) ATOMIC_LOAD(&pTest->testTimerId);
    checkId = (BOOL) ATOMIC_LOAD(&pTest->checkTimerId);
    CHK(!checkId || timerId == testTimerId, STATUS_INVALID_ARG);
    invokeCount = ATOMIC_INCREMENT(&pTest->invokeCount);
    cancelAfterCount = ATOMIC_LOAD(&pTest->cancelAfterCount);
    sleep = ATOMIC_LOAD(&pTest->sleepFor);

    if (sleep != 0) {
        THREAD_SLEEP(sleep);
    }

    // Cancel after specified iterations
    CHK(invokeCount < cancelAfterCount, STATUS_TIMER_QUEUE_STOP_SCHEDULING);

    // Return whatever the test specifies
    retStatus = (STATUS) ATOMIC_LOAD(&pTest->retStatus);

CleanUp:

    if (retStatus != STATUS_TIMER_QUEUE_STOP_SCHEDULING) {
        CHK_LOG_ERR(retStatus);
    }

    return retStatus;
}

STATUS multiUserAddAndCancelTestCallback(UINT32 timerId, UINT64 scheduledTime, UINT64 customData)
{
    UNUSED_PARAM(scheduledTime);
    UNUSED_PARAM(timerId);
    STATUS retStatus = STATUS_SUCCESS;

    PBOOL pCalled = (PBOOL) customData;
    CHK(pCalled != NULL, STATUS_NULL_ARG);

    *pCalled = TRUE;

    retStatus = STATUS_TIMER_QUEUE_STOP_SCHEDULING;

CleanUp:

    return retStatus;
}

TEST_F(TimerQueueFunctionalityTest, createFreeApiTest)
{
    TIMER_QUEUE_HANDLE handle;
    PTimerQueue pTimerQueue;

    EXPECT_NE(STATUS_SUCCESS, timerQueueCreate(NULL));
    EXPECT_NE(STATUS_SUCCESS, timerQueueCreateInternal(0, &pTimerQueue));
    EXPECT_NE(STATUS_SUCCESS, timerQueueCreateInternal(1, NULL));
    EXPECT_NE(STATUS_SUCCESS, timerQueueCreateInternal(0, NULL));
    EXPECT_NE(STATUS_SUCCESS, timerQueueFree(NULL));

    for (UINT32 i = 0; i < 1000; i++) {
        EXPECT_EQ(STATUS_SUCCESS, timerQueueCreate(&handle));
        EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
    }

    // Call again - idempotent
    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
}

TEST_F(TimerQueueFunctionalityTest, addRemoveTimerApiTest)
{
    UINT64 startTime = 1 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    UINT64 period = 200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    UINT64 badPeriod = MIN_TIMER_QUEUE_PERIOD_DURATION - 1;
    TIMER_QUEUE_HANDLE handle = INVALID_TIMER_QUEUE_HANDLE_VALUE;
    UINT32 timerId;

    // Invalid handle value
    EXPECT_NE(STATUS_SUCCESS, timerQueueAddTimer(handle, startTime, period, testTimerCallback, (UINT64) this, &timerId));

    // Create a valid timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCreate(&handle));

    // Invalid timer id
    EXPECT_NE(STATUS_SUCCESS, timerQueueCancelTimer(handle, UINT32_MAX, (UINT64) this));

    // Invalid period
    EXPECT_EQ(STATUS_INVALID_TIMER_PERIOD_VALUE, timerQueueAddTimer(handle, startTime, badPeriod, testTimerCallback, (UINT64) this, &timerId));
    // NULL callback
    EXPECT_NE(STATUS_SUCCESS, timerQueueAddTimer(handle, startTime, period, NULL, (UINT64) this, &timerId));
    // NULL timer id
    EXPECT_NE(STATUS_SUCCESS, timerQueueAddTimer(handle, startTime, period, testTimerCallback, (UINT64) this, NULL));

    // Valid callback
    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, 100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND, period, testTimerCallback, (UINT64) this, &timerId));
    ATOMIC_STORE(&testTimerId, (SIZE_T) timerId);
    THREAD_SLEEP(200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    EXPECT_EQ(1, ATOMIC_LOAD(&invokeCount));

    // Invalid handle
    EXPECT_NE(STATUS_SUCCESS, timerQueueCancelTimer(INVALID_TIMER_QUEUE_HANDLE_VALUE, 0, (UINT64) this));

    // Valid
    timerId = (UINT32) ATOMIC_LOAD(&testTimerId);
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCancelTimer(handle, timerId, (UINT64) this));

    // Valid - idempotent
    timerId = (UINT32) ATOMIC_LOAD(&testTimerId);
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCancelTimer(handle, timerId, (UINT64) this));

    // Free the timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));

    // Call again - idempotent
    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
}

TEST_F(TimerQueueFunctionalityTest, updateTimerPeriodApiTest)
{
    TIMER_QUEUE_HANDLE handle = INVALID_TIMER_QUEUE_HANDLE_VALUE;
    UINT32 timerId = 0;
    UINT64 customData = (UINT64) this;
    UINT64 period = 200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    UINT64 badPeriod = MIN_TIMER_QUEUE_PERIOD_DURATION - 1;

    // Invalid handle value
    EXPECT_NE(STATUS_SUCCESS, timerQueueUpdateTimerPeriod(handle, customData, timerId, period));

    // Create a valid timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCreate(&handle));

    // invalid timer id
    EXPECT_NE(STATUS_SUCCESS, timerQueueUpdateTimerPeriod(handle, customData, UINT32_MAX, period));

    // Valid callback
    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, 0, period, testTimerCallback, customData, &timerId));

    // invalid period
    EXPECT_NE(STATUS_SUCCESS, timerQueueUpdateTimerPeriod(handle, customData, timerId, badPeriod));

    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
}

TEST_F(TimerQueueFunctionalityTest, multipleAddRemoveMax)
{
    UINT64 startTime = 1000 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    UINT64 period = 2000 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    TIMER_QUEUE_HANDLE handle = INVALID_TIMER_QUEUE_HANDLE_VALUE;
    UINT32 timerId, i, middleTimerId = 0;

    // Create a valid timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCreate(&handle));

    for (i = 0; i < DEFAULT_TIMER_QUEUE_TIMER_COUNT; i++) {
        EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, startTime, period,
                                                     testTimerCallback, (UINT64) this, &timerId));

        if (i == DEFAULT_TIMER_QUEUE_TIMER_COUNT / 2) {
            middleTimerId = timerId;
        }
    }

    // We should fail with max timer error
    EXPECT_EQ(STATUS_MAX_TIMER_COUNT_REACHED, timerQueueAddTimer(handle, startTime, period,
                                                                 testTimerCallback, (UINT64) this, &timerId));


    // Remove one and add back again
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCancelTimer(handle, timerId, (UINT64) this));
    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, startTime, period,
                                                 testTimerCallback, (UINT64) this, &timerId));

    // Remove one in the middle and add back again
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCancelTimer(handle, middleTimerId, (UINT64) this));
    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, startTime, period,
                                                 testTimerCallback, (UINT64) this, &timerId));

    // We should fail with max timer error if we add again
    EXPECT_EQ(STATUS_MAX_TIMER_COUNT_REACHED, timerQueueAddTimer(handle, startTime, period,
                                                                 testTimerCallback, (UINT64) this, &timerId));

    THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    EXPECT_EQ(0, ATOMIC_LOAD(&invokeCount));

    // Free the timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
}

TEST_F(TimerQueueFunctionalityTest, addedTimerFiresBeforeOlder)
{
    UINT64 startTime = 100 * HUNDREDS_OF_NANOS_IN_A_SECOND;
    UINT64 period = 1000 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    TIMER_QUEUE_HANDLE handle = INVALID_TIMER_QUEUE_HANDLE_VALUE;
    UINT32 timerId, i, middleTimerId = 0;

    // Create a valid timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCreate(&handle));

    for (i = 0; i < DEFAULT_TIMER_QUEUE_TIMER_COUNT; i++) {
        EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, startTime, period,
                                                     testTimerCallback, (UINT64) this, &timerId));

        if (i == DEFAULT_TIMER_QUEUE_TIMER_COUNT / 2) {
            middleTimerId = timerId;
        }
    }

    // Remove one in the middle and add an earlier firing timer
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCancelTimer(handle, middleTimerId, (UINT64) this));
    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, 100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND, period,
                                                 testTimerCallback, (UINT64) this, &timerId));
    ATOMIC_STORE(&testTimerId, (SIZE_T) timerId);

    THREAD_SLEEP(200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    EXPECT_EQ(1, ATOMIC_LOAD(&invokeCount));

    // Free the timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
}

TEST_F(TimerQueueFunctionalityTest, timerStartTime)
{
    UINT64 startTime = 100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    UINT64 period = 300 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    TIMER_QUEUE_HANDLE handle = INVALID_TIMER_QUEUE_HANDLE_VALUE;
    UINT32 timerId;

    // Create a valid timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCreate(&handle));

    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, startTime, period,
                                                 testTimerCallback, (UINT64) this, &timerId));
    ATOMIC_STORE(&testTimerId, (SIZE_T) timerId);

    THREAD_SLEEP(10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    // Make sure it hasn't fired yet
    EXPECT_EQ(0, ATOMIC_LOAD(&invokeCount));

    THREAD_SLEEP(150 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    // Make sure it has fired on start
    EXPECT_EQ(1, ATOMIC_LOAD(&invokeCount));

    THREAD_SLEEP(300 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    // Make sure it has fired after the period again
    EXPECT_EQ(2, ATOMIC_LOAD(&invokeCount));

    THREAD_SLEEP(300 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    // Make sure it has fired after the period again
    EXPECT_EQ(3, ATOMIC_LOAD(&invokeCount));

    // Free the timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
}

TEST_F(TimerQueueFunctionalityTest, timerInterleavedSamePeriodDifferentStart)
{
    UINT64 startTime1 = 10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    UINT64 startTime2 = 100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    UINT64 period = 400 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    TIMER_QUEUE_HANDLE handle = INVALID_TIMER_QUEUE_HANDLE_VALUE;
    UINT32 timerId1, timerId2;

    // Create a valid timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCreate(&handle));

    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, startTime1, period,
                                                 testTimerCallback, (UINT64) this, &timerId1));

    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, startTime2, period,
                                                 testTimerCallback, (UINT64) this, &timerId2));

    // Nothing should have fired yet
    EXPECT_EQ(0, ATOMIC_LOAD(&invokeCount));

    // Set the first timer - it should fire first
    ATOMIC_STORE(&testTimerId, (SIZE_T) timerId1);
    THREAD_SLEEP(50 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    // Make sure it hasn't fired yet
    EXPECT_EQ(1, ATOMIC_LOAD(&invokeCount));

    // Set to the second as it's next on the start up
    ATOMIC_STORE(&testTimerId, (SIZE_T) timerId2);
    THREAD_SLEEP(150 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    // Make sure it has fired on start
    EXPECT_EQ(2, ATOMIC_LOAD(&invokeCount));

    // First timers period fires next
    ATOMIC_STORE(&testTimerId, (SIZE_T) timerId1);
    THREAD_SLEEP(250 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    // Make sure it has fired after the period again
    EXPECT_EQ(3, ATOMIC_LOAD(&invokeCount));

    // Second timer period fires next
    ATOMIC_STORE(&testTimerId, (SIZE_T) timerId2);
    THREAD_SLEEP(150 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    // Make sure it has fired after the period again
    EXPECT_EQ(4, ATOMIC_LOAD(&invokeCount));

    // Free the timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
}

TEST_F(TimerQueueFunctionalityTest, timerCancelFromCallback)
{
    UINT64 startTime = 50 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    UINT64 period = 200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    TIMER_QUEUE_HANDLE handle = INVALID_TIMER_QUEUE_HANDLE_VALUE;
    UINT32 timerId, count;

    // Make sure we terminate after 3 iterations
    cancelAfterCount = 3;

    // Create a valid timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCreate(&handle));

    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, startTime, period,
                                                 testTimerCallback, (UINT64) this, &timerId));
    ATOMIC_STORE(&testTimerId, (SIZE_T) timerId);

    // Make sure it hasn't fired yet
    EXPECT_EQ(0, ATOMIC_LOAD(&invokeCount));

    // First time fire after start period
    THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    EXPECT_EQ(1, ATOMIC_LOAD(&invokeCount));

    // Starts firing regularly
    THREAD_SLEEP(200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    EXPECT_EQ(2, ATOMIC_LOAD(&invokeCount));
    THREAD_SLEEP(200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    EXPECT_EQ(3, ATOMIC_LOAD(&invokeCount));
    THREAD_SLEEP(200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    EXPECT_EQ(4, ATOMIC_LOAD(&invokeCount));

    // Should be cancelled after the 3rd
    THREAD_SLEEP(500 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    // Make sure it has not fired after the period again
    EXPECT_EQ(4, ATOMIC_LOAD(&invokeCount));

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, timerQueueGetTimerCount(handle, &count));
    EXPECT_EQ(0, count);

    // Free the timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
}

TEST_F(TimerQueueFunctionalityTest, timerCancelDirectly)
{
    UINT64 startTime = 50 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    UINT64 period = 200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    TIMER_QUEUE_HANDLE handle = INVALID_TIMER_QUEUE_HANDLE_VALUE;
    UINT32 timerId, count;

    // Create a valid timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCreate(&handle));

    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, startTime, period,
                                                 testTimerCallback, (UINT64) this, &timerId));
    ATOMIC_STORE(&testTimerId, (SIZE_T) timerId);

    // Make sure it hasn't fired yet
    EXPECT_EQ(0, ATOMIC_LOAD(&invokeCount));
    THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    // Starts firing regularly
    EXPECT_EQ(1, ATOMIC_LOAD(&invokeCount));
    THREAD_SLEEP(200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    EXPECT_EQ(2, ATOMIC_LOAD(&invokeCount));
    THREAD_SLEEP(200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    EXPECT_EQ(3, ATOMIC_LOAD(&invokeCount));

    timerId = (UINT32) ATOMIC_LOAD(&testTimerId);
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCancelTimer(handle, timerId, (UINT64) this));

    // Should be cancelled
    THREAD_SLEEP(300 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    // Make sure it has not fired after the period again
    EXPECT_EQ(3, ATOMIC_LOAD(&invokeCount));

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, timerQueueGetTimerCount(handle, &count));
    EXPECT_EQ(0, count);

    // Free the timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
}

TEST_F(TimerQueueFunctionalityTest, timerSingleInvokeTimer)
{
    UINT64 startTime = 10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    TIMER_QUEUE_HANDLE handle = INVALID_TIMER_QUEUE_HANDLE_VALUE;
    UINT32 timerId, count;

    // Create a valid timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCreate(&handle));

    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, startTime,
            TIMER_QUEUE_SINGLE_INVOCATION_PERIOD,
            testTimerCallback, (UINT64) this, &timerId));
    ATOMIC_STORE(&testTimerId, (SIZE_T) timerId);

    // Make sure it hasn't fired yet
    EXPECT_EQ(0, ATOMIC_LOAD(&invokeCount));
    THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    // Fires once
    EXPECT_EQ(1, ATOMIC_LOAD(&invokeCount));
    THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    EXPECT_EQ(1, ATOMIC_LOAD(&invokeCount));
    THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    EXPECT_EQ(1, ATOMIC_LOAD(&invokeCount));

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, timerQueueGetTimerCount(handle, &count));
    EXPECT_EQ(0, count);

    // Should have already been cancelled
    timerId = (UINT32) ATOMIC_LOAD(&testTimerId);
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCancelTimer(handle, timerId, (UINT64) this));

    // Free the timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
}

TEST_F(TimerQueueFunctionalityTest, timerDoesntCancelOnError)
{
    UINT64 startTime = 10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    UINT64 period = 200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    TIMER_QUEUE_HANDLE handle = INVALID_TIMER_QUEUE_HANDLE_VALUE;
    UINT32 timerId, count;

    // Create a valid timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCreate(&handle));

    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, startTime, period,
                                                 testTimerCallback, (UINT64) this, &timerId));
    ATOMIC_STORE(&testTimerId, (SIZE_T) timerId);

    // Make sure it hasn't fired yet
    EXPECT_EQ(0, ATOMIC_LOAD(&invokeCount));
    THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    // Starts firing regularly
    EXPECT_EQ(1, ATOMIC_LOAD(&invokeCount));
    THREAD_SLEEP(200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    EXPECT_EQ(2, ATOMIC_LOAD(&invokeCount));
    THREAD_SLEEP(200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    EXPECT_EQ(3, ATOMIC_LOAD(&invokeCount));

    // Make sure the timer return status is an error but it doesn't cancel the timer
    ATOMIC_STORE(&retStatus, (SIZE_T) STATUS_INVALID_OPERATION);

    // Should be cancelled
    THREAD_SLEEP(200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    // Make sure it has fired after the period again
    EXPECT_EQ(4, ATOMIC_LOAD(&invokeCount));

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, timerQueueGetTimerCount(handle, &count));
    EXPECT_EQ(1, count);

    // Free the timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
}

TEST_F(TimerQueueFunctionalityTest, timerCancelDirectlyOneLeaveAnotherStart)
{
    UINT64 startTime1 = 10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    UINT64 startTime2 = 500 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    UINT64 period = 100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    TIMER_QUEUE_HANDLE handle = INVALID_TIMER_QUEUE_HANDLE_VALUE;
    UINT32 timerId1, timerId2;

    // Create a valid timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCreate(&handle));

    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, startTime1, period,
                                                 testTimerCallback, (UINT64) this, &timerId1));
    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, startTime2, period,
                                                 testTimerCallback, (UINT64) this, &timerId2));

    // Make sure it hasn't fired yet
    EXPECT_EQ(0, ATOMIC_LOAD(&invokeCount));
    ATOMIC_STORE(&testTimerId, (SIZE_T) timerId1);

    // First after start period fires
    THREAD_SLEEP(50 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    EXPECT_EQ(1, ATOMIC_LOAD(&invokeCount));
    // First starts firing regularly
    THREAD_SLEEP(120 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    EXPECT_EQ(2, ATOMIC_LOAD(&invokeCount));
    THREAD_SLEEP(120 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    EXPECT_EQ(3, ATOMIC_LOAD(&invokeCount));

    UINT32 timerId = (UINT32) ATOMIC_LOAD(&testTimerId);
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCancelTimer(handle, timerId, (UINT64) this));

    // First should be cancelled
    THREAD_SLEEP(150 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    EXPECT_EQ(3, ATOMIC_LOAD(&invokeCount));

    // The second should fire
    ATOMIC_STORE(&testTimerId, (SIZE_T) timerId2);
    THREAD_SLEEP(100 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    EXPECT_EQ(4, ATOMIC_LOAD(&invokeCount));

    // Free the timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
}

TEST_F(TimerQueueFunctionalityTest, miniStressTest)
{
    UINT64 period = 1 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    TIMER_QUEUE_HANDLE handle = INVALID_TIMER_QUEUE_HANDLE_VALUE;
    UINT32 timerIds[DEFAULT_TIMER_QUEUE_TIMER_COUNT], i;

    // Make sure we don't check for the timer in the test callback
    checkTimerId = FALSE;

    // Create a valid timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCreate(&handle));

    // Create timers that will start in a millisecond
    for (i = 0; i < DEFAULT_TIMER_QUEUE_TIMER_COUNT; i++) {
        EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, 0, period,
                                                     testTimerCallback, (UINT64) this, &timerIds[i]));
    }

    // Let it fire for a while
    THREAD_SLEEP(1000 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    // Cancel all the timers
    for (i = 0; i < DEFAULT_TIMER_QUEUE_TIMER_COUNT; i++) {
        EXPECT_EQ(STATUS_SUCCESS, timerQueueCancelTimer(handle, timerIds[i], (UINT64) this));
    }

    // There should be a number of timers * duration in millis invokes but the actual number is timing
    // dependent and will be lower. We will check for half.
    EXPECT_LT(1000 * DEFAULT_TIMER_QUEUE_TIMER_COUNT / 2, ATOMIC_LOAD(&invokeCount));

    // Free the timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
}

TEST_F(TimerQueueFunctionalityTest, multiUserAddAndCancelTest)
{
    volatile BOOL user1Called = FALSE, user2Called = FALSE;
    UINT64 start = 300 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND, period = 10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;;
    TIMER_QUEUE_HANDLE handle = INVALID_TIMER_QUEUE_HANDLE_VALUE;
    UINT32 user1TimerId, user2TimerId;

    // Create a valid timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCreate(&handle));
    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, 0, period,
                                                 multiUserAddAndCancelTestCallback, (UINT64) &user1Called, &user1TimerId));
    THREAD_SLEEP(50 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, start, period,
                                                 multiUserAddAndCancelTestCallback, (UINT64) &user2Called, &user2TimerId));
    THREAD_SLEEP(50 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);
    // user 1 may not be aware that it's timerid has exited and call cancel timer again.
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCancelTimer(handle, user1TimerId, (UINT64) &user1Called));

    THREAD_SLEEP(start + 50 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    // user 2's callback shouldnt get canceled by user 1's cancel call.
    EXPECT_TRUE(user2Called == TRUE);

    // Free the timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
}

TEST_F(TimerQueueFunctionalityTest, cancelTimerWithCustomData)
{
    TIMER_QUEUE_HANDLE handle = INVALID_TIMER_QUEUE_HANDLE_VALUE;
    UINT32 timerId, timerCount = 0;
    UINT64 period = 10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND, start = 300 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    BOOL user1called = FALSE, user2called = FALSE;

    // Create a valid timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCreate(&handle));

    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, start, period,
                                                 multiUserAddAndCancelTestCallback, (UINT64) &user1called, &timerId));
    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, start, period,
                                                 multiUserAddAndCancelTestCallback, (UINT64) &user1called, &timerId));
    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, start, period,
                                                 multiUserAddAndCancelTestCallback, (UINT64) &user2called, &timerId));
    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, start, period,
                                                 multiUserAddAndCancelTestCallback, (UINT64) &user2called, &timerId));

    EXPECT_EQ(STATUS_SUCCESS, timerQueueCancelTimersWithCustomData(handle, 3));
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCancelTimersWithCustomData(handle, (UINT64) NULL));
    EXPECT_EQ(STATUS_SUCCESS, timerQueueGetTimerCount(handle, &timerCount));
    EXPECT_EQ(4, timerCount); // nothing got canceled

    EXPECT_EQ(STATUS_SUCCESS, timerQueueCancelTimersWithCustomData(handle, (UINT64) &user2called));

    THREAD_SLEEP(start + 50 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    EXPECT_EQ(STATUS_SUCCESS, timerQueueGetTimerCount(handle, &timerCount));
    EXPECT_EQ(0, timerCount);
    EXPECT_TRUE(user1called == TRUE);
    EXPECT_TRUE(user2called == FALSE);

    // Free the timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
}

TEST_F(TimerQueueFunctionalityTest, cancelAllTimer)
{
    TIMER_QUEUE_HANDLE handle = INVALID_TIMER_QUEUE_HANDLE_VALUE;
    UINT32 timerId, timerCount = 0;
    UINT64 period = 10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND, start = 300 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    BOOL user1called = FALSE, user2called = FALSE;

    // Create a valid timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCreate(&handle));

    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, start, period,
                                                 multiUserAddAndCancelTestCallback, (UINT64) &user1called, &timerId));
    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, start, period,
                                                 multiUserAddAndCancelTestCallback, (UINT64) &user1called, &timerId));
    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, start, period,
                                                 multiUserAddAndCancelTestCallback, (UINT64) &user2called, &timerId));
    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, start, period,
                                                 multiUserAddAndCancelTestCallback, (UINT64) &user2called, &timerId));

    EXPECT_EQ(STATUS_SUCCESS, timerQueueCancelAllTimers(handle));
    EXPECT_EQ(STATUS_SUCCESS, timerQueueGetTimerCount(handle, &timerCount));
    EXPECT_EQ(0, timerCount);

    THREAD_SLEEP(start + 50 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    EXPECT_EQ(STATUS_SUCCESS, timerQueueGetTimerCount(handle, &timerCount));
    EXPECT_EQ(0, timerCount);
    EXPECT_TRUE(user1called == FALSE);
    EXPECT_TRUE(user2called == FALSE);

    // Free the timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
}

TEST_F(TimerQueueFunctionalityTest, testGetTimersWithCustomData)
{
    TIMER_QUEUE_HANDLE handle = INVALID_TIMER_QUEUE_HANDLE_VALUE;
    UINT32 timerId;
    UINT64 period = 10 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND, start = 300 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    BOOL user1called = FALSE, user2called = FALSE;
    UINT32 timerIds[40];
    UINT32 timerCount = ARRAY_SIZE(timerIds);

    // Create a valid timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCreate(&handle));

    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, start, period,
                                                 multiUserAddAndCancelTestCallback, (UINT64) &user1called, &timerId));
    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, start, period,
                                                 multiUserAddAndCancelTestCallback, (UINT64) &user1called, &timerId));
    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, start, period,
                                                 multiUserAddAndCancelTestCallback, (UINT64) &user2called, &timerId));
    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, start, period,
                                                 multiUserAddAndCancelTestCallback, (UINT64) &user2called, &timerId));

    EXPECT_EQ(STATUS_SUCCESS, timerQueueGetTimersWithCustomData(handle, (UINT64) &user1called, &timerCount, timerIds));
    EXPECT_EQ(2, timerCount);

    EXPECT_EQ(STATUS_SUCCESS, timerQueueCancelTimer(handle, timerIds[0], (UINT64) &user1called));
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCancelTimer(handle, timerIds[1], (UINT64) &user1called));

    THREAD_SLEEP(start + 50 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    EXPECT_EQ(STATUS_SUCCESS, timerQueueGetTimerCount(handle, &timerCount));
    EXPECT_EQ(0, timerCount);

    // user1's timers should've been cancelled
    EXPECT_TRUE(user1called == FALSE);
    EXPECT_TRUE(user2called == TRUE);

    // Free the timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
}

TEST_F(TimerQueueFunctionalityTest, updateTimerPeriodFunctionalityTest)
{
    TIMER_QUEUE_HANDLE handle = INVALID_TIMER_QUEUE_HANDLE_VALUE;
    UINT32 timerId, oldInvocationCount = 0, newInvocationCount = 0;
    UINT64 customData = (UINT64) this;

    cancelAfterCount = 4;

    // Make sure we don't check for the timer in the test callback
    checkTimerId = FALSE;

    // Create a valid timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCreate(&handle));

    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, 0, 200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND, testTimerCallback, (UINT64) this, &timerId));

    THREAD_SLEEP(HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    EXPECT_EQ(1, ATOMIC_LOAD(&invokeCount));

    EXPECT_EQ(STATUS_SUCCESS, timerQueueUpdateTimerPeriod(handle, customData, timerId, 400 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND));

    oldInvocationCount = (UINT32) ATOMIC_LOAD(&invokeCount);

    THREAD_SLEEP(500 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    newInvocationCount = (UINT32) ATOMIC_LOAD(&invokeCount);

    // invoke count should only differ by 1 because of the new period
    EXPECT_TRUE(newInvocationCount > oldInvocationCount && oldInvocationCount + 1 == newInvocationCount);

    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
}

TEST_F(TimerQueueFunctionalityTest, shutdownTimerQueueTest)
{
    TIMER_QUEUE_HANDLE handle = INVALID_TIMER_QUEUE_HANDLE_VALUE;
    UINT32 timerId;
    UINT64 curTime;

    // Make sure we don't check for the timer in the test callback
    checkTimerId = FALSE;

    // Create a valid timer queue
    EXPECT_EQ(STATUS_SUCCESS, timerQueueCreate(&handle));

    EXPECT_EQ(STATUS_SUCCESS, timerQueueAddTimer(handle, 0, 200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND, testTimerCallback, (UINT64) this, &timerId));

    THREAD_SLEEP(50 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    EXPECT_EQ(1, ATOMIC_LOAD(&invokeCount));

    // Call a shutdown with invalid param
    EXPECT_NE(STATUS_SUCCESS, timerQueueShutdown(INVALID_TIMER_QUEUE_HANDLE_VALUE));

    // Wait for some time for the timer to fire again
    THREAD_SLEEP(250 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    EXPECT_EQ(2, ATOMIC_LOAD(&invokeCount));

    // Make it sleep next time it fires
    curTime = GETTIME();
    ATOMIC_STORE(&sleepFor, 3 * HUNDREDS_OF_NANOS_IN_A_SECOND);

    // Wait for the next firing
    THREAD_SLEEP(250 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND);

    // We should now block until the timer thread returns
    EXPECT_EQ(STATUS_SUCCESS, timerQueueShutdown(handle));

    // Should have at least 3 seconds passed
    EXPECT_LE(curTime + 3 * HUNDREDS_OF_NANOS_IN_A_SECOND, GETTIME());

    // Ensure we can't add a new timer
    EXPECT_EQ(STATUS_TIMER_QUEUE_SHUTDOWN, timerQueueAddTimer(handle, 0, 200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND, testTimerCallback, (UINT64) this, &timerId));

    // Calling again has no effect
    EXPECT_EQ(STATUS_SUCCESS, timerQueueShutdown(handle));

    EXPECT_EQ(STATUS_SUCCESS, timerQueueFree(&handle));
}
