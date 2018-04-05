/**
 * Various utility functionality
 */
#ifndef __UTILS_H__
#define __UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#pragma once

#include <com/amazonaws/kinesis/video/common/CommonDefs.h>
#include <com/amazonaws/kinesis/video/common/PlatformUtils.h>

#define MAX_STRING_CONVERSION_BASE          36

// Max path characters as defined in linux/limits.h
#define MAX_PATH_LEN                        4096

// Check for whitespace
#define IS_WHITE_SPACE(ch)          (((ch) == ' ') || ((ch) == '\t') || ((ch) == '\r') || ((ch) == '\n') || ((ch) == '\v') ||  ((ch) == '\f'))

/**
 * Error values
 */
#define STATUS_UTILS_BASE                           0x40000000
#define STATUS_INVALID_BASE64_ENCODE                STATUS_UTILS_BASE + 0x00000001
#define STATUS_INVALID_BASE                         STATUS_UTILS_BASE + 0x00000002
#define STATUS_INVALID_DIGIT                        STATUS_UTILS_BASE + 0x00000003
#define STATUS_INT_OVERFLOW                         STATUS_UTILS_BASE + 0x00000004
#define STATUS_EMPTY_STRING                         STATUS_UTILS_BASE + 0x00000005
#define STATUS_DIRECTORY_OPEN_FAILED                STATUS_UTILS_BASE + 0x00000006
#define STATUS_PATH_TOO_LONG                        STATUS_UTILS_BASE + 0x00000007
#define STATUS_UNKNOWN_DIR_ENTRY_TYPE               STATUS_UTILS_BASE + 0x00000008
#define STATUS_REMOVE_DIRECTORY_FAILED              STATUS_UTILS_BASE + 0x00000009
#define STATUS_REMOVE_FILE_FAILED                   STATUS_UTILS_BASE + 0x0000000a
#define STATUS_REMOVE_LINK_FAILED                   STATUS_UTILS_BASE + 0x0000000b
#define STATUS_DIRECTORY_ACCESS_DENIED              STATUS_UTILS_BASE + 0x0000000c
#define STATUS_DIRECTORY_MISSING_PATH               STATUS_UTILS_BASE + 0x0000000d
#define STATUS_DIRECTORY_ENTRY_STAT_ERROR           STATUS_UTILS_BASE + 0x0000000e

/**
 * Base64 encode/decode functionality
 */
PUBLIC_API STATUS base64Encode(PVOID, UINT32, PCHAR, PUINT32);
PUBLIC_API STATUS base64Decode(PCHAR, PBYTE, PUINT32);

/**
 * Hex encode/decode functionality
 */
PUBLIC_API STATUS hexEncode(PVOID, UINT32, PCHAR, PUINT32);
PUBLIC_API STATUS hexDecode(PCHAR, PBYTE, PUINT32);

/**
 * Integer to string conversion routines
 */
PUBLIC_API STATUS ulltostr(UINT64, PCHAR, UINT32, UINT32, PUINT32);
PUBLIC_API STATUS ultostr(UINT32, PCHAR, UINT32, UINT32, PUINT32);

/**
 * String to integer conversion routines. NOTE: The base is in [2-36]
 */
PUBLIC_API STATUS strtoui32(PCHAR, PCHAR, UINT32, PUINT32);
PUBLIC_API STATUS strtoui64(PCHAR, PCHAR, UINT32, PUINT64);
PUBLIC_API STATUS strtoi32(PCHAR, PCHAR, UINT32, PINT32);
PUBLIC_API STATUS strtoi64(PCHAR, PCHAR, UINT32, PINT64);

/**
 * File I/O functionality
 */
PUBLIC_API STATUS readFile(PCHAR filePath, BOOL binMode, PBYTE pBuffer, PUINT64 pSize);
PUBLIC_API STATUS readFileSegment(PCHAR filePath, BOOL binMode, PBYTE pBuffer, UINT64 offset, UINT64 readSize);
PUBLIC_API STATUS writeFile(PCHAR filePath, BOOL binMode, PBYTE pBuffer, UINT64 size);
PUBLIC_API STATUS getFileLength(PCHAR filePath, PUINT64 pSize);
PUBLIC_API STATUS fileExists(PCHAR filePath, PBOOL pExists);
PUBLIC_API STATUS createFile(PCHAR filePath, UINT64 size);

/////////////////////////////////////////
// Directory functionality
/////////////////////////////////////////

typedef enum {
    DIR_ENTRY_TYPE_FILE,
    DIR_ENTRY_TYPE_LINK,
    DIR_ENTRY_TYPE_DIRECTORY,
    DIR_ENTRY_TYPE_UNKNOWN
} DIR_ENTRY_TYPES;

/**
 * Callback function declaration.
 *
 * @UINT64 - the caller passed data
 * @DIR_ENTRY_TYPES - the type of the entry
 * @PCHAR - the full path of the entry
 * @PCHAR - the name of the entry
 */
typedef STATUS (*DirectoryEntryCallbackFunc)(UINT64, DIR_ENTRY_TYPES, PCHAR, PCHAR);

/**
 * Remove a directory - empty or not
 *
 * @PCHAR - directory path
 * @UINT64 - custom caller data passed to the callback
 * @BOOL - whether to iterate
 * @DirectoryEntryCallbackFunc - the callback function called with each entry
 */
PUBLIC_API STATUS traverseDirectory(PCHAR, UINT64, BOOL iterate, DirectoryEntryCallbackFunc);

/**
 * Remove a directory - empty or not
 *
 * @PCHAR - directory path
 */
PUBLIC_API STATUS removeDirectory(PCHAR);

/**
 * Gets the directory size
 *
 * @PCHAR - directory path
 * @PUINT64 - returned combined size
 */
PUBLIC_API STATUS getDirectorySize(PCHAR, PUINT64);

/**
 * Double-linked list definition
 */
typedef struct __DoubleListNode {
    __DoubleListNode* pNext;
    __DoubleListNode* pPrev;
    UINT64 data;
} DoubleListNode, *PDoubleListNode;

typedef struct {
    UINT32 count;
    PDoubleListNode pHead;
    PDoubleListNode pTail;
} DoubleList, *PDoubleList;

typedef struct __SingleListNode {
    __SingleListNode* pNext;
    UINT64 data;
} SingleListNode, *PSingleListNode;

typedef struct {
    UINT32 count;
    PSingleListNode pHead;
    PSingleListNode pTail;
} SingleList, *PSingleList;

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Double-linked list functionality
//////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Create a new double linked list
 */
PUBLIC_API STATUS doubleListCreate(PDoubleList*);

/**
 * Frees a double linked list and deallocates the nodes
 */
PUBLIC_API STATUS doubleListFree(PDoubleList);

/**
 * Clears and deallocates all the items
 */
PUBLIC_API STATUS doubleListClear(PDoubleList);

/**
 * Insert a node in the head position in the list
 */
PUBLIC_API STATUS doubleListInsertNodeHead(PDoubleList, PDoubleListNode);

/**
 * Insert a new node with the data at the head position in the list
 */
PUBLIC_API STATUS doubleListInsertItemHead(PDoubleList, UINT64);

/**
 * Insert a node in the tail position in the list
 */
PUBLIC_API STATUS doubleListInsertNodeTail(PDoubleList, PDoubleListNode);

/**
 * Insert a new node with the data at the tail position in the list
 */
PUBLIC_API STATUS doubleListInsertItemTail(PDoubleList, UINT64);

/**
 * Insert a node before a given node
 */
PUBLIC_API STATUS doubleListInsertNodeBefore(PDoubleList, PDoubleListNode, PDoubleListNode);

/**
 * Insert a new node with the data before a given node
 */
PUBLIC_API STATUS doubleListInsertItemBefore(PDoubleList, PDoubleListNode, UINT64);

/**
 * Insert a node after a given node
 */
PUBLIC_API STATUS doubleListInsertNodeAfter(PDoubleList, PDoubleListNode, PDoubleListNode);

/**
 * Insert a new node with the data after a given node
 */
PUBLIC_API STATUS doubleListInsertItemAfter(PDoubleList, PDoubleListNode, UINT64);

/**
 * Removes and deletes the head
 */
PUBLIC_API STATUS doubleListDeleteHead(PDoubleList);

/**
 * Removes and deletes the tail
 */
PUBLIC_API STATUS doubleListDeleteTail(PDoubleList);

/**
 * Removes the specified node
 */
PUBLIC_API STATUS doubleListRemoveNode(PDoubleList, PDoubleListNode);

/**
 * Removes and deletes the specified node
 */
PUBLIC_API STATUS doubleListDeleteNode(PDoubleList, PDoubleListNode);

/**
 * Gets the head node
 */
PUBLIC_API STATUS doubleListGetHeadNode(PDoubleList, PDoubleListNode*);

/**
 * Gets the tail node
 */
PUBLIC_API STATUS doubleListGetTailNode(PDoubleList, PDoubleListNode*);

/**
 * Gets the node at the specified index
 */
PUBLIC_API STATUS doubleListGetNodeAt(PDoubleList, UINT32, PDoubleListNode*);

/**
 * Gets the node data at the specified index
 */
PUBLIC_API STATUS doubleListGetNodeDataAt(PDoubleList, UINT32, PUINT64);

/**
 * Gets the node data
 */
PUBLIC_API STATUS doubleListGetNodeData(PDoubleListNode, PUINT64);

/**
 * Gets the next node
 */
PUBLIC_API STATUS doubleListGetNextNode(PDoubleListNode, PDoubleListNode*);

/**
 * Gets the previous node
 */
PUBLIC_API STATUS doubleListGetPrevNode(PDoubleListNode, PDoubleListNode*);

/**
 * Gets the count of nodes in the list
 */
PUBLIC_API STATUS doubleListGetNodeCount(PDoubleList, PUINT32);

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Single-linked list functionality
//////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Create a new single linked list
 */
PUBLIC_API STATUS singleListCreate(PSingleList*);

/**
 * Frees a single linked list and deallocates the nodes
 */
PUBLIC_API STATUS singleListFree(PSingleList);

/**
 * Clears and deallocates all the items
 */
PUBLIC_API STATUS singleListClear(PSingleList);

/**
 * Insert a node in the head position in the list
 */
PUBLIC_API STATUS singleListInsertNodeHead(PSingleList, PSingleListNode);

/**
 * Insert a new node with the data at the head position in the list
 */
PUBLIC_API STATUS singleListInsertItemHead(PSingleList, UINT64);

/**
 * Insert a node in the tail position in the list
 */
PUBLIC_API STATUS singleListInsertNodeTail(PSingleList, PSingleListNode);

/**
 * Insert a new node with the data at the tail position in the list
 */
PUBLIC_API STATUS singleListInsertItemTail(PSingleList, UINT64);

/**
 * Insert a node after a given node
 */
PUBLIC_API STATUS singleListInsertNodeAfter(PSingleList, PSingleListNode, PSingleListNode);

/**
 * Insert a new node with the data after a given node
 */
PUBLIC_API STATUS singleListInsertItemAfter(PSingleList, PSingleListNode, UINT64);

/**
 * Removes and deletes the head
 */
PUBLIC_API STATUS singleListDeleteHead(PSingleList);

/**
 * Removes and deletes the specified node
 */
PUBLIC_API STATUS singleListDeleteNode(PSingleList, PSingleListNode);

/**
 * Removes and deletes the next node of the specified node
 */
PUBLIC_API STATUS singleListDeleteNextNode(PSingleList, PSingleListNode);

/**
 * Gets the head node
 */
PUBLIC_API STATUS singleListGetHeadNode(PSingleList, PSingleListNode*);

/**
 * Gets the tail node
 */
PUBLIC_API STATUS singleListGetTailNode(PSingleList, PSingleListNode*);

/**
 * Gets the node at the specified index
 */
PUBLIC_API STATUS singleListGetNodeAt(PSingleList, UINT32, PSingleListNode*);

/**
 * Gets the node data at the specified index
 */
PUBLIC_API STATUS singleListGetNodeDataAt(PSingleList, UINT32, PUINT64);

/**
 * Gets the node data
 */
PUBLIC_API STATUS singleListGetNodeData(PSingleListNode, PUINT64);

/**
 * Gets the next node
 */
PUBLIC_API STATUS singleListGetNextNode(PSingleListNode, PSingleListNode*);

/**
 * Gets the count of nodes in the list
 */
PUBLIC_API STATUS singleListGetNodeCount(PSingleList, PUINT32);

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Stack/Queue functionality
//////////////////////////////////////////////////////////////////////////////////////////////////////

typedef SingleList StackQueue;
typedef PSingleList PStackQueue;
typedef PSingleListNode StackQueueIterator;
typedef StackQueueIterator* PStackQueueIterator;

#define IS_VALID_ITERATOR(x)    ((x) != NULL)

/**
 * Create a new stack queue
 */
PUBLIC_API STATUS stackQueueCreate(PStackQueue*);

/**
 * Frees and de-allocates the stack queue
 */
PUBLIC_API STATUS stackQueueFree(PStackQueue);

/**
 * Clears and de-allocates all the items
 */
PUBLIC_API STATUS stackQueueClear(PStackQueue);

/**
 * Gets the number of items in the stack/queue
 */
PUBLIC_API STATUS stackQueueGetCount(PStackQueue, PUINT32);

/**
 * Gets the item at the given index
 */
PUBLIC_API STATUS stackQueueGetAt(PStackQueue, UINT32, PUINT64);

/**
 * Sets the item value at the given index
 */
PUBLIC_API STATUS stackQueueSetAt(PStackQueue, UINT32, UINT64);

/**
 * Gets the index of an item
 */
PUBLIC_API STATUS stackQueueGetIndexOf(PStackQueue, UINT64, PUINT32);

/**
 * Removes the item at the given index
 */
PUBLIC_API STATUS stackQueueRemoveAt(PStackQueue, UINT32);

/**
 * Removes the item at the given item
 */
PUBLIC_API STATUS stackQueueRemoveItem(PStackQueue, UINT64);

/**
 * Whether the stack queue is empty
 */
PUBLIC_API STATUS stackQueueIsEmpty(PStackQueue, PBOOL);

/**
 * Pushes an item onto the stack
 */
PUBLIC_API STATUS stackQueuePush(PStackQueue, UINT64);

/**
 * Pops an item from the stack
 */
PUBLIC_API STATUS stackQueuePop(PStackQueue, PUINT64);

/**
 * Peeks an item from the stack without popping
 */
PUBLIC_API STATUS stackQueuePeek(PStackQueue, PUINT64);

/**
 * Enqueues an item in the queue
 */
PUBLIC_API STATUS stackQueueEnqueue(PStackQueue, UINT64);

/**
 * Dequeues an item from the queue
 */
PUBLIC_API STATUS stackQueueDequeue(PStackQueue, PUINT64);

/**
 * Gets the iterator
 */
PUBLIC_API STATUS stackQueueGetIterator(PStackQueue, PStackQueueIterator);

/**
 * Iterates to next
 */
PUBLIC_API STATUS stackQueueIteratorNext(PStackQueueIterator);

/**
 * Gets the data
 */
PUBLIC_API STATUS stackQueueIteratorGetItem(StackQueueIterator, PUINT64);

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Hash table functionality
//////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Hash table declaration
 * NOTE: Variable size structure - the buckets follow directly after the main structure
 */
typedef struct {
    UINT32 itemCount;
    UINT32 bucketCount;
    UINT32 bucketLength;
    /*-- HashBucket[bucketCount] buckets; --*/
} HashTable, *PHashTable;

/**
 * Hash table entry declaration
 */
typedef struct {
    UINT64 key;
    UINT64 value;
} HashEntry, *PHashEntry;

/**
 * Minimum number of buckets
 */
#define MIN_HASH_BUCKET_COUNT                   16

/**
 * Hash table iteration callback function.
 * IMPORTANT!
 * To terminate the iteration the caller must return
 * STATUS_HASH_ENTRY_ITERATION_ABORT
 *
 * @UINT64 - data that was passed to the iterate function
 * @PHashEntry - the entry to process
 */
typedef STATUS (*HashEntryCallbackFunc)(UINT64, PHashEntry);

/**
 * Error values
 */
#define STATUS_HASH_TABLE_BASE                      STATUS_UTILS_BASE + 0x00100000
#define STATUS_HASH_KEY_NOT_PRESENT                 STATUS_HASH_TABLE_BASE + 0x00000001
#define STATUS_HASH_KEY_ALREADY_PRESENT             STATUS_HASH_TABLE_BASE + 0x00000002
#define STATUS_HASH_ENTRY_ITERATION_ABORT           STATUS_HASH_TABLE_BASE + 0x00000003

/**
 * Create a new hash table with default parameters
 */
PUBLIC_API STATUS hashTableCreate(PHashTable*);

/**
 * Create a new hash table with specific parameters
 */
PUBLIC_API STATUS hashTableCreateWithParams(UINT32, UINT32, PHashTable*);

/**
 * Frees and de-allocates the hash table
 */
PUBLIC_API STATUS hashTableFree(PHashTable);

/**
 * Clears all the items and the buckets
 */
PUBLIC_API STATUS hashTableClear(PHashTable);

/**
 * Gets the number of items in the hash table
 */
PUBLIC_API STATUS hashTableGetCount(PHashTable, PUINT32);

/**
 * Whether the hash table is empty
 */
PUBLIC_API STATUS hashTableIsEmpty(PHashTable, PBOOL);

/**
 * Puts an item into the hash table
 */
PUBLIC_API STATUS hashTablePut(PHashTable, UINT64, UINT64);

/**
 * Upserts an item into the hash table
 */
PUBLIC_API STATUS hashTableUpsert(PHashTable, UINT64, UINT64);

/**
 * Gets an item from the hash table
 */
PUBLIC_API STATUS hashTableGet(PHashTable, UINT64, PUINT64);

/**
 * Checks whether an item exists in the hash table
 */
PUBLIC_API STATUS hashTableContains(PHashTable, UINT64, PBOOL);

/**
 * Removes an item from the hash table. If the bucket is empty it's deleted. The existing items will be shifted.
 */
PUBLIC_API STATUS hashTableRemove(PHashTable, UINT64);

/**
 * Gets the number of buckets
 */
PUBLIC_API STATUS hashTableGetBucketCount(PHashTable, PUINT32);

/**
 * Gets all the entries from the hash table
 */
PUBLIC_API STATUS hashTableGetAllEntries(PHashTable, PHashEntry, PUINT32);

/**
 * Iterates over the hash entries. No predefined order
 */
PUBLIC_API STATUS hashTableIterateEntries(PHashTable, UINT64, HashEntryCallbackFunc);

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitfield functionality
//////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Returns the byte count
 */
#define GET_BYTE_COUNT_FOR_BITS(x) ((((x) + 7) & ~7) >> 3)

/**
 * Bit field declaration
 * NOTE: Variable size structure - the bit field follow directly after the main structure
 */
typedef struct {
    UINT32 itemCount;
    /*-- BYTE[byteCount] bits; --*/
} BitField, *PBitField;

/**
 * Create a new bit field with all bits set to 0
 */
PUBLIC_API STATUS bitFieldCreate(UINT32, PBitField*);

/**
 * Frees and de-allocates the bit field object
 */
PUBLIC_API STATUS bitFieldFree(PBitField);

/**
 * Sets or clears all the bits
 */
PUBLIC_API STATUS bitFieldReset(PBitField, BOOL);

/**
 * Gets the bit field size in items
 */
PUBLIC_API STATUS bitFieldGetCount(PBitField, PUINT32);

/**
 * Gets the value of the bit
 */
PUBLIC_API STATUS bitFieldGet(PBitField, UINT32, PBOOL);

/**
 * Sets the value of the bit
 */
PUBLIC_API STATUS bitFieldSet(PBitField, UINT32, BOOL);

//////////////////////////////////////////////////////////////////////////////////////////////////////
// BitBuffer reader functionality
//////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Error values
 */
#define STATUS_BIT_READER_BASE                      STATUS_UTILS_BASE + 0x01000000
#define STATUS_BIT_READER_OUT_OF_RANGE              STATUS_BIT_READER_BASE + 0x00000001
#define STATUS_BIT_READER_INVALID_SIZE              STATUS_BIT_READER_BASE + 0x00000002

/**
 * Bit Buffer reader declaration
 */
typedef struct {
    // Bit buffer
    PBYTE buffer;

    // Size of the buffer in bits
    UINT32 bitBufferSize;

    // Current bit
    UINT32 currentBit;
} BitReader, *PBitReader;

/**
 * Resets the bit reader object
 */
PUBLIC_API STATUS bitReaderReset(PBitReader, PBYTE, UINT32);

/**
 * Set current pointer
 */
PUBLIC_API STATUS bitReaderSetCurrent(PBitReader, UINT32);

/**
 * Read a bit from the current pointer
 */
PUBLIC_API STATUS bitReaderReadBit(PBitReader, PUINT32);

/**
 * Read up-to 32 bits from the current pointer
 */
PUBLIC_API STATUS bitReaderReadBits(PBitReader, UINT32, PUINT32);

/**
 * Read the Exponential Golomb encoded number from the current position
 */
PUBLIC_API STATUS bitReaderReadExpGolomb(PBitReader, PUINT32);

/**
 * Read the Exponential Golomb encoded signed number from the current position
 */
PUBLIC_API STATUS bitReaderReadExpGolombSe(PBitReader, PINT32);

////////////////////////////////////////////////////
// Endianness functionality
////////////////////////////////////////////////////
typedef INT16(*getInt16Func) (INT16);
typedef INT32(*getInt32Func) (INT32);
typedef INT64(*getInt64Func) (INT64);
typedef VOID(*putInt16Func) (PINT16, INT16);
typedef VOID(*putInt32Func) (PINT32, INT32);
typedef VOID(*putInt64Func) (PINT64, INT64);

extern getInt16Func getInt16;
extern getInt32Func getInt32;
extern getInt64Func getInt64;
extern putInt16Func putInt16;
extern putInt32Func putInt32;
extern putInt64Func putInt64;

PUBLIC_API BOOL isBigEndian();
PUBLIC_API VOID initializeEndianness();

////////////////////////////////////////////////////
// Dumping memory functionality
////////////////////////////////////////////////////
VOID dumpMemoryHex(PVOID, UINT32);

#ifdef __cplusplus
}
#endif

#endif // __DASH_UTILS_H__
