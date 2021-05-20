#include "UtilTestFixture.h"

class StackQueueFunctionalityTest : public UtilTestBase {
};


TEST_F(StackQueueFunctionalityTest, NegativeInvalidInput_StackQueueCreate)
{
    EXPECT_NE(STATUS_SUCCESS, stackQueueCreate(NULL));
}

TEST_F(StackQueueFunctionalityTest, PositiveIdempotentInvalidInput_StackQueueFree)
{
    EXPECT_EQ(STATUS_SUCCESS, stackQueueFree(NULL));
}

TEST_F(StackQueueFunctionalityTest, NegativeInvalidInput_StackQueueClear)
{
    EXPECT_NE(STATUS_SUCCESS, stackQueueClear(NULL, FALSE));
}

TEST_F(StackQueueFunctionalityTest, NegativeInvalidInput_StackQueueGetCount)
{
    PStackQueue pStackQueue = (PStackQueue) 1;
    UINT32 count;

    EXPECT_NE(STATUS_SUCCESS, stackQueueGetCount(NULL, &count));
    EXPECT_NE(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, NULL));
}

TEST_F(StackQueueFunctionalityTest, NegativeInvalidInput_StackQueueIsEmpty)
{
    PStackQueue pStackQueue = (PStackQueue) 1;
    BOOL isEmpty;

    EXPECT_NE(STATUS_SUCCESS, stackQueueIsEmpty(NULL, &isEmpty));
    EXPECT_NE(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, NULL));
}

TEST_F(StackQueueFunctionalityTest, NegativeInvalidInput_StackQueuePushEnqueue)
{
    EXPECT_NE(STATUS_SUCCESS, stackQueuePush(NULL, 0));
    EXPECT_NE(STATUS_SUCCESS, stackQueueEnqueue(NULL, 0));
}

TEST_F(StackQueueFunctionalityTest, NegativeInvalidInput_StackQueuePopPeekDequeue)
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

    EXPECT_EQ(STATUS_SUCCESS, stackQueueFree(pStackQueue));
}

TEST_F(StackQueueFunctionalityTest, StackQueueBasicOperations)
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
    EXPECT_EQ(STATUS_SUCCESS, stackQueueClear(pStackQueue, FALSE));

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, &isEmpty));
    EXPECT_EQ(0, retCount);
    EXPECT_TRUE(isEmpty);

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, stackQueueFree(pStackQueue));
}

TEST_F(StackQueueFunctionalityTest, StackQueuePutGetRemoveOperations)
{
    PStackQueue pStackQueue;
    UINT64 count = 100;
    UINT64 data;
    UINT32 retCount, index;
    BOOL isEmpty;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, stackQueueCreate(&pStackQueue));

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, &isEmpty));
    EXPECT_EQ(0, retCount);
    EXPECT_TRUE(isEmpty);

    EXPECT_NE(STATUS_SUCCESS, stackQueueGetAt(pStackQueue, 0, &data));
    EXPECT_NE(STATUS_SUCCESS, stackQueueGetIndexOf(pStackQueue, 0, NULL));
    EXPECT_EQ(STATUS_NOT_FOUND, stackQueueGetIndexOf(pStackQueue, 0, &index));

    // Push
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, stackQueuePush(pStackQueue, i));
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, &isEmpty));
    EXPECT_EQ(count, (UINT64) retCount);
    EXPECT_FALSE(isEmpty);

    // Get at
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, stackQueueGetAt(pStackQueue, (UINT32)i, &data));
        EXPECT_EQ(count - i - 1, data);
        EXPECT_EQ(STATUS_SUCCESS, stackQueueGetIndexOf(pStackQueue, data, &index));
        EXPECT_EQ(i, index);
    }

    // Remove at from the middle
    for (UINT64 i = 0; i < count / 2; i++) {
        // Peek first
        EXPECT_EQ(STATUS_SUCCESS, stackQueueRemoveAt(pStackQueue, (UINT32)(count / 2)));
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, &isEmpty));
    EXPECT_EQ(count / 2, retCount);
    EXPECT_FALSE(isEmpty);

    // Validate the half
    for (UINT64 i = 0; i < count / 2; i++) {
        EXPECT_EQ(STATUS_SUCCESS, stackQueueGetAt(pStackQueue, (UINT32)i, &data));
        EXPECT_EQ(count - i - 1, data);
    }

    // Remove at the rest from the beginning
    for (UINT64 i = 0; i < count / 2; i++) {
        // Peek first
        EXPECT_EQ(STATUS_SUCCESS, stackQueueRemoveAt(pStackQueue, 0));
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, &isEmpty));
    EXPECT_EQ(0, retCount);
    EXPECT_TRUE(isEmpty);

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, stackQueueFree(pStackQueue));
}

TEST_F(StackQueueFunctionalityTest, StackQueuePutGetSetOperations)
{
    PStackQueue pStackQueue;
    UINT64 count = 100;
    UINT64 data;
    UINT32 retCount, index;
    BOOL isEmpty;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, stackQueueCreate(&pStackQueue));

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, &isEmpty));
    EXPECT_EQ(0, retCount);
    EXPECT_TRUE(isEmpty);

    EXPECT_NE(STATUS_SUCCESS, stackQueueGetAt(pStackQueue, 0, &data));
    EXPECT_NE(STATUS_SUCCESS, stackQueueGetIndexOf(pStackQueue, 0, NULL));
    EXPECT_EQ(STATUS_NOT_FOUND, stackQueueGetIndexOf(pStackQueue, 0, &index));

    // Push
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, stackQueuePush(pStackQueue, i));
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, &isEmpty));
    EXPECT_EQ(count, (UINT64) retCount);
    EXPECT_FALSE(isEmpty);

    // Get at
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, stackQueueGetAt(pStackQueue, (UINT32)i, &data));
        EXPECT_EQ(count - i - 1, data);
        EXPECT_EQ(STATUS_SUCCESS, stackQueueGetIndexOf(pStackQueue, data, &index));
        EXPECT_EQ(i, index);
    }

    // Set at
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, stackQueueSetAt(pStackQueue, (UINT32)i, 1000 + i));
    }

    // Get at - verify
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, stackQueueGetAt(pStackQueue, (UINT32)i, &data));
        EXPECT_EQ(i + 1000, data);
        EXPECT_EQ(STATUS_SUCCESS, stackQueueGetIndexOf(pStackQueue, data, &index));
        EXPECT_EQ(i, index);
    }

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, stackQueueFree(pStackQueue));
}

TEST_F(StackQueueFunctionalityTest, StackQueuePutRemoveOperations)
{
    PStackQueue pStackQueue;
    UINT64 count = 100;
    UINT32 retCount;
    BOOL isEmpty;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, stackQueueCreate(&pStackQueue));

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, &isEmpty));
    EXPECT_EQ(0, retCount);
    EXPECT_TRUE(isEmpty);

    EXPECT_NE(STATUS_SUCCESS, stackQueueRemoveItem(NULL, 10));
    EXPECT_EQ(STATUS_NOT_FOUND, stackQueueRemoveItem(pStackQueue, 10));

    // Push
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, stackQueuePush(pStackQueue, i));
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, &isEmpty));
    EXPECT_EQ(count, (UINT64) retCount);
    EXPECT_FALSE(isEmpty);

    // Remove items
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, stackQueueRemoveItem(pStackQueue, i));
        EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
        EXPECT_EQ(count - i - 1, retCount);
    }

    EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
    EXPECT_EQ(0, retCount);

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, stackQueueFree(pStackQueue));
}

TEST_F(StackQueueFunctionalityTest, StackQueueIteratorOperations)
{
    PStackQueue pStackQueue;
    UINT64 count = 100, i = 0;
    UINT64 data;
    UINT32 retCount, index;
    BOOL isEmpty;
    StackQueueIterator iterator;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, stackQueueCreate(&pStackQueue));

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, &isEmpty));
    EXPECT_EQ(0, retCount);
    EXPECT_TRUE(isEmpty);

    EXPECT_NE(STATUS_SUCCESS, stackQueueGetAt(pStackQueue, 0, &data));
    EXPECT_NE(STATUS_SUCCESS, stackQueueGetIndexOf(pStackQueue, 0, NULL));
    EXPECT_EQ(STATUS_NOT_FOUND, stackQueueGetIndexOf(pStackQueue, 0, &index));

    // Positive and negative cases
    EXPECT_EQ(STATUS_SUCCESS, stackQueueGetIterator(pStackQueue, &iterator));
    EXPECT_NE(STATUS_SUCCESS, stackQueueGetIterator(NULL, &iterator));
    EXPECT_NE(STATUS_SUCCESS, stackQueueGetIterator(pStackQueue, NULL));

    EXPECT_EQ(STATUS_NOT_FOUND, stackQueueIteratorNext(&iterator));
    EXPECT_EQ(STATUS_NULL_ARG, stackQueueIteratorNext(NULL));

    EXPECT_EQ(STATUS_NOT_FOUND, stackQueueIteratorGetItem(iterator, &data));
    EXPECT_NE(STATUS_SUCCESS, stackQueueIteratorGetItem(iterator, NULL));

    // Push
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, stackQueuePush(pStackQueue, i));
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, stackQueueGetCount(pStackQueue, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, stackQueueIsEmpty(pStackQueue, &isEmpty));
    EXPECT_EQ(count, (UINT64) retCount);
    EXPECT_FALSE(isEmpty);

    // Iterate and verify
    EXPECT_EQ(STATUS_SUCCESS, stackQueueGetIterator(pStackQueue, &iterator));
    while (IS_VALID_ITERATOR(iterator)) {
        EXPECT_EQ(STATUS_SUCCESS, stackQueueIteratorGetItem(iterator, &data));
        EXPECT_EQ(count - i - 1, data);
        EXPECT_EQ(STATUS_SUCCESS, stackQueueIteratorNext(&iterator));
        i++;
    }

    EXPECT_EQ(0, data);

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, stackQueueFree(pStackQueue));
}
