/*******************************************
Main internal include file
*******************************************/
#ifndef __CONTENT_VIEW_INCLUDE_I__
#define __CONTENT_VIEW_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////
// Project include files
////////////////////////////////////////////////////
#include "com/amazonaws/kinesis/video/view/Include.h"

////////////////////////////////////////////////////
// General defines and data structures
////////////////////////////////////////////////////

/**
 * Gets the item pointer from the item buffer
 */
#define GET_VIEW_ITEM_FROM_INDEX(view, index) (&(view)->itemBuffer[((index) == 0) ? 0 : ((index) % (view)->itemBufferCount)])

/**
 * ContentView internal structure
 *
 * IMPORTANT! This structure is tightly packed without the compiler directives.
 */
typedef struct {
    // The original public structure
    // IMPORTANT! The contentView is 32 bit and together with the allocationSize field following
    // it will be 64 bit packed together with the rest of the structure
    ContentView contentView;

    // Overall allocation size
    UINT32 allocationSize;

    // The location of the head and the tail
    UINT64 head;
    UINT64 tail;

    // The current location
    UINT64 current;

    // The custom data for the callback
    UINT64 customData;

    // The function pointer for a callback - optional
    ContentViewItemRemoveNotificationCallbackFunc removeCallbackFunc;

    // Buffer duration to keep
    UINT64 bufferDuration;

    // Controls how content view drop frames when it's overflown
    CONTENT_VIEW_OVERFLOW_POLICY bufferOverflowStrategy;

    // Ensure the struct is 64 bit packed. If a UINT32 field need to be added in the future this variable can be used.
    UINT32 reserved;

    // The size of the buffer
    UINT64 itemBufferCount;

    // The actual buffer which follows immediately after the structure
    PViewItem itemBuffer;
} RollingContentView, *PRollingContentView;

////////////////////////////////////////////////////
// Internal functionality
////////////////////////////////////////////////////
PViewItem findViewItemWithTimestamp(PRollingContentView, PViewItem, PViewItem, UINT64, BOOL);

#ifdef __cplusplus
}
#endif
#endif /* __CONTENT_VIEW_INCLUDE_I__ */
