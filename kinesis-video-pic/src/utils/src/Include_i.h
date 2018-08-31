/**
 * Internal functionality
 */
#ifndef __UTILS_I_H__
#define __UTILS_I_H__

#ifdef __cplusplus
extern "C" {
#endif

#pragma once

#include <com/amazonaws/kinesis/video/common/CommonDefs.h>

#include "com/amazonaws/kinesis/video/utils/Include.h"

/**
 * Thread wrapper for Windows
 */
typedef struct {
    // Stored routine
    startRoutine storedStartRoutine;

    // Original arguments
    PVOID storedArgs;
} WindowsThreadRoutineWrapper, *PWindowsThreadRoutineWrapper;

/**
 * Internal String operations
 */
STATUS strtoint(PCHAR, PCHAR, UINT32, PUINT64, PBOOL);

/**
 * Internal safe multiplication
 */
STATUS unsignedSafeMultiplyAdd(UINT64, UINT64, UINT64, PUINT64);

/**
 * Internal Double Linked List operations
 */
STATUS doubleListAllocNode(UINT64, PDoubleListNode*);
STATUS doubleListInsertNodeHeadInternal(PDoubleList, PDoubleListNode);
STATUS doubleListInsertNodeTailInternal(PDoubleList, PDoubleListNode);
STATUS doubleListInsertNodeBeforeInternal(PDoubleList, PDoubleListNode, PDoubleListNode);
STATUS doubleListInsertNodeAfterInternal(PDoubleList, PDoubleListNode, PDoubleListNode);
STATUS doubleListRemoveNodeInternal(PDoubleList, PDoubleListNode);
STATUS doubleListGetNodeAtInternal(PDoubleList, UINT32, PDoubleListNode*);

/**
 * Internal Single Linked List operations
 */
STATUS singleListAllocNode(UINT64, PSingleListNode*);
STATUS singleListInsertNodeHeadInternal(PSingleList, PSingleListNode);
STATUS singleListInsertNodeTailInternal(PSingleList, PSingleListNode);
STATUS singleListInsertNodeAfterInternal(PSingleList, PSingleListNode, PSingleListNode);
STATUS singleListGetNodeAtInternal(PSingleList, UINT32, PSingleListNode*);

/**
 * Internal Hash Table operations
 */
#define DEFAULT_HASH_TABLE_BUCKET_LENGTH        2
#define DEFAULT_HASH_TABLE_BUCKET_COUNT         10000
#define MIN_HASH_TABLE_ENTRIES_ALLOC_LENGTH     8

/**
 * Bucket declaration
 * NOTE: Variable size structure - the buckets can follow directly after the main structure
 * or in case of allocated array it's a separate allocation
 */
typedef struct {
    UINT32 count;
    UINT32 length;
    PHashEntry entries;
} HashBucket, *PHashBucket;

UINT64 getKeyHash(UINT64);
PHashBucket getHashBucket(PHashTable, UINT64);

/**
 * Internal Directory functionality
 */
STATUS removeFileDir(UINT64, DIR_ENTRY_TYPES, PCHAR, PCHAR);
STATUS getFileDirSize(UINT64, DIR_ENTRY_TYPES, PCHAR, PCHAR);

/**
 * Endianness functionality
 */
INLINE INT16 getInt16Swap(INT16);
INLINE INT16 getInt16NoSwap(INT16);
INLINE INT32 getInt32Swap(INT32);
INLINE INT32 getInt32NoSwap(INT32);
INLINE INT64 getInt64Swap(INT64);
INLINE INT64 getInt64NoSwap(INT64);

INLINE VOID putInt16Swap(PINT16, INT16);
INLINE VOID putInt16NoSwap(PINT16, INT16);
INLINE VOID putInt32Swap(PINT32, INT32);
INLINE VOID putInt32NoSwap(PINT32, INT32);
INLINE VOID putInt64Swap(PINT64, INT64);
INLINE VOID putInt64NoSwap(PINT64, INT64);

#ifdef __cplusplus
}
#endif

#endif // __UTILS_I_H__
