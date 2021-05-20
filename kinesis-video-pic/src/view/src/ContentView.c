/**
 * Kinesis Video Content View
 */
#define LOG_CLASS "ContentView"

#include "Include_i.h"

//////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////
/**
 * Creates a content view
 */
STATUS createContentView(UINT32 maxItemCount, UINT64 bufferDuration, ContentViewItemRemoveNotificationCallbackFunc removeCallbackFunc,
                         UINT64 customData, CONTENT_VIEW_OVERFLOW_POLICY overflowStrategy, PContentView* ppContentView)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pContentView = NULL;
    UINT32 allocationSize;

    // Check the input params
    CHK(ppContentView != NULL, STATUS_NULL_ARG);
    CHK(maxItemCount > MIN_CONTENT_VIEW_ITEMS, STATUS_MIN_CONTENT_VIEW_ITEMS);
    CHK(bufferDuration > MIN_CONTENT_VIEW_BUFFER_DURATION, STATUS_INVALID_CONTENT_VIEW_DURATION);

    // Allocate the main struct
    // NOTE: The calloc will Zero the fields
    // NOTE: The actual rolling buffer follows the structure
    allocationSize = SIZEOF(RollingContentView) + SIZEOF(ViewItem) * maxItemCount;
    pContentView = (PRollingContentView) MEMCALLOC(1, allocationSize);
    CHK(pContentView != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Set the pointer
    pContentView->itemBuffer = (PViewItem)(pContentView + 1);

    // Set the values
    pContentView->contentView.version = CONTENT_VIEW_CURRENT_VERSION;
    pContentView->allocationSize = allocationSize;
    pContentView->customData = customData;
    pContentView->removeCallbackFunc = removeCallbackFunc;
    pContentView->itemBufferCount = maxItemCount;
    pContentView->bufferDuration = bufferDuration;
    pContentView->bufferOverflowStrategy = overflowStrategy;

    // Assign the created object
    *ppContentView = (PContentView) pContentView;

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        freeContentView((PContentView) pContentView);
    }

    LEAVES();
    return retStatus;
}

/**
 * Frees the content view object
 */
STATUS freeContentView(PContentView pContentView)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    // Call is idempotent
    CHK(pContentView != NULL, retStatus);

    // Purge the content view if we have any items
    contentViewRemoveAll(pContentView);

    // Release the object
    MEMFREE(pContentView);

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Checks whether an item with the specified index exists
 */
STATUS contentViewItemExists(PContentView pContentView, UINT64 itemIndex, PBOOL pExists)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;

    // Check the input params
    CHK(pContentView != NULL && pExists != NULL, STATUS_NULL_ARG);

    *pExists = (itemIndex >= pRollingView->tail && itemIndex < pRollingView->head);

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Checks whether the specified timestamp is within the range
 */
STATUS contentViewTimestampInRange(PContentView pContentView, UINT64 timestamp, BOOL checkAckTimeStamp, PBOOL pInRange)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;
    PViewItem pNewest, pOldest;
    BOOL inRange = FALSE;

    // Check the input params
    CHK(pContentView != NULL && pInRange != NULL, STATUS_NULL_ARG);

    // Quick check if any items exist - success early return
    CHK(pRollingView->head != pRollingView->tail, STATUS_SUCCESS);

    // Get the head and tail items
    pNewest = GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->head - 1);
    pOldest = GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->tail);

    // Check the ranges
    if (checkAckTimeStamp) {
        inRange = (timestamp >= pOldest->ackTimestamp && timestamp <= pNewest->ackTimestamp + pNewest->duration);
    } else {
        inRange = (timestamp >= pOldest->timestamp && timestamp <= pNewest->timestamp + pNewest->duration);
    }

CleanUp:

    if (pInRange != NULL) {
        *pInRange = inRange;
    }

    LEAVES();
    return retStatus;
}

/**
 * Gets the current item and advances the index
 */
STATUS contentViewGetNext(PContentView pContentView, PViewItem* ppItem)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;
    PViewItem pCurrent;

    // Check the input params
    CHK(pContentView != NULL && ppItem != NULL, STATUS_NULL_ARG);

    // Quick check if any items exist - early return
    CHK((pRollingView->head != pRollingView->tail) && (pRollingView->current != pRollingView->head), STATUS_CONTENT_VIEW_NO_MORE_ITEMS);

    // Get the current item
    pCurrent = GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->current);

    // Increment the current
    pRollingView->current++;

    *ppItem = pCurrent;

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Gets an item from the given index. Current remains untouched.
 */
STATUS contentViewGetItemAt(PContentView pContentView, UINT64 itemIndex, PViewItem* ppItem)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;
    BOOL exists;

    // Check the input params
    CHK(pContentView != NULL && ppItem != NULL, STATUS_NULL_ARG);
    CHK_STATUS(contentViewItemExists(pContentView, itemIndex, &exists));
    CHK(exists, STATUS_CONTENT_VIEW_INVALID_INDEX);

    // Get the current item
    *ppItem = GET_VIEW_ITEM_FROM_INDEX(pRollingView, itemIndex);

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS contentViewGetItemWithTimestamp(PContentView pContentView, UINT64 timestamp, BOOL checkAckTimeStamp, PViewItem* ppItem)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;
    BOOL exists;

    // Check the input params
    CHK(pContentView != NULL && ppItem != NULL, STATUS_NULL_ARG);

    CHK_STATUS(contentViewTimestampInRange(pContentView, timestamp, checkAckTimeStamp, &exists));
    CHK(exists, STATUS_CONTENT_VIEW_INVALID_TIMESTAMP);

    // Find the item using a binary search method as the items are timestamp ordered.
    *ppItem = findViewItemWithTimestamp(pRollingView, GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->tail),
                                        GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->head - 1), timestamp, checkAckTimeStamp);

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Gets the current item index
 */
STATUS contentViewGetCurrentIndex(PContentView pContentView, PUINT64 pIndex)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;

    // Check the input params
    CHK(pContentView != NULL && pIndex != NULL, STATUS_NULL_ARG);

    *pIndex = pRollingView->current;

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Sets the current item index
 */
STATUS contentViewSetCurrentIndex(PContentView pContentView, UINT64 index)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;

    // Check the input params
    CHK(pContentView != NULL, STATUS_NULL_ARG);
    CHK(index >= pRollingView->tail && index <= pRollingView->head, STATUS_CONTENT_VIEW_INVALID_INDEX);

    pRollingView->current = index;

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Sets the current to a specified timestamp.
 */
STATUS contentViewRollbackCurrent(PContentView pContentView, UINT64 duration, BOOL keyFrame, BOOL lastReceivedAck)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PViewItem pCurrent = NULL;
    UINT64 timestamp;
    UINT64 curIndex, lastIndex;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;

    // Check the input params
    CHK(pContentView != NULL, STATUS_NULL_ARG);

    // Quick check if needs any rolling back
    CHK(pRollingView->current != pRollingView->tail && duration != 0, STATUS_SUCCESS);

    // Get the timestamp of the current
    curIndex = lastIndex = pRollingView->current;
    pCurrent = GET_VIEW_ITEM_FROM_INDEX(pRollingView, curIndex);
    timestamp = pCurrent->ackTimestamp;

    while (TRUE) {
        pCurrent = GET_VIEW_ITEM_FROM_INDEX(pRollingView, curIndex);

        // If we don't need a key frame or we need a key frame and the current is a fragment start
        if (!keyFrame || CHECK_ITEM_FRAGMENT_START(pCurrent->flags)) {
            // Set the current
            pRollingView->current = curIndex;

            // Check for the Ack first
            if (lastReceivedAck && CHECK_ITEM_RECEIVED_ACK(pCurrent->flags)) {
                // We have the start of the fragment and received Ack.
                // Move forward till next fragment start and set it as current
                pRollingView->current = lastIndex;
                break;
            }

            // If we have the current item with timestamp and duration
            // earlier than the specified timestamp then we terminate
            if (pCurrent->ackTimestamp + duration <= timestamp) {
                break;
            }

            // Store the last index
            lastIndex = curIndex;
        }

        // Terminate the loop if we reached the tail
        if (curIndex == pRollingView->tail) {
            break;
        }

        // Iterate backwards
        curIndex--;
    }

    // Roll forward until we have "good" frame
    curIndex = pRollingView->current;
    while (TRUE) {
        pCurrent = GET_VIEW_ITEM_FROM_INDEX(pRollingView, curIndex);

        if (!CHECK_ITEM_SKIP_ITEM(pCurrent->flags)) {
            break;
        }

        // Iterate forward
        curIndex++;

        // Set the current
        pRollingView->current = curIndex;

        // Terminate the loop if we reached the head
        if (curIndex == pRollingView->head) {
            break;
        }
    }

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Resets the current item to tail position
 */
STATUS contentViewResetCurrent(PContentView pContentView)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;

    // Check the input params
    CHK(pContentView != NULL, STATUS_NULL_ARG);

    // Start from the current and iterate
    pRollingView->current = pRollingView->tail;

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Gets the tail item
 */
STATUS contentViewGetTail(PContentView pContentView, PViewItem* ppItem)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;

    // Check the input params
    CHK(pContentView != NULL && ppItem != NULL, STATUS_NULL_ARG);

    // Quick check if any items exist - early return
    CHK(pRollingView->head != 0, STATUS_CONTENT_VIEW_NO_MORE_ITEMS);

    // Get the tail item
    *ppItem = GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->tail);

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Gets the head item
 */
STATUS contentViewGetHead(PContentView pContentView, PViewItem* ppItem)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;

    // Check the input params
    CHK(pContentView != NULL && ppItem != NULL, STATUS_NULL_ARG);

    // Quick check if any items exist - early return
    CHK(pRollingView->head != 0, STATUS_CONTENT_VIEW_NO_MORE_ITEMS);

    // Get the head item
    *ppItem = GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->head - 1);

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Gets the overall allocation size
 */
STATUS contentViewGetAllocationSize(PContentView pContentView, PUINT32 pAllocationSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;

    // Check the input params
    CHK(pContentView != NULL && pAllocationSize != NULL, STATUS_NULL_ARG);

    *pAllocationSize = pRollingView->allocationSize;

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Adds a new item to the head
 *
 * IMPORTANT: This function will evict the tail and call the remove callback (if specified)
 * based on temporal and size buffer pressure. It will not de-allocate the actual allocation.
 */
STATUS contentViewAddItem(PContentView pContentView, UINT64 timestamp, UINT64 ackTimeStamp, UINT64 duration, ALLOCATION_HANDLE allocHandle,
                          UINT32 offset, UINT32 length, UINT32 flags)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;
    PViewItem pHead = NULL;
    BOOL windowAvailability = FALSE;

    // Check the input params
    CHK(pContentView != NULL, STATUS_NULL_ARG);
    CHK(length > 0, STATUS_INVALID_CONTENT_VIEW_LENGTH);

    // If we have any items in the buffer
    if (pRollingView->head != pRollingView->tail) {
        pHead = GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->head - 1);

        // Check the continuity with the existing head item (if any exist)
        CHK(pHead->timestamp <= timestamp, STATUS_CONTENT_VIEW_INVALID_TIMESTAMP);

        // Check if we need to evict based on the max buffer depth or temporal window
        CHK_STATUS(contentViewCheckAvailability(pContentView, &windowAvailability));
        if (!windowAvailability) {
            CHK_STATUS(contentViewTrimTailItems(pContentView));
        }
    }

    // Append the new item and increment the head
    pHead = GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->head);
    pHead->timestamp = timestamp;
    pHead->ackTimestamp = ackTimeStamp;
    pHead->duration = duration;
    pHead->flags = flags;
    pHead->handle = allocHandle;
    pHead->length = length;
    pHead->index = pRollingView->head;
    SET_ITEM_DATA_OFFSET(pHead->flags, offset);

    pRollingView->head++;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS contentViewCheckAvailability(PContentView pContentView, PBOOL pWindowAvailability)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;
    BOOL windowAvailability = TRUE;
    PViewItem pTail = NULL;
    PViewItem pHead = NULL;

    CHK(pContentView != NULL && pWindowAvailability != NULL, STATUS_NULL_ARG);

    // If the tail and head are the same then there must be availability
    if (pRollingView->head != pRollingView->tail) {
        pHead = GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->head - 1);
        pTail = GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->tail);

        // Check for the item count and duration limits
        if (pRollingView->head - pRollingView->tail >= pRollingView->itemBufferCount ||
            pHead->ackTimestamp + pHead->duration - pTail->ackTimestamp >= pRollingView->bufferDuration) {
            windowAvailability = FALSE;
        }
    }

    if (pWindowAvailability != NULL) {
        *pWindowAvailability = windowAvailability;
    }

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS contentViewRemoveAll(PContentView pContentView)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;
    PViewItem pTail = NULL;
    BOOL currentRemoved;

    // Check the input params
    CHK(pContentView != NULL, STATUS_NULL_ARG);

    // Quick check if anything is needed to be done
    CHK(pRollingView->tail != pRollingView->head, retStatus);

    while (pRollingView->tail != pRollingView->head) {
        pTail = GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->tail);

        // Move the tail first
        pRollingView->tail++;

        // Move the current if needed
        if (pRollingView->current < pRollingView->tail) {
            pRollingView->current = pRollingView->tail;
            currentRemoved = TRUE;
        } else {
            currentRemoved = FALSE;
        }

        // Callback if it's specified
        if (pRollingView->removeCallbackFunc != NULL) {
            // NOTE: The call is prompt - shouldn't block
            pRollingView->removeCallbackFunc(pContentView, pRollingView->customData, pTail, currentRemoved);
        }
    }

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS contentViewTrimTail(PContentView pContentView, UINT64 itemIndex)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;
    PViewItem pTail = NULL;
    BOOL currentRemoved;

    // Check the input params
    CHK(pContentView != NULL, STATUS_NULL_ARG);
    CHK(itemIndex >= pRollingView->tail && itemIndex <= pRollingView->head, STATUS_CONTENT_VIEW_INVALID_INDEX);

    while (pRollingView->tail != itemIndex) {
        pTail = GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->tail);

        // Move the tail first
        pRollingView->tail++;

        // Move the current if needed
        if (pRollingView->current < pRollingView->tail) {
            pRollingView->current = pRollingView->tail;
            currentRemoved = TRUE;
        } else {
            currentRemoved = FALSE;
        }

        // Callback if it's specified
        if (pRollingView->removeCallbackFunc != NULL) {
            // NOTE: The call is prompt - shouldn't block
            pRollingView->removeCallbackFunc(pContentView, pRollingView->customData, pTail, currentRemoved);
        }
    }

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS contentViewTrimTailItems(PContentView pContentView)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;
    PViewItem pTail = NULL, pCurrentViewItem = NULL;
    BOOL viewItemDropped = FALSE;
    UINT64 currentStreamingItem = MAX_UINT64;
    UINT32 dropFrameCount = 0;

    // Check the input params
    CHK(pContentView != NULL, STATUS_NULL_ARG);

    pTail = GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->tail);
    // if current is greater than tail, it means the application has consumed a view item by calling contentViewGetNext
    if (pRollingView->current > pRollingView->tail) {
        currentStreamingItem = pRollingView->current - 1;
    } else {
        // otherwise no view item has ever been consumed, thus any view item dropped were never sent. currentStreamingItem
        // remains MAX_UINT64 which wont trigger any view item retaining logic.
        viewItemDropped = TRUE;
    }
    switch (pRollingView->bufferOverflowStrategy) {
        case CONTENT_VIEW_OVERFLOW_POLICY_DROP_TAIL_VIEW_ITEM:

            // if tail is the currentStreamingItem, dont drop tail, drop the view item after tail.
            if (pRollingView->tail == currentStreamingItem) {
                pRollingView->tail++;
                pTail = GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->tail);
            }

            // Callback if it's specified, also dont drop currently streaming item to avoid corrupting data.
            if (pRollingView->removeCallbackFunc != NULL) {
                // NOTE: The call is prompt - shouldn't block
                pRollingView->removeCallbackFunc(pContentView, pRollingView->customData, pTail, TRUE);
            }

            pRollingView->tail++;
            break;

        case CONTENT_VIEW_OVERFLOW_POLICY_DROP_UNTIL_FRAGMENT_START:

            do {
                // Callback if it's specified
                if (pRollingView->removeCallbackFunc != NULL) {
                    // also dont drop currently streaming item to avoid corrupting data.
                    if (pRollingView->tail != currentStreamingItem) {
                        // NOTE: The call is prompt - shouldn't block
                        pRollingView->removeCallbackFunc(pContentView, pRollingView->customData, pTail, viewItemDropped);
                        dropFrameCount++;
                    } else {
                        // we have passed the current streaming item. View items dropped beyond this point
                        // were never sent.
                        viewItemDropped = TRUE;
                    }
                }
                pRollingView->tail++;
                pTail = GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->tail);

                // Stop looping when
                // - pRollingView->tail == pRollingView->head which means there is no more view item
                // - when a new fragment start is reached AND some frames have been dropped.
            } while (pRollingView->tail != pRollingView->head && (dropFrameCount == 0 || !CHECK_ITEM_FRAGMENT_START(pTail->flags)));
            if (pRollingView->tail == pRollingView->head) {
                DLOGW("ContentView is not big enough to contain a single fragment.");
            }
            break;
    }

    // If tail rolled pass current, then reset current to tail.
    if (pRollingView->current <= pRollingView->tail) {
        pRollingView->current = pRollingView->tail;

        // If currentStreamingItem isn't sentinel and tail has rolled pass the currentStreamingItem, prepend
        // currentStreamingItem to tail and make it the new tail. This item will be freed automatically when it either
        // fall out of window or in later trim tail event (persisted ack received)
        if (currentStreamingItem != MAX_UINT64 && currentStreamingItem < pRollingView->tail) {
            pRollingView->tail--;
            pRollingView->current = pRollingView->tail + 1;
            pCurrentViewItem = GET_VIEW_ITEM_FROM_INDEX(pRollingView, currentStreamingItem);
            pTail = GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->tail);
            // Do not set pTail's timestamp and duration to pCurrentViewItem's because otherwise it would create temporal
            // gap in content view and drop frame would not cause window duration to drop.
            pTail->flags = pCurrentViewItem->flags;
            pTail->handle = pCurrentViewItem->handle;
            pTail->length = pCurrentViewItem->length;
            pTail->index = pRollingView->tail;
        }
    }

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Gets the buffer duration - the time from current to head and optional overall window duration
 */
STATUS contentViewGetWindowDuration(PContentView pContentView, PUINT64 pCurrentDuration, PUINT64 pWindowDuration)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;
    PViewItem pTail, pHead, pCurrent;
    UINT64 currentDuration = 0;
    UINT64 windowDuration = 0;

    // Check the input params
    CHK(pContentView != NULL && pCurrentDuration != NULL, STATUS_NULL_ARG);

    // Quick check if any items exist - early return
    CHK(pRollingView->head != pRollingView->tail, STATUS_SUCCESS);

    pHead = GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->head - 1);
    pTail = GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->tail);
    pCurrent = GET_VIEW_ITEM_FROM_INDEX(pRollingView, pRollingView->current);

    windowDuration = pHead->ackTimestamp + pHead->duration - pTail->ackTimestamp;

    // Check if current is at head and if so early return
    CHK(pRollingView->head != pRollingView->current, STATUS_SUCCESS);
    currentDuration = pHead->ackTimestamp + pHead->duration - pCurrent->ackTimestamp;

CleanUp:

    // Set the values
    if (pCurrentDuration != NULL) {
        *pCurrentDuration = currentDuration;
    }

    if (pWindowDuration != NULL) {
        *pWindowDuration = windowDuration;
    }

    LEAVES();
    return retStatus;
}

/**
 * Gets the view's window item count and entire window item count (optionally)
 */
STATUS contentViewGetWindowItemCount(PContentView pContentView, PUINT64 pCurrentItemCount, PUINT64 pWindowItemCount)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;
    UINT64 currentItemCount = 0;
    UINT64 windowItemCount = 0;

    // Check the input params
    CHK(pContentView != NULL && pCurrentItemCount != NULL, STATUS_NULL_ARG);

    // Quick check if any items exist - early return
    CHK(pRollingView->head != pRollingView->tail, STATUS_SUCCESS);

    windowItemCount = pRollingView->head - pRollingView->tail;
    currentItemCount = pRollingView->head - pRollingView->current;

CleanUp:

    // Set the values
    if (pCurrentItemCount != NULL) {
        *pCurrentItemCount = currentItemCount;
    }

    if (pWindowItemCount != NULL) {
        *pWindowItemCount = windowItemCount;
    }

    LEAVES();
    return retStatus;
}

/**
 * Gets the view's window allocation size and entire window allocation size (optionally)
 */
STATUS contentViewGetWindowAllocationSize(PContentView pContentView, PUINT64 pCurrentAllocationSize, PUINT64 pWindowAllocationSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PRollingContentView pRollingView = (PRollingContentView) pContentView;
    PViewItem pCurrent;
    UINT64 currentAllocationSize = 0, windowAllocationSize = 0;
    UINT64 curIndex;

    // Check the input params
    CHK(pContentView != NULL && pCurrentAllocationSize != NULL, STATUS_NULL_ARG);

    // Quick check if any items exist - early return
    CHK(pRollingView->head != pRollingView->tail, STATUS_SUCCESS);

    // Quick check if we need to calculate the current and the current is at the head and no overall window size is specified
    CHK(pRollingView->head != pRollingView->current || pWindowAllocationSize != NULL, STATUS_SUCCESS);

    curIndex = pRollingView->head - 1;
    while (TRUE) {
        pCurrent = GET_VIEW_ITEM_FROM_INDEX(pRollingView, curIndex);

        if (curIndex >= pRollingView->current && pRollingView->head != pRollingView->current) {
            // Add to the current window size
            currentAllocationSize += pCurrent->length;
        }

        // Add to the overall window size
        windowAllocationSize += pCurrent->length;

        // Check if we need to terminate after the current if the whole window is not specified
        if (curIndex == pRollingView->current && pWindowAllocationSize == NULL) {
            break;
        }

        // Terminate the loop if we reached the tail
        if (curIndex == pRollingView->tail) {
            break;
        }

        // Iterate backwards
        curIndex--;
    }

CleanUp:

    // Set the values
    if (pCurrentAllocationSize != NULL) {
        *pCurrentAllocationSize = currentAllocationSize;
    }

    if (pWindowAllocationSize != NULL) {
        *pWindowAllocationSize = windowAllocationSize;
    }

    LEAVES();
    return retStatus;
}

/**
 * Finds an element with a timestamp using binary search method.
 */
PViewItem findViewItemWithTimestamp(PRollingContentView pView, PViewItem pOldest, PViewItem pNewest, UINT64 timestamp, BOOL checkAckTimeStamp)
{
    PViewItem pCurItem = pOldest;
    UINT64 curIndex = 0, oldestIndex = pOldest->index, newestIndex = pNewest->index, curItemTimestamp;

    // We have to deal with indexes as the view can be sparse
    while (oldestIndex <= newestIndex) {
        // Calculate the current index
        curIndex = (oldestIndex + newestIndex) / 2;
        CHECK(STATUS_SUCCEEDED(contentViewGetItemAt((PContentView) pView, curIndex, &pCurItem)));

        if (checkAckTimeStamp) {
            curItemTimestamp = pCurItem->ackTimestamp;
        } else {
            curItemTimestamp = pCurItem->timestamp;
        }

        // Check if the current is the sought item
        if (curItemTimestamp <= timestamp && curItemTimestamp + pCurItem->duration >= timestamp) {
            // found the item - break from the loop
            break;
        }

        // Check if it's earlier
        if (curItemTimestamp > timestamp) {
            // Iterate with the earlier items
            newestIndex = curIndex - 1;
        } else {
            // Iterate with the later items
            oldestIndex = curIndex + 1;
        }
    }

    return pCurItem;
}
