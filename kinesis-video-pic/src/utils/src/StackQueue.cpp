#include "Include_i.h"

/**
 * Create a new stack/queue
 */
STATUS stackQueueCreate(PStackQueue* ppStackQueue)
{
    return singleListCreate(ppStackQueue);
}

/**
 * Frees and de-allocates the stack queue
 */
STATUS stackQueueFree(PStackQueue pStackQueue)
{
    return singleListFree(pStackQueue);
}

/**
 * Clears and de-allocates all the items
 */
STATUS stackQueueClear(PStackQueue pStackQueue)
{
    return singleListClear(pStackQueue);
}

/**
 * Gets the number of items in the stack/queue
 */
STATUS stackQueueGetCount(PStackQueue pStackQueue, PUINT32 pCount)
{
    return singleListGetNodeCount(pStackQueue, pCount);
}

/**
 * Whether the stack queue is empty
 */
STATUS stackQueueIsEmpty(PStackQueue pStackQueue, PBOOL pIsEmpty)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 count;

    // The call is idempotent so we shouldn't fail
    CHK(pStackQueue != NULL && pIsEmpty != NULL, STATUS_NULL_ARG);

    CHK_STATUS(singleListGetNodeCount(pStackQueue, &count));

    *pIsEmpty = (count == 0);

CleanUp:

    return retStatus;
}

/**
 * Pushes an item onto the stack
 */
STATUS stackQueuePush(PStackQueue pStackQueue, UINT64 item)
{
    return singleListInsertItemHead(pStackQueue, item);
}

/**
 * Pops an item from the stack
 */
STATUS stackQueuePop(PStackQueue pStackQueue, PUINT64 pItem)
{
    STATUS retStatus = STATUS_SUCCESS;

    CHK_STATUS(stackQueuePeek(pStackQueue, pItem));
    CHK_STATUS(singleListDeleteHead(pStackQueue));

CleanUp:

    return retStatus;
}

/**
 * Peeks an item from the stack without popping
 */
STATUS stackQueuePeek(PStackQueue pStackQueue, PUINT64 pItem)
{
    STATUS retStatus = STATUS_SUCCESS;
    PSingleListNode pHead;

    CHK_STATUS(singleListGetHeadNode(pStackQueue, &pHead));
    CHK(pHead != NULL, STATUS_INVALID_OPERATION);
    CHK_STATUS(singleListGetNodeData(pHead, pItem));

CleanUp:

    return retStatus;
}

/**
 * Enqueues an item in the queue
 */
STATUS stackQueueEnqueue(PStackQueue pStackQueue, UINT64 item)
{
    return singleListInsertItemTail(pStackQueue, item);
}

/**
 * Dequeues an item from the queue
 */
STATUS stackQueueDequeue(PStackQueue pStackQueue, PUINT64 pItem)
{
    STATUS retStatus = STATUS_SUCCESS;
    PSingleListNode pHead;

    CHK_STATUS(singleListGetHeadNode(pStackQueue, &pHead));
    CHK(pHead != NULL, STATUS_INVALID_OPERATION);
    CHK_STATUS(singleListGetNodeData(pHead, pItem));
    CHK_STATUS(singleListDeleteHead(pStackQueue));

CleanUp:

    return retStatus;
}
