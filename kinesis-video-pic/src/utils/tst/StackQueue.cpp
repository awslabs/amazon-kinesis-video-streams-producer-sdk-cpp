#include "gtest/gtest.h"
#include <com/amazonaws/kinesis/video/utils/Include.h>

TEST(NegativeInvalidInput, StackQueueCreate)
{
    EXPECT_NE(STATUS_SUCCESS, stackQueueCreate(NULL));
}

TEST(PositiveIdempotentInvalidInput, StackQueueFree)
{
    EXPECT_EQ(STATUS_SUCCESS, stackQueueFree(NULL));
}

TEST(NegativeInvalidInput, StackQueueClear)
{
    EXPECT_NE(STATUS_SUCCESS, stackQueueClear(NULL));
}

TEST(NegativeInvalidInput, StackQueueGetCount)
{
    PStackQueue pStackQueue = (PStackQueue) 1;
    UINT32 count;

    EXPECT_NE(STATUS_SUCCESS, stackQueueGetCount(NULL, &count));
    EXPECT_NE(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, NULL));
}

TEST(NegativeInvalidInput, StackQueueIsEmpty)
{
    PStackQueue pStackQueue = (PStackQueue) 1;
    BOOL isEmpty;

    EXPECT_NE(STATUS_SUCCESS, stackQueueIsEmpty(NULL, &isEmpty));
    EXPECT_NE(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, NULL));
}

TEST(NegativeInvalidInput, StackQueuePushEnqueue)
{
    EXPECT_NE(STATUS_SUCCESS, stackQueuePush(NULL, 0));
    EXPECT_NE(STATUS_SUCCESS, stackQueueEnqueue(NULL, 0));
}

TEST(NegativeInvalidInput, StackQueuePopPeekDequeue)
{
    PStackQueue pStackQueue;
    UINT64 item;

    EXPECT_EQ(STATUS_SUCCESS, stackQueueCreate(&pStackQueue));

    EXPECT_NE(STATUS_SUCCESS, stackQueuePop(NULL, &item));
    EXPECT_NE(STATUS_SUCCESS, stackQueuePop(pStackQueue, NULL));

    EXPECT_NE(STATUS_SUCCESS, stackQueuePeek(NULL, &item));
    EXPECT_NE(STATUS_SUCCESS, stackQueuePeek(pStackQueue, NULL));

    EXPECT_NE(STATUS_SUCCESS, stackQueueDequeue(NULL, &item));
    EXPECT_NE(STATUS_SUCCESS, stackQueueDequeue(pStackQueue, NULL));
}

TEST(FunctionalTest, StackQueueBasicOperations)
{
    PStackQueue pStackQueue;
    UINT64 count = 100;
    UINT64 data;
    UINT32 retCount;
    BOOL isEmpty;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, stackQueueCreate(&pStackQueue));

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, &isEmpty));
    EXPECT_EQ(0, retCount);
    EXPECT_TRUE(isEmpty);

    // Push
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, stackQueuePush(pStackQueue, i));
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, &isEmpty));
    EXPECT_EQ(count, (UINT64) retCount);
    EXPECT_FALSE(isEmpty);

    for (UINT64 i = 0; i < count; i++) {
        // Peek first
        EXPECT_EQ(STATUS_SUCCESS, stackQueuePeek(pStackQueue, &data));
        EXPECT_EQ(count - i - 1, data);
        EXPECT_EQ(STATUS_SUCCESS, stackQueuePop(pStackQueue, &data));
        EXPECT_EQ(count - i - 1, data);
        EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
        EXPECT_EQ(count - i - 1, (UINT64) retCount);
    }

    EXPECT_EQ(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, &isEmpty));
    EXPECT_TRUE(isEmpty);

    // Enqueue
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, stackQueueEnqueue(pStackQueue, i));
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, &isEmpty));
    EXPECT_EQ(count, (UINT64) retCount);
    EXPECT_FALSE(isEmpty);

    for (UINT64 i = 0; i < count; i++) {
        // Dequeue
        EXPECT_EQ(STATUS_SUCCESS, stackQueueDequeue(pStackQueue, &data));
        EXPECT_EQ(i, data);
        EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
        EXPECT_EQ(count - i - 1, (UINT64) retCount);
    }

    EXPECT_EQ(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, &isEmpty));
    EXPECT_TRUE(isEmpty);


    // Enqueue/push
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, stackQueueEnqueue(pStackQueue, i));
        EXPECT_EQ(STATUS_SUCCESS, stackQueuePush(pStackQueue, i));
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, &isEmpty));
    EXPECT_EQ(count * 2, (UINT64) retCount);
    EXPECT_FALSE(isEmpty);

    // Clear the data
    EXPECT_EQ(STATUS_SUCCESS, stackQueueClear(pStackQueue));

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, &isEmpty));
    EXPECT_EQ(0, retCount);
    EXPECT_TRUE(isEmpty);

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, stackQueueFree(pStackQueue));
}
