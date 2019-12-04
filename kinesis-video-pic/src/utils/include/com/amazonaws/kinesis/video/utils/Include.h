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

// For tight packing
#pragma pack(push, include, 1) // for byte alignment

#define MAX_STRING_CONVERSION_BASE          36

// Max path characters as defined in linux/limits.h
#define MAX_PATH_LEN                        4096

// thread stack size to use when running on constrained device like raspberry pi
#define THREAD_STACK_SIZE_ON_CONSTRAINED_DEVICE     (512 * 1024)

// Check for whitespace
#define IS_WHITE_SPACE(ch)          (((ch) == ' ') || ((ch) == '\t') || ((ch) == '\r') || ((ch) == '\n') || ((ch) == '\v') ||  ((ch) == '\f'))

/**
 * Error values
 */
#define STATUS_UTILS_BASE                               0x40000000
#define STATUS_INVALID_BASE64_ENCODE                    STATUS_UTILS_BASE + 0x00000001
#define STATUS_INVALID_BASE                             STATUS_UTILS_BASE + 0x00000002
#define STATUS_INVALID_DIGIT                            STATUS_UTILS_BASE + 0x00000003
#define STATUS_INT_OVERFLOW                             STATUS_UTILS_BASE + 0x00000004
#define STATUS_EMPTY_STRING                             STATUS_UTILS_BASE + 0x00000005
#define STATUS_DIRECTORY_OPEN_FAILED                    STATUS_UTILS_BASE + 0x00000006
#define STATUS_PATH_TOO_LONG                            STATUS_UTILS_BASE + 0x00000007
#define STATUS_UNKNOWN_DIR_ENTRY_TYPE                   STATUS_UTILS_BASE + 0x00000008
#define STATUS_REMOVE_DIRECTORY_FAILED                  STATUS_UTILS_BASE + 0x00000009
#define STATUS_REMOVE_FILE_FAILED                       STATUS_UTILS_BASE + 0x0000000a
#define STATUS_REMOVE_LINK_FAILED                       STATUS_UTILS_BASE + 0x0000000b
#define STATUS_DIRECTORY_ACCESS_DENIED                  STATUS_UTILS_BASE + 0x0000000c
#define STATUS_DIRECTORY_MISSING_PATH                   STATUS_UTILS_BASE + 0x0000000d
#define STATUS_DIRECTORY_ENTRY_STAT_ERROR               STATUS_UTILS_BASE + 0x0000000e
#define STATUS_STRFTIME_FALIED                          STATUS_UTILS_BASE + 0x0000000f
#define STATUS_MAX_TIMESTAMP_FORMAT_STR_LEN_EXCEEDED    STATUS_UTILS_BASE + 0x00000010
#define STATUS_UTIL_MAX_TAG_COUNT                       STATUS_UTILS_BASE + 0x00000011
#define STATUS_UTIL_INVALID_TAG_VERSION                 STATUS_UTILS_BASE + 0x00000012
#define STATUS_UTIL_TAGS_COUNT_NON_ZERO_TAGS_NULL       STATUS_UTILS_BASE + 0x00000013
#define STATUS_UTIL_INVALID_TAG_NAME_LEN                STATUS_UTILS_BASE + 0x00000014
#define STATUS_UTIL_INVALID_TAG_VALUE_LEN               STATUS_UTILS_BASE + 0x00000015

/**
 * Base64 encode/decode functionality
 */
PUBLIC_API STATUS base64Encode(PVOID, UINT32, PCHAR, PUINT32);
PUBLIC_API STATUS base64Decode(PCHAR, UINT32, PBYTE, PUINT32);

/**
 * Hex encode/decode functionality
 */
PUBLIC_API STATUS hexEncode(PVOID, UINT32, PCHAR, PUINT32);
PUBLIC_API STATUS hexEncodeCase(PVOID, UINT32, PCHAR, PUINT32, BOOL);
PUBLIC_API STATUS hexDecode(PCHAR, UINT32, PBYTE, PUINT32);

/**
 * Integer to string conversion routines
 */
PUBLIC_API STATUS ulltostr(UINT64, PCHAR, UINT32, UINT32, PUINT32);
PUBLIC_API STATUS ultostr(UINT32, PCHAR, UINT32, UINT32, PUINT32);

/**
 * String to integer conversion routines. NOTE: The base is in [2-36]
 *
 * @param 1 - IN - Input string to process
 * @param 2 - IN/OPT - Pointer to the end of the string. If NULL, the NULL terminator would be used
 * @param 3 - IN - Base of the number (10 - for decimal)
 * @param 4 - OUT - The resulting value
 */
PUBLIC_API STATUS strtoui32(PCHAR, PCHAR, UINT32, PUINT32);
PUBLIC_API STATUS strtoui64(PCHAR, PCHAR, UINT32, PUINT64);
PUBLIC_API STATUS strtoi32(PCHAR, PCHAR, UINT32, PINT32);
PUBLIC_API STATUS strtoi64(PCHAR, PCHAR, UINT32, PINT64);

/**
 * Safe variant of strchr
 *
 * @param 1 - IN - Input string to process
 * @param 2 - IN/OPT - String length. 0 if NULL terminated and the length is calculated.
 * @param 3 - IN - The character to look for
 *
 * @return - Pointer to the first occurrence or NULL
 */
PUBLIC_API PCHAR strnchr(PCHAR, UINT32, CHAR);

/**
 * Left and right trim of the whitespace
 *
 * @param 1 - IN - Input string to process
 * @param 2 - IN/OPT - String length. 0 if NULL terminated and the length is calculated.
 * @param 3 and 4 - OUT - The pointer to the trimmed start and/or end
 *
 * @return Status of the operation
 */
PUBLIC_API STATUS ltrimstr(PCHAR, UINT32, PCHAR*);
PUBLIC_API STATUS rtrimstr(PCHAR, UINT32, PCHAR*);
PUBLIC_API STATUS trimstrall(PCHAR, UINT32, PCHAR*, PCHAR*);

/**
 * To lower and to upper string conversion
 *
 * @param 1 - IN - Input string to convert
 * @param 2 - IN - String length. 0 if NULL terminated and the length is calculated.
 * @param 3 - OUT - The pointer to the converted string - can be pointing to the same location. Size should be enough
 *
 * @return Status of the operation
 */
PUBLIC_API STATUS tolowerstr(PCHAR, UINT32, PCHAR);
PUBLIC_API STATUS toupperstr(PCHAR, UINT32, PCHAR);

/**
 * To lower/upper string conversion internal function
 *
 * @param 1 - IN - Input string to convert
 * @param 2 - IN - String length. 0 if NULL terminated and the length is calculated.
 * @param 3 - INT - Whether to upper (TRUE) or to lower (FALSE)
 * @param 4 - OUT - The pointer to the converted string - can be pointing to the same location. Size should be enough
 *
 * @return Status of the operation
 */
STATUS tolowerupperstr(PCHAR, UINT32, BOOL, PCHAR);

/**
 * File I/O functionality
 */
PUBLIC_API STATUS readFile(PCHAR filePath, BOOL binMode, PBYTE pBuffer, PUINT64 pSize);
PUBLIC_API STATUS readFileSegment(PCHAR filePath, BOOL binMode, PBYTE pBuffer, UINT64 offset, UINT64 readSize);
PUBLIC_API STATUS writeFile(PCHAR filePath, BOOL binMode, BOOL append, PBYTE pBuffer, UINT64 size);
PUBLIC_API STATUS getFileLength(PCHAR filePath, PUINT64 pSize);
PUBLIC_API STATUS fileExists(PCHAR filePath, PBOOL pExists);
PUBLIC_API STATUS createFile(PCHAR filePath, UINT64 size);

/////////////////////////////////////////
// Tags functionality
/////////////////////////////////////////

/**
 * Max tag count
 */
#define MAX_TAG_COUNT                            50

/**
 * Max tag name length in chars
 */
#define MAX_TAG_NAME_LEN                         128

/**
 * Max tag value length in chars
 */
#define MAX_TAG_VALUE_LEN                        1024

/**
 * Defines the full tag structure length when the pointers to the strings are allocated after the
 * main struct. We will add 2 for NULL terminators
 */
#define TAG_FULL_LENGTH                         (SIZEOF(Tag) + (MAX_TAG_NAME_LEN + MAX_TAG_VALUE_LEN + 2) * SIZEOF(CHAR))

/**
 * Current version of the tag structure
 */
#define TAG_CURRENT_VERSION                                 0

/**
 * Tag declaration
 */
typedef struct __Tag Tag;
struct __Tag {
    // Version of the struct
    UINT32 version;

    // Tag name - null terminated
    PCHAR name; // pointer to a string with MAX_TAG_NAME_LEN chars max including the NULL terminator

    // Tag value - null terminated
    PCHAR value; // pointer to a string with MAX_TAG_VALUE_LEN chars max including the NULL terminator
};
typedef struct __Tag* PTag;

/**
 * Validates the tags
 *
 * @param 1 UINT32 - IN - Number of tags
 * @param 2 PTag - IN - Array of tags
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS validateTags(UINT32, PTag);

/**
 * Packages the tags in a provided buffer
 *
 * @param 1 UINT32 - IN - Number of tags
 * @param 2 PTag - IN - Array of tags
 * @param 3 UINT32 - IN - Buffer size
 * @param 4 PTag - IN/OUT - Buffer to package in
 * @param 5 PUINT32 - OUT/OPT - Real size of the bytes needed
 *
 * @return Status of the function call.
 */
PUBLIC_API STATUS packageTags(UINT32, PTag, UINT32, PTag, PUINT32);

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
    struct __DoubleListNode* pNext;
    struct __DoubleListNode* pPrev;
    UINT64 data;
} DoubleListNode, *PDoubleListNode;

typedef struct {
    UINT32 count;
    PDoubleListNode pHead;
    PDoubleListNode pTail;
} DoubleList, *PDoubleList;

typedef struct __SingleListNode {
    struct __SingleListNode* pNext;
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
PUBLIC_API STATUS doubleListClear(PDoubleList, BOOL);

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

/**
 * Append a double list to the other and then free the list being appended
 */
PUBLIC_API STATUS doubleListAppendList(PDoubleList, PDoubleList*);

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
PUBLIC_API STATUS singleListClear(PSingleList, BOOL);

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

/**
 * Append a single list to the other and then free the list being appended
 */
STATUS singleListAppendList(PSingleList, PSingleList*);

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
PUBLIC_API STATUS stackQueueClear(PStackQueue, BOOL);

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
 * Hash table error values starting from 0x40100000
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
 * Bit reader error values starting from 0x41000000
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

////////////////////////////////////////////////////
// Check memory content
////////////////////////////////////////////////////
BOOL checkBufferValues(PVOID, BYTE, SIZE_T);

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Time functionality
//////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @UINT64  - IN - timestamp in 100ns to be converted to string
 * @PCHAR   - IN - timestamp format string
 * @PCHAR   - IN - buffer to hold the resulting string
 * @UINT32  - IN - buffer size
 * @PUINT32 - OUT - actual number of characters in the result string not including null terminator
 * @return  - STATUS code of the execution
 */
PUBLIC_API STATUS generateTimestampStr(UINT64, PCHAR, PCHAR, UINT32, PUINT32);

// yyyy-mm-dd HH:MM:SS
#define MAX_TIMESTAMP_FORMAT_STR_LEN                    19

// Max timestamp string length including null terminator
#define MAX_TIMESTAMP_STR_LEN                           17

// (thread-0x7000076b3000)
#define MAX_THREAD_ID_STR_LEN                           23

// Max log message length
#define MAX_LOG_FORMAT_LENGTH                           600

// Set the global log level
#define SET_LOGGER_LOG_LEVEL(l)                         loggerSetLogLevel((l))

// Get the global log level
#define GET_LOGGER_LOG_LEVEL()                          loggerGetLogLevel()

/*
 * Set log level
 * @UINT32 - IN - target log level
 */
PUBLIC_API VOID loggerSetLogLevel(UINT32);

/**
 * Get current log level
 * @return - UINT32 - current log level
 */
PUBLIC_API UINT32 loggerGetLogLevel();

/**
 * Prepend log message with timestamp and thread id.
 * @PCHAR - IN - buffer holding the log
 * @UINT32 - IN - buffer length
 * @PCHAR - IN - log format string
 * @return - VOID
 */
PUBLIC_API VOID addLogMetadata(PCHAR, UINT32, PCHAR);

/**
 * Updates a CRC32 checksum
 * @UINT32 - IN - initial checksum result from previous update; for the first call, it should be 0.
 * @PBYTE - IN - buffer used to compute checksum
 * @UINT32 - IN - number of bytes to use from buffer
 * @return - UINT32 crc32 checksum
 */
PUBLIC_API UINT32 updateCrc32(UINT32, PBYTE, UINT32);

/**
 * @PBYTE - IN - buffer used to compute checksum
 * @UINT32 - IN - number of bytes to use from buffer
 * @return - UINT32 crc32 checksum
 */
#define COMPUTE_CRC32(pBuffer, len) (updateCrc32(0, pBuffer, len))

//////////////////////////////////////////////////////////////////////////////////////////////////////
// TimerQueue functionality
//////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Minimum number of timers in the timer queue
 */
#define MIN_TIMER_QUEUE_TIMER_COUNT                                 1

/**
 * Default timer queue max timer count
 */
#define DEFAULT_TIMER_QUEUE_TIMER_COUNT                             32

/**
 * Sentinel value to specify no periodic invocation
 */
#define TIMER_QUEUE_SINGLE_INVOCATION_PERIOD                        0

/**
 * Shortest period value to schedule the call
 */
#define MIN_TIMER_QUEUE_PERIOD_DURATION                             (1 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

/**
 * Definition of the TimerQueue handle
 */
typedef UINT64 TIMER_QUEUE_HANDLE;
typedef TIMER_QUEUE_HANDLE* PTIMER_QUEUE_HANDLE;

/**
 * This is a sentinel indicating an invalid handle value
 */
#ifndef INVALID_TIMER_QUEUE_HANDLE_VALUE
#define INVALID_TIMER_QUEUE_HANDLE_VALUE ((TIMER_QUEUE_HANDLE) INVALID_PIC_HANDLE_VALUE)
#endif

/**
 * Checks for the handle validity
 */
#ifndef IS_VALID_TIMER_QUEUE_HANDLE
#define IS_VALID_TIMER_QUEUE_HANDLE(h) ((h) != INVALID_TIMER_QUEUE_HANDLE_VALUE)
#endif

/**
 * Timer queue callback
 *
 * IMPORTANT!!!
 * The callback should be 'prompt' - any lengthy or blocking operations should be executed
 * in their own execution unit - aka thread.
 *
 * NOTE: To terminate the scheduling of the calls return STATUS_TIMER_QUEUE_STOP_SCHEDULING
 * NOTE: Returning other non-STATUS_SUCCESS status will issue a warning but the timer will
 * still continue to be scheduled.
 *
 * @UINT32 - Timer ID that's fired
 * @UINT64 - Current time the scheduling is triggered
 * @UINT64 - Data that was passed to the timer function
 *
 */
typedef STATUS (*TimerCallbackFunc)(UINT32, UINT64, UINT64);

/**
 * Timer queue error values starting from 0x41100000
 */
#define STATUS_TIMER_QUEUE_BASE                                     STATUS_UTILS_BASE + 0x01100000
#define STATUS_TIMER_QUEUE_STOP_SCHEDULING                          STATUS_TIMER_QUEUE_BASE + 0x00000001
#define STATUS_INVALID_TIMER_COUNT_VALUE                            STATUS_TIMER_QUEUE_BASE + 0x00000002
#define STATUS_INVALID_TIMER_PERIOD_VALUE                           STATUS_TIMER_QUEUE_BASE + 0x00000003
#define STATUS_MAX_TIMER_COUNT_REACHED                              STATUS_TIMER_QUEUE_BASE + 0x00000004

/**
 * @param - PTIMER_QUEUE_HANDLE - OUT - Timer queue handle
 *
 * @return  - STATUS code of the execution
 */
PUBLIC_API STATUS timerQueueCreate(PTIMER_QUEUE_HANDLE);

/*
 * Frees the Timer queue object
 *
 * NOTE: The call is idempotent.
 *
 * @param - PTIMER_QUEUE_HANDLE - IN/OUT/OPT - Timer queue handle to free
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS timerQueueFree(PTIMER_QUEUE_HANDLE);

/*
 * Add timer to the timer queue.
 *
 * NOTE: The timer period value of TIMER_QUEUE_SINGLE_INVOCATION_PERIOD will schedule the call only once
 *
 * @param - TIMER_QUEUE_HANDLE - IN - Timer queue handle
 * @param - UINT64 - IN - Start duration in 100ns at which to start the first time
 * @param - UINT64 - IN - Timer period value in 100ns to schedule the callback
 * @param - TimerCallbackFunc - IN - Callback to call for the timer
 * @param - UINT64 - IN - Timer callback function custom data
 * @param - PUINT32 - IN - Created timers ID
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS timerQueueAddTimer(TIMER_QUEUE_HANDLE, UINT64, UINT64, TimerCallbackFunc, UINT64, PUINT32);

/*
 * Cancel the timer. customData is needed to handle case when user 1 add timer and then the timer
 * get cancelled because the callback returned STATUS_TIMER_QUEUE_STOP_SCHEDULING. Then user 2 add
 * another timer but then user 1 cancel timeId it first received. Without checking custom data user 2's timer
 * would be deleted by user 1.
 *
 * @param - TIMER_QUEUE_HANDLE - IN - Timer queue handle
 * @param - UINT32 - IN - Timer id to cancel
 * @param - UINT64 - IN - provided customData. CustomData needs to match in order to successfully cancel.
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS timerQueueCancelTimer(TIMER_QUEUE_HANDLE, UINT32, UINT64);

/*
 * Cancel all timers with customData
 *
 * @param - TIMER_QUEUE_HANDLE - IN - Timer queue handle
 * @param - UINT64 - IN - provided customData.
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS timerQueueCancelTimersWithCustomData(TIMER_QUEUE_HANDLE, UINT64);

/*
 * Cancel all timers
 *
 * @param - TIMER_QUEUE_HANDLE - IN - Timer queue handle
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS timerQueueCancelAllTimers(TIMER_QUEUE_HANDLE);

/*
 * Get active timer count
 *
 * @param - TIMER_QUEUE_HANDLE - IN - Timer queue handle
 * @param - PUINT32 - OUT - pointer to store active timer count
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS timerQueueGetTimerCount(TIMER_QUEUE_HANDLE, PUINT32);

/*
 * Get timer ids with custom data
 *
 * @param - TIMER_QUEUE_HANDLE - IN - Timer queue handle
 * @param - UINT64 - IN - custom data to match
 * @param - PUINT32 - IN/OUT - size of the array passing in and will store the number of timer ids when the function returns
 * @param - PUINT32* - OUT/OPT - array that will store the timer ids. can be NULL.
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS timerQueueGetTimersWithCustomData(TIMER_QUEUE_HANDLE, UINT64, PUINT32, PUINT32);

/*
 * update timer id's period. Do nothing if timer not found.
 *
 * @param - TIMER_QUEUE_HANDLE - IN - Timer queue handle
 * @param - UINT64 - IN - custom data to match
 * @param - UINT32 - IN - Timer id to update
 * @param - UINT32 - IN - new period
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS timerQueueUpdateTimerPeriod(TIMER_QUEUE_HANDLE, UINT64, UINT32, UINT64);

/*
 * stop the timer. Once stopped timer can't be restarted. There will be no more timer callback invocation after
 * timerQueueShutdown returns.
 *
 * @param - TIMER_QUEUE_HANDLE - IN - Timer queue handle
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS timerQueueShutdown(TIMER_QUEUE_HANDLE);

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Semaphore functionality
//////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Semaphore shutdown timeout value
 */
#define SEMAPHORE_SHUTDOWN_TIMEOUT                                      (200 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

/**
 * Definition of the Semaphore handle
 */
typedef UINT64 SEMAPHORE_HANDLE;
typedef SEMAPHORE_HANDLE* PSEMAPHORE_HANDLE;

/**
 * This is a sentinel indicating an invalid handle value
 */
#ifndef INVALID_SEMAPHORE_HANDLE_VALUE
#define INVALID_SEMAPHORE_HANDLE_VALUE ((SEMAPHORE_HANDLE) INVALID_PIC_HANDLE_VALUE)
#endif

/**
 * Checks for the handle validity
 */
#ifndef IS_VALID_SEMAPHORE_HANDLE
#define IS_VALID_SEMAPHORE_HANDLE(h) ((h) != INVALID_SEMAPHORE_HANDLE_VALUE)
#endif

/**
 * Semaphore error values starting from 0x41200000
 */
#define STATUS_SEMAPHORE_BASE                                           STATUS_UTILS_BASE + 0x01200000
#define STATUS_SEMAPHORE_OPERATION_AFTER_SHUTDOWN                       STATUS_SEMAPHORE_BASE + 0x00000001
#define STATUS_SEMAPHORE_ACQUIRE_WHEN_LOCKED                            STATUS_SEMAPHORE_BASE + 0x00000002

/**
 * Create a semaphore object
 *
 * @param - UINT32 - IN - The permit count
 * @param - PSEMAPHORE_HANDLE - OUT - Semaphore handle
 *
 * @return  - STATUS code of the execution
 */
PUBLIC_API STATUS semaphoreCreate(UINT32, PSEMAPHORE_HANDLE);

/*
 * Frees the semaphore object releasing all the awaiting threads.
 *
 * NOTE: The call is idempotent.
 *
 * @param - PSEMAPHORE_HANDLE - IN/OUT/OPT - Semaphore handle to free
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS semaphoreFree(PSEMAPHORE_HANDLE);

/*
 * Acquires a semaphore. Will block for the specified amount of time before failing the acquisition.
 *
 * IMPORTANT NOTE: On failed acquire it will not increment the acquired count.
 *
 * @param - SEMAPHORE_HANDLE - IN - Semaphore handle
 * @param - UINT64 - IN - Time to wait to acquire in 100ns
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS semaphoreAcquire(SEMAPHORE_HANDLE, UINT64);

/*
 * Releases a semaphore. Blocked threads will be released to acquire the available slot
 *
 * @param - SEMAPHORE_HANDLE - IN - Semaphore handle
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS semaphoreRelease(SEMAPHORE_HANDLE);

/*
 * Locks the semaphore for any acquisitions.
 *
 * @param - SEMAPHORE_HANDLE - IN - Semaphore handle
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS semaphoreLock(SEMAPHORE_HANDLE);

/*
 * Unlocks the semaphore.
 *
 * @param - SEMAPHORE_HANDLE - IN - Semaphore handle
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS semaphoreUnlock(SEMAPHORE_HANDLE);

/*
 * Await for the semaphore to drain.
 *
 * @param - SEMAPHORE_HANDLE - IN - Semaphore handle
 * @param - UINT64 - IN - Timeout value to wait for
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS semaphoreWaitUntilClear(SEMAPHORE_HANDLE, UINT64);

#pragma pack(pop, include)

#ifdef __cplusplus
}
#endif

#endif // __DASH_UTILS_H__
