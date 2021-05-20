#include "UtilTestFixture.h"

class DoubleListFunctionalityTest : public UtilTestBase {
};

TEST_F(DoubleListFunctionalityTest, NegativeInvalidInput_DoubleListCreate)
{
    EXPECT_NE(STATUS_SUCCESS, doubleListCreate(NULL));
}

TEST_F(DoubleListFunctionalityTest, PositiveIdempotentInvalidInput_DoubleListFree)
{
    EXPECT_EQ(STATUS_SUCCESS, doubleListFree(NULL));
}

TEST_F(DoubleListFunctionalityTest, NegativeInvalidInput_DoubleListClear)
{
    EXPECT_NE(STATUS_SUCCESS, doubleListClear(NULL, FALSE));
}

TEST_F(DoubleListFunctionalityTest, NegativeInvalidInput_DoubleListInsertNodeHeadTail)
{
    DoubleListNode node;
    MEMSET(&node, 0x00, SIZEOF(DoubleListNode));
    PDoubleList pList;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListCreate(&pList));

    EXPECT_NE(STATUS_SUCCESS, doubleListInsertNodeHead(NULL, &node));
    EXPECT_NE(STATUS_SUCCESS, doubleListInsertNodeHead(pList, NULL));
    EXPECT_NE(STATUS_SUCCESS, doubleListInsertNodeTail(NULL, &node));
    EXPECT_NE(STATUS_SUCCESS, doubleListInsertNodeTail(pList, NULL));

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListFree(pList));
}

TEST_F(DoubleListFunctionalityTest, NegativeInvalidInput_DoubleListInsertNodeBeforeAfter)
{
    DoubleListNode node1, node2;
    MEMSET(&node1, 0x00, SIZEOF(DoubleListNode));
    MEMSET(&node2, 0x00, SIZEOF(DoubleListNode));
    PDoubleList pList;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListCreate(&pList));

    EXPECT_NE(STATUS_SUCCESS, doubleListInsertNodeBefore(NULL, &node1, &node2));
    EXPECT_NE(STATUS_SUCCESS, doubleListInsertNodeBefore(pList, NULL, &node2));
    EXPECT_NE(STATUS_SUCCESS, doubleListInsertNodeBefore(pList, &node1, NULL));

    EXPECT_NE(STATUS_SUCCESS, doubleListInsertNodeAfter(NULL, &node1, &node2));
    EXPECT_NE(STATUS_SUCCESS, doubleListInsertNodeAfter(pList, NULL, &node2));
    EXPECT_NE(STATUS_SUCCESS, doubleListInsertNodeAfter(pList, &node1, NULL));

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListFree(pList));
}

TEST_F(DoubleListFunctionalityTest, NegativeInvalidInput_DoubleListInsertItemHeadTailBeforeAfter)
{
    DoubleListNode node;
    MEMSET(&node, 0x00, SIZEOF(DoubleListNode));
    PDoubleList pList;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListCreate(&pList));

    EXPECT_NE(STATUS_SUCCESS, doubleListInsertItemHead(NULL, 0));

    EXPECT_NE(STATUS_SUCCESS, doubleListInsertItemTail(NULL, 0));

    EXPECT_NE(STATUS_SUCCESS, doubleListInsertItemBefore(NULL, &node, 0));
    EXPECT_NE(STATUS_SUCCESS, doubleListInsertItemBefore(pList, NULL, 0));

    EXPECT_NE(STATUS_SUCCESS, doubleListInsertItemAfter(NULL, &node, 0));
    EXPECT_NE(STATUS_SUCCESS, doubleListInsertItemAfter(pList, NULL, 0));

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListFree(pList));
}

TEST_F(DoubleListFunctionalityTest, PositiveIdempotentInvalidInput_DoubleListDeleteHeadTail)
{
    EXPECT_NE(STATUS_SUCCESS, doubleListDeleteHead(NULL));
    EXPECT_NE(STATUS_SUCCESS, doubleListDeleteTail(NULL));
}

TEST_F(DoubleListFunctionalityTest, NegativeInvalidInput_DoubleListRemoveDeleteNode)
{
    DoubleListNode node;
    MEMSET(&node, 0x00, SIZEOF(DoubleListNode));
    PDoubleList pList;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListCreate(&pList));

    EXPECT_NE(STATUS_SUCCESS, doubleListRemoveNode(NULL, &node));
    EXPECT_NE(STATUS_SUCCESS, doubleListRemoveNode(pList, NULL));

    EXPECT_NE(STATUS_SUCCESS, doubleListDeleteNode(NULL, &node));
    EXPECT_NE(STATUS_SUCCESS, doubleListDeleteNode(pList, NULL));

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListFree(pList));
}

TEST_F(DoubleListFunctionalityTest, NegativeInvalidInput_DoubleListGetHeadTailNode)
{
    PDoubleListNode pNode;
    PDoubleList pList;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListCreate(&pList));

    EXPECT_NE(STATUS_SUCCESS, doubleListGetHeadNode(NULL, &pNode));
    EXPECT_NE(STATUS_SUCCESS, doubleListGetHeadNode(pList, NULL));

    EXPECT_NE(STATUS_SUCCESS, doubleListGetTailNode(NULL, &pNode));
    EXPECT_NE(STATUS_SUCCESS, doubleListGetTailNode(pList, NULL));

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListFree(pList));
}

TEST_F(DoubleListFunctionalityTest, NegativeInvalidInput_DoubleListGetNodeAt)
{
    PDoubleListNode pNode;
    PDoubleList pList;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListCreate(&pList));

    EXPECT_NE(STATUS_SUCCESS, doubleListGetNodeAt(NULL, 0, &pNode));
    EXPECT_NE(STATUS_SUCCESS, doubleListGetNodeAt(pList, 0, NULL));

    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemHead(pList, 10));
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeAt(pList, 0, &pNode));
    EXPECT_EQ(10, pNode->data);
    EXPECT_NE(STATUS_SUCCESS, doubleListGetNodeAt(pList, 1, &pNode));

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListFree(pList));
}

TEST_F(DoubleListFunctionalityTest, NegativeInvalidInput_DoubleListGetNodeDataAt)
{
    UINT64 data;
    PDoubleList pList;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListCreate(&pList));

    EXPECT_NE(STATUS_SUCCESS, doubleListGetNodeDataAt(NULL, 0, &data));
    EXPECT_NE(STATUS_SUCCESS, doubleListGetNodeDataAt(pList, 0, NULL));

    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemHead(pList, 10));
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeDataAt(pList, 0, &data));
    EXPECT_EQ(10, data);
    EXPECT_NE(STATUS_SUCCESS, doubleListGetNodeDataAt(pList, 1, &data));

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListFree(pList));
}

TEST_F(DoubleListFunctionalityTest, NegativeInvalidInput_DoubleListGetNodeData)
{
    PDoubleListNode pNode = (PDoubleListNode) 1;
    UINT64 data;

    EXPECT_NE(STATUS_SUCCESS, doubleListGetNodeData(NULL, &data));
    EXPECT_NE(STATUS_SUCCESS, doubleListGetNodeData(pNode, NULL));
}

TEST_F(DoubleListFunctionalityTest, NegativeInvalidInput_DoubleListGetNodeNextPrev)
{
    PDoubleListNode pNode;

    EXPECT_NE(STATUS_SUCCESS, doubleListGetNextNode(NULL, &pNode));
    EXPECT_NE(STATUS_SUCCESS, doubleListGetNextNode(pNode, NULL));

    EXPECT_NE(STATUS_SUCCESS, doubleListGetPrevNode(NULL, &pNode));
    EXPECT_NE(STATUS_SUCCESS, doubleListGetPrevNode(pNode, NULL));
}

TEST_F(DoubleListFunctionalityTest, NegativeInvalidInput_DoubleListGetNodeCount)
{
    PDoubleList pList = (PDoubleList) 1;
    UINT32 count;

    EXPECT_NE(STATUS_SUCCESS, doubleListGetNodeCount(NULL, &count));
    EXPECT_NE(STATUS_SUCCESS, doubleListGetNodeCount(pList, NULL));
}

TEST_F(DoubleListFunctionalityTest,  DoubleListClear)
{
    PDoubleList pList;
    UINT64 count = 10;
    UINT32 retCount;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListCreate(&pList));

    // Insert at the head
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemHead(pList, i));
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(count, (UINT64) retCount);

    // Clear the data
    EXPECT_EQ(STATUS_SUCCESS, doubleListClear(pList, FALSE));

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(0, (UINT64) retCount);

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListFree(pList));
}

TEST_F(DoubleListFunctionalityTest, DoubleListBasicOperationsCreateInsertGetDelete)
{
    PDoubleList pList;
    PDoubleListNode pInsertNode;
    PDoubleListNode pNode, pHead, pTail;
    UINT64 data;
    UINT64 count = 10000;
    UINT32 retCount;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListCreate(&pList));

    // Insert at the head
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemHead(pList, i));
    }

    // Validate
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeDataAt(pList, (UINT32) i, &data));
        EXPECT_EQ(data, count - i - 1);
        EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeAt(pList, (UINT32) i, &pNode));
        EXPECT_EQ(pNode->data, count - i - 1);
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(count, (UINT64) retCount);

    // Remove the nodes
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, doubleListDeleteHead(pList));
        EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeCount(pList, &retCount));
        EXPECT_EQ(count - i - 1, (UINT64) retCount);
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(0, (UINT64) retCount);

    // Insert at the tail
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemTail(pList, i));
    }

    // Validate
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeDataAt(pList, (UINT32) i, &data));
        EXPECT_EQ(data, i);
        EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeAt(pList, (UINT32) i, &pNode));
        EXPECT_EQ(pNode->data, i);
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(count, (UINT64) retCount);

    // Remove the nodes
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, doubleListDeleteHead(pList));
        EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeCount(pList, &retCount));
        EXPECT_EQ(count - i - 1, (UINT64) retCount);
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(0, (UINT64) retCount);

    // Insert a few nodes
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemHead(pList, 10));
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemTail(pList, 20));
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemTail(pList, 30));

    // Check the head and tail
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeAt(pList, 0, &pNode));
    EXPECT_EQ(pNode->data, 10);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetHeadNode(pList, &pHead));
    EXPECT_EQ(pNode, pHead);

    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeAt(pList, 2, &pNode));
    EXPECT_EQ(pNode->data, 30);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetTailNode(pList, &pTail));
    EXPECT_EQ(pNode, pTail);

    // Insert before head, after, before and after tail
    pInsertNode = (PDoubleListNode) MEMALLOC(SIZEOF(DoubleListNode));
    pInsertNode->data = 0;
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertNodeBefore(pList, pHead, pInsertNode));
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeAt(pList, 0, &pNode));
    EXPECT_EQ(pNode->data, 0);

    pInsertNode = (PDoubleListNode) MEMALLOC(SIZEOF(DoubleListNode));
    pInsertNode->data = 5;
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertNodeAfter(pList, pNode, pInsertNode));
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeAt(pList, 1, &pNode));
    EXPECT_EQ(pNode->data, 5);

    pInsertNode = (PDoubleListNode) MEMALLOC(SIZEOF(DoubleListNode));
    pInsertNode->data = 15;
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeAt(pList, 2, &pNode));
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertNodeAfter(pList, pNode, pInsertNode));
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeAt(pList, 3, &pNode));
    EXPECT_EQ(pNode->data, 15);

    pInsertNode = (PDoubleListNode) MEMALLOC(SIZEOF(DoubleListNode));
    pInsertNode->data = 25;
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertNodeBefore(pList, pTail, pInsertNode));
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetPrevNode(pTail, &pNode));
    EXPECT_EQ(pNode->data, 25);

    pInsertNode = (PDoubleListNode) MEMALLOC(SIZEOF(DoubleListNode));
    pInsertNode->data = 35;
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertNodeAfter(pList, pTail, pInsertNode));
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeCount(pList, &retCount));

    // Gets the tail node
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeAt(pList, retCount - 1, &pNode));
    EXPECT_EQ(pNode->data, 35);

    // Ensure the tail node is the same
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetTailNode(pList, &pTail));
    EXPECT_EQ(pNode, pTail);

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListFree(pList));
}

TEST_F(DoubleListFunctionalityTest, DoubleListRemoveDeleteNode)
{
    PDoubleList pList;
    PDoubleListNode pNode, pHead, pTail;
    UINT64 count = 100;
    UINT32 retCount;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListCreate(&pList));

    // Delete the head/tail of an empty list - should work OK
    EXPECT_EQ(STATUS_SUCCESS, doubleListDeleteHead(pList));
    EXPECT_EQ(STATUS_SUCCESS, doubleListDeleteTail(pList));

    // Insert at the tail
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemTail(pList, i));
    }

    // Remove the first, second, one before last and last
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetHeadNode(pList, &pHead));
    EXPECT_EQ(STATUS_SUCCESS, doubleListRemoveNode(pList, pHead));
    if (pHead) {
        MEMFREE(pHead);
    }

    EXPECT_EQ(STATUS_SUCCESS, doubleListGetHeadNode(pList, &pHead));
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNextNode(pHead, &pNode));
    EXPECT_EQ(STATUS_SUCCESS, doubleListRemoveNode(pList, pNode));
    if (pNode) {
        MEMFREE(pNode);
    }

    EXPECT_EQ(STATUS_SUCCESS, doubleListGetTailNode(pList, &pTail));
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetPrevNode(pTail, &pNode));
    EXPECT_EQ(STATUS_SUCCESS, doubleListRemoveNode(pList, pNode));
    if (pNode) {
        MEMFREE(pNode);
    }

    EXPECT_EQ(STATUS_SUCCESS, doubleListGetTailNode(pList, &pTail));
    EXPECT_EQ(STATUS_SUCCESS, doubleListRemoveNode(pList, pTail));
    if (pTail) {
        MEMFREE(pTail);
    }

    // Do the same with deleting
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetHeadNode(pList, &pHead));
    EXPECT_EQ(STATUS_SUCCESS, doubleListDeleteNode(pList, pHead));

    EXPECT_EQ(STATUS_SUCCESS, doubleListGetHeadNode(pList, &pHead));
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNextNode(pHead, &pNode));
    EXPECT_EQ(STATUS_SUCCESS, doubleListDeleteNode(pList, pNode));

    EXPECT_EQ(STATUS_SUCCESS, doubleListGetTailNode(pList, &pTail));
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetPrevNode(pTail, &pNode));
    EXPECT_EQ(STATUS_SUCCESS, doubleListDeleteNode(pList, pNode));

    EXPECT_EQ(STATUS_SUCCESS, doubleListGetTailNode(pList, &pTail));
    EXPECT_EQ(STATUS_SUCCESS, doubleListDeleteNode(pList, pTail));

    // Remove all the remaining nodes
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeCount(pList, &retCount));

    // Remove the nodes
    for (UINT64 i = 0; i < retCount; i++) {
        EXPECT_EQ(STATUS_SUCCESS, doubleListDeleteHead(pList));
    }

    // Add a double node and remove head and then tail
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemHead(pList, 1));
    EXPECT_EQ(STATUS_SUCCESS, doubleListDeleteHead(pList));
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetHeadNode(pList, &pHead));
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetTailNode(pList, &pTail));
    EXPECT_EQ(retCount, 0);
    EXPECT_EQ(pHead, pTail);
    EXPECT_TRUE(pHead == NULL);

    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemHead(pList, 1));
    EXPECT_EQ(STATUS_SUCCESS, doubleListDeleteTail(pList));
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetHeadNode(pList, &pHead));
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetTailNode(pList, &pTail));
    EXPECT_EQ(retCount, 0);
    EXPECT_EQ(pHead, pTail);
    EXPECT_TRUE(pHead == NULL);

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListFree(pList));
}

TEST_F(DoubleListFunctionalityTest, DoubleListInsertBeforeAfter)
{
    PDoubleList pList;
    PDoubleListNode pHead, pTail;
    UINT64 data;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListCreate(&pList));

    // Insert at the tail
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemTail(pList, 10));
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetHeadNode(pList, &pHead));

    // Insert before and then after and validate
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemBefore(pList, pHead, 0));
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemAfter(pList, pHead, 20));

    // Validate
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeDataAt(pList, 0, &data));
    EXPECT_EQ(data, 0);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeDataAt(pList, 1, &data));
    EXPECT_EQ(data, 10);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeDataAt(pList, 2, &data));
    EXPECT_EQ(data, 20);

    // Insert before and after the tail
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetTailNode(pList, &pTail));
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemBefore(pList, pTail, 15));
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemAfter(pList, pTail, 25));

    // Validate
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeDataAt(pList, 0, &data));
    EXPECT_EQ(data, 0);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeDataAt(pList, 1, &data));
    EXPECT_EQ(data, 10);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeDataAt(pList, 2, &data));
    EXPECT_EQ(data, 15);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeDataAt(pList, 3, &data));
    EXPECT_EQ(data, 20);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeDataAt(pList, 4, &data));
    EXPECT_EQ(data, 25);

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, doubleListFree(pList));
}

TEST_F(DoubleListFunctionalityTest, DoubleListAppendNonEmptyList) {
    PDoubleList pListDst, pListToAppend;
    PDoubleListNode pNode;

    // Create the list dst
    EXPECT_EQ(STATUS_SUCCESS, doubleListCreate(&pListDst));

    // Insert at the tail
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemTail(pListDst, 10));
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemTail(pListDst, 20));

    // Create the list to append
    EXPECT_EQ(STATUS_SUCCESS, doubleListCreate(&pListToAppend));

    // Insert at the tail
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemTail(pListToAppend, 30));
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemTail(pListToAppend, 40));
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemTail(pListToAppend, 50));

    EXPECT_EQ(2, pListDst->count);
    EXPECT_EQ(3, pListToAppend->count);

    EXPECT_EQ(STATUS_SUCCESS, doubleListAppendList(pListDst, &pListToAppend));

    EXPECT_EQ(5, pListDst->count);
    EXPECT_EQ(NULL, pListToAppend);

    // Validate
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetHeadNode(pListDst, &pNode));
    EXPECT_NE(NULL, (UINT64) pNode);
    EXPECT_EQ(pNode->data, 10);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNextNode(pNode, &pNode));
    EXPECT_NE(NULL, (UINT64)  pNode);
    EXPECT_EQ(pNode->data, 20);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNextNode(pNode, &pNode));
    EXPECT_NE(NULL, (UINT64)  pNode);
    EXPECT_EQ(pNode->data, 30);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNextNode(pNode, &pNode));
    EXPECT_NE(NULL, (UINT64)  pNode);
    EXPECT_EQ(pNode->data, 40);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNextNode(pNode, &pNode));
    EXPECT_NE(NULL, (UINT64)  pNode);
    EXPECT_EQ(pNode->data, 50);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNextNode(pNode, &pNode));
    EXPECT_EQ(NULL, pNode);

    EXPECT_EQ(STATUS_SUCCESS, doubleListGetTailNode(pListDst, &pNode));
    EXPECT_NE(NULL, (UINT64)  pNode);
    EXPECT_EQ(pNode->data, 50);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetPrevNode(pNode, &pNode));
    EXPECT_NE(NULL, (UINT64)  pNode);
    EXPECT_EQ(pNode->data, 40);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetPrevNode(pNode, &pNode));
    EXPECT_NE(NULL, (UINT64)  pNode);
    EXPECT_EQ(pNode->data, 30);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetPrevNode(pNode, &pNode));
    EXPECT_NE(NULL, (UINT64)  pNode);
    EXPECT_EQ(pNode->data, 20);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetPrevNode(pNode, &pNode));
    EXPECT_NE(NULL, (UINT64)  pNode);
    EXPECT_EQ(pNode->data, 10);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetPrevNode(pNode, &pNode));
    EXPECT_EQ(NULL, pNode);

    // Destroy the list dst. list to append should have been freed
    EXPECT_EQ(STATUS_SUCCESS, doubleListFree(pListDst));
}

TEST_F(DoubleListFunctionalityTest, DoubleListAppendEmptyListOrNull) {
    PDoubleList pListDst, pListToAppend;
    UINT64 data;

    // Create the list dst
    EXPECT_EQ(STATUS_SUCCESS, doubleListCreate(&pListDst));

    // Insert at the tail
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemTail(pListDst, 10));
    EXPECT_EQ(STATUS_SUCCESS, doubleListInsertItemTail(pListDst, 20));

    // Create the list to append
    EXPECT_EQ(STATUS_SUCCESS, doubleListCreate(&pListToAppend));

    EXPECT_EQ(2, pListDst->count);
    EXPECT_EQ(0, pListToAppend->count);

    // append empty list to dst list
    EXPECT_EQ(STATUS_SUCCESS, doubleListAppendList(pListDst, &pListToAppend));

    EXPECT_EQ(2, pListDst->count);
    EXPECT_EQ(NULL, pListToAppend);

    // Validate
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeDataAt(pListDst, 0, &data));
    EXPECT_EQ(data, 10);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeDataAt(pListDst, 1, &data));
    EXPECT_EQ(data, 20);

    // append null to dst list
    EXPECT_EQ(STATUS_SUCCESS, doubleListAppendList(pListDst, &pListToAppend));

    EXPECT_EQ(2, pListDst->count);
    EXPECT_EQ(NULL, pListToAppend);

    // Validate
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeDataAt(pListDst, 0, &data));
    EXPECT_EQ(data, 10);
    EXPECT_EQ(STATUS_SUCCESS, doubleListGetNodeDataAt(pListDst, 1, &data));
    EXPECT_EQ(data, 20);

    // Destroy the list dst. list to append should have been freed
    EXPECT_EQ(STATUS_SUCCESS, doubleListFree(pListDst));
}
