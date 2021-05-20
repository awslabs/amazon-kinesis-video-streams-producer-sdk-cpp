#include "UtilTestFixture.h"

class SingleListFunctionalityTest : public UtilTestBase {
};

TEST_F(SingleListFunctionalityTest, NegativeInvalidInput_SingleListCreate)
{
    EXPECT_NE(STATUS_SUCCESS, singleListCreate(NULL));
}

TEST_F(SingleListFunctionalityTest, PositiveIdempotentInvalidInput_SingleListFree)
{
    EXPECT_EQ(STATUS_SUCCESS, singleListFree(NULL));
}

TEST_F(SingleListFunctionalityTest, NegativeInvalidInput_SingleListClear)
{
    EXPECT_NE(STATUS_SUCCESS, singleListClear(NULL, FALSE));
}

TEST_F(SingleListFunctionalityTest, NegativeInvalidInput_SingleListInsertNodeHeadTail)
{
    SingleListNode node;
    MEMSET(&node, 0x00, SIZEOF(SingleListNode));
    PSingleList pList;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, singleListCreate(&pList));

    EXPECT_NE(STATUS_SUCCESS, singleListInsertNodeHead(NULL, &node));
    EXPECT_NE(STATUS_SUCCESS, singleListInsertNodeHead(pList, NULL));
    EXPECT_NE(STATUS_SUCCESS, singleListInsertNodeTail(NULL, &node));
    EXPECT_NE(STATUS_SUCCESS, singleListInsertNodeTail(pList, NULL));

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, singleListFree(pList));
}

TEST_F(SingleListFunctionalityTest, NegativeInvalidInput_SingleListInsertNodeAfter)
{
    SingleListNode node1, node2;
    MEMSET(&node1, 0x00, SIZEOF(SingleListNode));
    MEMSET(&node2, 0x00, SIZEOF(SingleListNode));
    PSingleList pList;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, singleListCreate(&pList));

    EXPECT_NE(STATUS_SUCCESS, singleListInsertNodeAfter(NULL, &node1, &node2));
    EXPECT_NE(STATUS_SUCCESS, singleListInsertNodeAfter(pList, NULL, &node2));
    EXPECT_NE(STATUS_SUCCESS, singleListInsertNodeAfter(pList, &node1, NULL));

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, singleListFree(pList));
}

TEST_F(SingleListFunctionalityTest, NegativeInvalidInput_SingleListInsertItemHeadTailAfter)
{
    SingleListNode node;
    MEMSET(&node, 0x00, SIZEOF(SingleListNode));
    PSingleList pList;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, singleListCreate(&pList));

    EXPECT_NE(STATUS_SUCCESS, singleListInsertItemHead(NULL, 0));

    EXPECT_NE(STATUS_SUCCESS, singleListInsertItemTail(NULL, 0));

    EXPECT_NE(STATUS_SUCCESS, singleListInsertItemAfter(NULL, &node, 0));
    EXPECT_NE(STATUS_SUCCESS, singleListInsertItemAfter(pList, NULL, 0));

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, singleListFree(pList));
}

TEST_F(SingleListFunctionalityTest, PositiveIdempotentInvalidInput_SingleListDeleteHead)
{
    EXPECT_NE(STATUS_SUCCESS, singleListDeleteHead(NULL));
}

TEST_F(SingleListFunctionalityTest, NegativeInvalidInput_SingleListDeleteNextNode)
{
    SingleListNode node;
    MEMSET(&node, 0x00, SIZEOF(SingleListNode));
    PSingleList pList;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, singleListCreate(&pList));

    EXPECT_NE(STATUS_SUCCESS, singleListDeleteNextNode(NULL, &node));
    EXPECT_NE(STATUS_SUCCESS, singleListDeleteNextNode(pList, NULL));

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, singleListFree(pList));
}

TEST_F(SingleListFunctionalityTest, NegativeInvalidInput_SingleListDeleteNode)
{
    SingleListNode node;
    MEMSET(&node, 0x00, SIZEOF(SingleListNode));
    PSingleList pList;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, singleListCreate(&pList));

    EXPECT_NE(STATUS_SUCCESS, singleListDeleteNode(NULL, &node));
    EXPECT_NE(STATUS_SUCCESS, singleListDeleteNode(pList, NULL));

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, singleListFree(pList));
}

TEST_F(SingleListFunctionalityTest, NegativeInvalidInput_SingleListGetHeadTailNode)
{
    PSingleListNode pNode;
    PSingleList pList;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, singleListCreate(&pList));

    EXPECT_NE(STATUS_SUCCESS, singleListGetHeadNode(NULL, &pNode));
    EXPECT_NE(STATUS_SUCCESS, singleListGetHeadNode(pList, NULL));

    EXPECT_NE(STATUS_SUCCESS, singleListGetTailNode(NULL, &pNode));
    EXPECT_NE(STATUS_SUCCESS, singleListGetTailNode(pList, NULL));

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, singleListFree(pList));
}

TEST_F(SingleListFunctionalityTest, NegativeInvalidInput_SingleListGetNodeAt)
{
    PSingleListNode pNode;
    PSingleList pList;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, singleListCreate(&pList));

    EXPECT_NE(STATUS_SUCCESS, singleListGetNodeAt(NULL, 0, &pNode));
    EXPECT_NE(STATUS_SUCCESS, singleListGetNodeAt(pList, 0, NULL));

    EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemHead(pList, 10));
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeAt(pList, 0, &pNode));
    EXPECT_EQ(10, pNode->data);
    EXPECT_NE(STATUS_SUCCESS, singleListGetNodeAt(pList, 1, &pNode));

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, singleListFree(pList));
}

TEST_F(SingleListFunctionalityTest, NegativeInvalidInput_SingleListGetNodeDataAt)
{
    UINT64 data;
    PSingleList pList;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, singleListCreate(&pList));

    EXPECT_NE(STATUS_SUCCESS, singleListGetNodeDataAt(NULL, 0, &data));
    EXPECT_NE(STATUS_SUCCESS, singleListGetNodeDataAt(pList, 0, NULL));

    EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemHead(pList, 10));
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeDataAt(pList, 0, &data));
    EXPECT_EQ(10, data);
    EXPECT_NE(STATUS_SUCCESS, singleListGetNodeDataAt(pList, 1, &data));

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, singleListFree(pList));
}

TEST_F(SingleListFunctionalityTest, NegativeInvalidInput_SingleListGetNodeData)
{
    PSingleListNode pNode = (PSingleListNode) 1;
    UINT64 data;

    EXPECT_NE(STATUS_SUCCESS, singleListGetNodeData(NULL, &data));
    EXPECT_NE(STATUS_SUCCESS, singleListGetNodeData(pNode, NULL));
}

TEST_F(SingleListFunctionalityTest, NegativeInvalidInput_SingleListGetNodeNext)
{
    PSingleListNode pNode = (PSingleListNode) 1;

    EXPECT_NE(STATUS_SUCCESS, singleListGetNextNode(NULL, &pNode));
    EXPECT_NE(STATUS_SUCCESS, singleListGetNextNode(pNode, NULL));
}

TEST_F(SingleListFunctionalityTest, NegativeInvalidInput_SingleListGetNodeCount)
{
    PSingleList pList = (PSingleList) 1;
    UINT32 count;

    EXPECT_NE(STATUS_SUCCESS, singleListGetNodeCount(NULL, &count));
    EXPECT_NE(STATUS_SUCCESS, singleListGetNodeCount(pList, NULL));
}

TEST_F(SingleListFunctionalityTest, SingleListClear)
{
    PSingleList pList;
    UINT64 count = 10;
    UINT32 retCount;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, singleListCreate(&pList));

    // Insert at the head
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemHead(pList, i));
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(count, (UINT64) retCount);

    // Clear the data
    EXPECT_EQ(STATUS_SUCCESS, singleListClear(pList, FALSE));

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(0, (UINT64) retCount);

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, singleListFree(pList));
}

TEST_F(SingleListFunctionalityTest, SingleListBasicOperationsCreateInsertGetDelete)
{
    PSingleList pList;
    PSingleListNode pInsertNode;
    PSingleListNode pNode, pHead, pTail;
    UINT64 data;
    UINT64 count = 10000;
    UINT32 retCount;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, singleListCreate(&pList));

    // Insert at the head
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemHead(pList, i));
    }

    // Validate
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeDataAt(pList, (UINT32) i, &data));
        EXPECT_EQ(data, count - i - 1);
        EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeAt(pList, (UINT32)i, &pNode));
        EXPECT_EQ(pNode->data, count - i - 1);
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(count, (UINT64) retCount);

    // Remove the nodes
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, singleListDeleteHead(pList));
        EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeCount(pList, &retCount));
        EXPECT_EQ(count - i - 1, (UINT64) retCount);
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(0, (UINT64) retCount);

    // Insert at the tail
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemTail(pList, i));
    }

    // Validate
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeDataAt(pList, (UINT32) i, &data));
        EXPECT_EQ(data, i);
        EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeAt(pList, (UINT32) i, &pNode));
        EXPECT_EQ(pNode->data, i);
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(count, (UINT64) retCount);

    // Remove the nodes
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, singleListDeleteHead(pList));
        EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeCount(pList, &retCount));
        EXPECT_EQ(count - i - 1, (UINT64) retCount);
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(0, (UINT64) retCount);

    // Insert a few nodes
    EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemHead(pList, 10));
    EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemTail(pList, 20));
    EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemTail(pList, 30));

    // Check the head and tail
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeAt(pList, 0, &pNode));
    EXPECT_EQ(pNode->data, 10);
    EXPECT_EQ(STATUS_SUCCESS, singleListGetHeadNode(pList, &pHead));
    EXPECT_EQ(pNode, pHead);

    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeAt(pList, 2, &pNode));
    EXPECT_EQ(pNode->data, 30);
    EXPECT_EQ(STATUS_SUCCESS, singleListGetTailNode(pList, &pTail));
    EXPECT_EQ(pNode, pTail);

    // Insert after head, and tail
    pInsertNode = (PSingleListNode) MEMALLOC(SIZEOF(SingleListNode));
    pInsertNode->data = 15;
    EXPECT_EQ(STATUS_SUCCESS, singleListInsertNodeAfter(pList, pHead, pInsertNode));
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeAt(pList, 1, &pNode));
    EXPECT_EQ(pNode->data, 15);

    pInsertNode = (PSingleListNode) MEMALLOC(SIZEOF(SingleListNode));
    pInsertNode->data = 35;
    EXPECT_EQ(STATUS_SUCCESS, singleListInsertNodeAfter(pList, pTail, pInsertNode));
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeAt(pList, 4, &pNode));
    EXPECT_EQ(pNode->data, 35);

    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeCount(pList, &retCount));

    // Gets the tail node
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeAt(pList, retCount - 1, &pNode));
    EXPECT_EQ(pNode->data, 35);

    // Ensure the tail node is the same
    EXPECT_EQ(STATUS_SUCCESS, singleListGetTailNode(pList, &pTail));
    EXPECT_EQ(pNode, pTail);

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, singleListFree(pList));
}

TEST_F(SingleListFunctionalityTest, SingleListDeleteNextNode)
{
    PSingleList pList;
    PSingleListNode pNode, pHead, pTail;
    UINT64 count = 100;
    UINT32 retCount;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, singleListCreate(&pList));

    // Delete the head/tail of an empty list - should work OK
    EXPECT_EQ(STATUS_SUCCESS, singleListDeleteHead(pList));

    // Insert at the tail
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemTail(pList, i));
    }

    // Delete the first, second, one before last and last
    EXPECT_EQ(STATUS_SUCCESS, singleListGetHeadNode(pList, &pHead));
    EXPECT_EQ(STATUS_SUCCESS, singleListDeleteHead(pList));

    EXPECT_EQ(STATUS_SUCCESS, singleListGetHeadNode(pList, &pHead));
    EXPECT_EQ(STATUS_SUCCESS, singleListDeleteNextNode(pList, pHead));

    // Get the 3rd last and remove it's next
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeAt(pList, retCount - 3, &pNode));
    EXPECT_EQ(STATUS_SUCCESS, singleListDeleteNextNode(pList, pNode));

    // Get the 2nd last and remove it's next
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeAt(pList, retCount - 2, &pNode));
    EXPECT_EQ(STATUS_SUCCESS, singleListDeleteNextNode(pList, pNode));

    // Get the last and remove it's next
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeAt(pList, retCount - 1, &pNode));
    EXPECT_EQ(STATUS_SUCCESS, singleListDeleteNextNode(pList, pNode));

    // Remove all the remaining nodes
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeCount(pList, &retCount));

    // Remove the nodes
    for (UINT64 i = 0; i < retCount; i++) {
        EXPECT_EQ(STATUS_SUCCESS, singleListDeleteHead(pList));
    }

    // Add a single node and remove head
    EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemHead(pList, 1));
    EXPECT_EQ(STATUS_SUCCESS, singleListDeleteHead(pList));
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, singleListGetHeadNode(pList, &pHead));
    EXPECT_EQ(STATUS_SUCCESS, singleListGetTailNode(pList, &pTail));
    EXPECT_EQ(retCount, 0);
    EXPECT_EQ(pHead, pTail);
    EXPECT_TRUE(pHead == NULL);

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, singleListFree(pList));
}

TEST_F(SingleListFunctionalityTest, SingleListDeleteNode)
{
    PSingleList pList;
    PSingleListNode pNode, pHead, pTail;
    UINT64 count = 100;
    UINT32 retCount;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, singleListCreate(&pList));

    // Delete the head/tail of an empty list - should work OK
    EXPECT_EQ(STATUS_SUCCESS, singleListDeleteHead(pList));

    // Insert at the tail
    for (UINT64 i = 0; i < count; i++) {
        EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemTail(pList, i));
    }

    // Delete the first, second as head, then second after head, one before last and last
    EXPECT_EQ(STATUS_SUCCESS, singleListGetHeadNode(pList, &pHead));
    EXPECT_EQ(STATUS_SUCCESS, singleListDeleteNode(pList, pHead));

    EXPECT_EQ(STATUS_SUCCESS, singleListGetHeadNode(pList, &pHead));
    EXPECT_EQ(STATUS_SUCCESS, singleListDeleteNode(pList, pHead));

    EXPECT_EQ(STATUS_SUCCESS, singleListGetHeadNode(pList, &pHead));
    pNode = pHead->pNext;
    EXPECT_EQ(STATUS_SUCCESS, singleListDeleteNode(pList, pNode));

    // Get the 3rd last and delete it
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeAt(pList, retCount - 3, &pNode));
    EXPECT_EQ(STATUS_SUCCESS, singleListDeleteNode(pList, pNode));

    // Get the 2nd last and delete it
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeAt(pList, retCount - 2, &pNode));
    EXPECT_EQ(STATUS_SUCCESS, singleListDeleteNode(pList, pNode));

    // Get the last and delete it
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeAt(pList, retCount - 1, &pNode));
    EXPECT_EQ(STATUS_SUCCESS, singleListDeleteNode(pList, pNode));

    // Get the tail and delete it
    EXPECT_EQ(STATUS_SUCCESS, singleListGetTailNode(pList, &pTail));
    EXPECT_EQ(STATUS_SUCCESS, singleListDeleteNode(pList, pTail));

    // Remove all the remaining nodes
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeCount(pList, &retCount));

    // Remove the nodes
    for (UINT64 i = 0; i < retCount; i++) {
        EXPECT_EQ(STATUS_SUCCESS, singleListDeleteHead(pList));
    }

    // Add a single node and remove head
    EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemHead(pList, 1));
    EXPECT_EQ(STATUS_SUCCESS, singleListDeleteHead(pList));
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeCount(pList, &retCount));
    EXPECT_EQ(STATUS_SUCCESS, singleListGetHeadNode(pList, &pHead));
    EXPECT_EQ(STATUS_SUCCESS, singleListGetTailNode(pList, &pTail));
    EXPECT_EQ(retCount, 0);
    EXPECT_EQ(pHead, pTail);
    EXPECT_TRUE(pHead == NULL);

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, singleListFree(pList));
}

TEST_F(SingleListFunctionalityTest, SingleListInsertAfter)
{
    PSingleList pList;
    PSingleListNode pHead, pTail;
    UINT64 data;

    // Create the list
    EXPECT_EQ(STATUS_SUCCESS, singleListCreate(&pList));

    // Insert at the tail
    EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemTail(pList, 10));
    EXPECT_EQ(STATUS_SUCCESS, singleListGetHeadNode(pList, &pHead));

    // Insert after and validate
    EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemAfter(pList, pHead, 20));

    // Validate
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeDataAt(pList, 0, &data));
    EXPECT_EQ(data, 10);
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeDataAt(pList, 1, &data));
    EXPECT_EQ(data, 20);

    // Insert after the tail
    EXPECT_EQ(STATUS_SUCCESS, singleListGetTailNode(pList, &pTail));
    EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemAfter(pList, pTail, 30));

    // Validate
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeDataAt(pList, 0, &data));
    EXPECT_EQ(data, 10);
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeDataAt(pList, 1, &data));
    EXPECT_EQ(data, 20);
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeDataAt(pList, 2, &data));
    EXPECT_EQ(data, 30);

    // Destroy the list
    EXPECT_EQ(STATUS_SUCCESS, singleListFree(pList));
}

TEST_F(SingleListFunctionalityTest, SingleListAppendNonEmptyList) {
    PSingleList pListDst, pListToAppend;
    UINT64 data;

    // Create the list dst
    EXPECT_EQ(STATUS_SUCCESS, singleListCreate(&pListDst));

    // Insert at the tail
    EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemTail(pListDst, 10));
    EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemTail(pListDst, 20));

    // Create the list to append
    EXPECT_EQ(STATUS_SUCCESS, singleListCreate(&pListToAppend));

    // Insert at the tail
    EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemTail(pListToAppend, 30));
    EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemTail(pListToAppend, 40));
    EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemTail(pListToAppend, 50));

    EXPECT_EQ(2, pListDst->count);
    EXPECT_EQ(3, pListToAppend->count);

    EXPECT_EQ(STATUS_SUCCESS, singleListAppendList(pListDst, &pListToAppend));

    EXPECT_EQ(5, pListDst->count);
    EXPECT_EQ(NULL, pListToAppend);

    // Validate
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeDataAt(pListDst, 0, &data));
    EXPECT_EQ(data, 10);
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeDataAt(pListDst, 1, &data));
    EXPECT_EQ(data, 20);
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeDataAt(pListDst, 2, &data));
    EXPECT_EQ(data, 30);
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeDataAt(pListDst, 3, &data));
    EXPECT_EQ(data, 40);
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeDataAt(pListDst, 4, &data));
    EXPECT_EQ(data, 50);

    // Destroy the list dst. list to append should have been freed
    EXPECT_EQ(STATUS_SUCCESS, singleListFree(pListDst));
}

TEST_F(SingleListFunctionalityTest, SingleListAppendEmptyListOrNull) {
    PSingleList pListDst, pListToAppend;
    UINT64 data;

    // Create the list dst
    EXPECT_EQ(STATUS_SUCCESS, singleListCreate(&pListDst));

    // Insert at the tail
    EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemTail(pListDst, 10));
    EXPECT_EQ(STATUS_SUCCESS, singleListInsertItemTail(pListDst, 20));

    // Create the list to append
    EXPECT_EQ(STATUS_SUCCESS, singleListCreate(&pListToAppend));

    EXPECT_EQ(2, pListDst->count);
    EXPECT_EQ(0, pListToAppend->count);

    // append empty list to dst list
    EXPECT_EQ(STATUS_SUCCESS, singleListAppendList(pListDst, &pListToAppend));

    EXPECT_EQ(2, pListDst->count);
    EXPECT_EQ(NULL, pListToAppend);

    // Validate
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeDataAt(pListDst, 0, &data));
    EXPECT_EQ(data, 10);
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeDataAt(pListDst, 1, &data));
    EXPECT_EQ(data, 20);

    // append null to dst list
    EXPECT_EQ(STATUS_SUCCESS, singleListAppendList(pListDst, &pListToAppend));

    EXPECT_EQ(2, pListDst->count);
    EXPECT_EQ(NULL, pListToAppend);

    // Validate
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeDataAt(pListDst, 0, &data));
    EXPECT_EQ(data, 10);
    EXPECT_EQ(STATUS_SUCCESS, singleListGetNodeDataAt(pListDst, 1, &data));
    EXPECT_EQ(data, 20);

    // Destroy the list dst. list to append should have been freed
    EXPECT_EQ(STATUS_SUCCESS, singleListFree(pListDst));
}
