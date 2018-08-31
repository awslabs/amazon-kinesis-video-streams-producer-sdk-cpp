/**
 * Implementation of a heap based on AIV heap
 */

#define LOG_CLASS "AIVHeap"
#include "Include_i.h"

#ifdef HEAP_DEBUG
    AIV_ALLOCATION_HEADER gAivHeader = {{0, AIV_ALLOCATION_TYPE, 0, ALLOCATION_HEADER_MAGIC}, 0, ALLOCATION_FLAGS_FREE, NULL, NULL};
    AIV_ALLOCATION_FOOTER gAivFooter = {{1, ALLOCATION_FOOTER_MAGIC}};

#define AIV_ALLOCATION_FOOTER_SIZE      SIZEOF(gAivFooter)

#else
    AIV_ALLOCATION_HEADER gAivHeader = {{0, AIV_ALLOCATION_TYPE, 0}, 0, ALLOCATION_FLAGS_FREE, NULL, NULL};
    AIV_ALLOCATION_FOOTER gAivFooter = {};

#define AIV_ALLOCATION_FOOTER_SIZE      0

#endif

#define AIV_ALLOCATION_HEADER_SIZE      SIZEOF(gAivHeader)

/**
 * Debug print analytics information
 */
DEFINE_HEAP_CHK(aivHeapDebugCheckAllocator)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PAIV_ALLOCATION_HEADER pBlock = NULL;
    PAivHeap pAivHeap = (PAivHeap) pHeap;

    // Call the base heap functionality
    CHK_STATUS(commonHeapDebugCheckAllocator(pHeap, dump));

    if (dump) {
        DLOGI("Allocated blocks pointer: \t\t\t\t%p", pAivHeap->pAlloc);
        DLOGI("*******************************************");
    }

    pBlock = pAivHeap->pAlloc;
    // walk the allocated blocks
    while(pBlock != NULL) {
        if (dump) {
            DLOGI("Block:\t%p\t\trequested size:\t%d\t\tsize:\t%d", pBlock, pBlock->allocSize, ((PALLOCATION_HEADER)pBlock)->size);
        }

        if (pBlock->allocSize > ((PALLOCATION_HEADER)pBlock)->size) {
            DLOGE("Block %p has a requested size of %d which is greater than the entire allocation size %d",
                pBlock, pBlock->allocSize, ((PALLOCATION_HEADER)pBlock)->size);
            retStatus = STATUS_HEAP_CORRUPTED;
        }

        if (pBlock->state != ALLOCATION_FLAGS_ALLOC) {
            DLOGE("Block %p is in allocated list but doesn't have it's flag set as allocated", pBlock);
            retStatus = STATUS_HEAP_CORRUPTED;
        }
#ifdef HEAP_DEBUG
        // Check the allocation 'guard band' in debug mode
        if (0 != MEMCMP(((PALLOCATION_HEADER)pBlock)->magic, ALLOCATION_HEADER_MAGIC, SIZEOF(ALLOCATION_HEADER_MAGIC))) {
            DLOGE("Invalid header for allocation %p", pBlock);
            retStatus = STATUS_HEAP_CORRUPTED;
        }

        // Check the footer
        if (0 != MEMCMP((PBYTE)(pBlock + 1) + ((PALLOCATION_HEADER)pBlock)->size, &gAivFooter, AIV_ALLOCATION_FOOTER_SIZE)) {
            DLOGE("Invalid footer for allocation %p", pBlock);
            retStatus = STATUS_HEAP_CORRUPTED;
        }
#endif
        pBlock = pBlock->pNext;
    }

    if (dump) {
        DLOGI("*******************************************");
        DLOGI("Free blocks pointer: \t\t\t\t%p", pAivHeap->pFree);
        DLOGI("*******************************************");
    }

    pBlock = pAivHeap->pFree;
    // walk the free blocks
    while(pBlock != NULL) {
        if (dump) {
            DLOGI("Block:\t%p\t\tsize:\t%d", pBlock, ((PALLOCATION_HEADER)pBlock)->size);
        }

        if (pBlock->state != ALLOCATION_FLAGS_FREE) {
            DLOGE("Block %p is in free list but doesn't have it's flag set as free", pBlock);
            retStatus = STATUS_HEAP_CORRUPTED;
        }
#ifdef HEAP_DEBUG
        // Check the allocation 'guard band' in debug mode
        if (0 != MEMCMP(((PALLOCATION_HEADER)pBlock)->magic, ALLOCATION_HEADER_MAGIC, SIZEOF(ALLOCATION_HEADER_MAGIC))) {
            DLOGE("Invalid header for allocation %p", pBlock);
            retStatus = STATUS_HEAP_CORRUPTED;
        }
#endif
        pBlock = pBlock->pNext;
    }

    if (dump) {
        DLOGI("*******************************************");
    }

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Creates the heap
 */
DEFINE_CREATE_HEAP(aivHeapCreate)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PBaseHeap pBaseHeap = NULL;

    CHK_STATUS(commonHeapCreate(ppHeap, SIZEOF(AivHeap)));

    // Set the function pointers
    pBaseHeap = (PBaseHeap) *ppHeap;
    pBaseHeap->heapInitializeFn = aivHeapInit;
    pBaseHeap->heapReleaseFn = aivHeapRelease;
    pBaseHeap->heapGetSizeFn = commonHeapGetSize; // Use the common heap functionality
    pBaseHeap->heapAllocFn = aivHeapAlloc;
    pBaseHeap->heapFreeFn = aivHeapFree;
    pBaseHeap->heapGetAllocSizeFn = aivHeapGetAllocSize;
    pBaseHeap->heapMapFn = aivHeapMap;
    pBaseHeap->heapUnmapFn = aivHeapUnmap;
    pBaseHeap->heapDebugCheckAllocatorFn = aivHeapDebugCheckAllocator;
    pBaseHeap->getAllocationSizeFn = aivGetAllocationSize;
    pBaseHeap->getAllocationHeaderSizeFn = aivGetAllocationHeaderSize;
    pBaseHeap->getAllocationFooterSizeFn = aivGetAllocationFooterSize;
    pBaseHeap->getHeapLimitsFn = aivGetHeapLimits;

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Initialize the heap
 */
DEFINE_INIT_HEAP(aivHeapInit)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PAivHeap pAivHeap = (PAivHeap) pHeap;

    CHK(pAivHeap != NULL, STATUS_NULL_ARG);

    // Set the initial values in case the base init fails
    pAivHeap->pAllocation = NULL;
    pAivHeap->pFree = NULL;
    pAivHeap->pAlloc = NULL;

    // Call the base functionality
    CHK_STATUS(commonHeapInit(pHeap, heapLimit));

    // Allocate the entire heap backed by the process default heap.
    pAivHeap->pAllocation = MEMALLOC((SIZE_T)heapLimit);
    CHK_ERR(pAivHeap->pAllocation != NULL,
        STATUS_NOT_ENOUGH_MEMORY,
        "Failed to allocate heap with limit size %" PRIu64,
        heapLimit);

#ifdef HEAP_DEBUG
    // Null the memory in debug mode
    MEMSET(pAivHeap->pAllocation, 0x00, heapLimit);
#endif

    // Set the free pointer
    pAivHeap->pFree = (PAIV_ALLOCATION_HEADER) pAivHeap->pAllocation;

    // Set the free link
    MEMCPY(pAivHeap->pFree, &gAivHeader, AIV_ALLOCATION_HEADER_SIZE);

    // We don't need to re-adjust the heap size to accommodate the header allocation as it will be taken care during alloc
    ((PALLOCATION_HEADER)pAivHeap->pFree)->size = (UINT32)(pHeap->heapLimit - AIV_ALLOCATION_HEADER_SIZE);

CleanUp:

    // Clean-up on error
    if (STATUS_FAILED(retStatus)) {
        if (pAivHeap->pAllocation != NULL) {
            MEMFREE(pAivHeap->pAllocation);
            pAivHeap->pAllocation = NULL;
        }

        // Re-set everything
        pHeap->heapLimit = 0;
    }

    LEAVES();
    return retStatus;
}

/**
 * Free the AIV heap
 */
DEFINE_RELEASE_HEAP(aivHeapRelease)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PAivHeap pAivHeap = (PAivHeap) pHeap;

    // The call should be idempotent
    CHK (pHeap != NULL, STATUS_SUCCESS);

    // Regardless of the status (heap might be corrupted) we still want to free the memory
    retStatus = commonHeapRelease(pHeap);

    // Release the entire heap regardless of the status that's returned earlier
    if (pAivHeap->pAllocation != NULL) {
        MEMFREE(pAivHeap->pAllocation);
    }

    // Free the object itself
    MEMFREE(pHeap);

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Allocate from the heap
 */
DEFINE_HEAP_ALLOC(aivHeapAlloc)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PAIV_ALLOCATION_HEADER pFree = NULL;
    PAivHeap pAivHeap = (PAivHeap) pHeap;

    // Call the common heap function
    retStatus = commonHeapAlloc(pHeap, size, pHandle);
    CHK(retStatus == STATUS_NOT_ENOUGH_MEMORY || retStatus == STATUS_SUCCESS, retStatus);
    if (retStatus == STATUS_NOT_ENOUGH_MEMORY) {
        // If we are out of memory then we don't need to return a failure - just
        // Early return with success
        CHK(FALSE, STATUS_SUCCESS);
    }

    pFree = getFreeBlock(pAivHeap, size);

    // We might hit this due to de-fragmentation of the heap
    // IMPORTANT! We will return success without setting the handle
    CHK(pFree != NULL, STATUS_SUCCESS);

    // Split the free block
    splitFreeBlock(pAivHeap, pFree, size);

    // Add the block to the allocated list
    addAllocatedBlock(pAivHeap, pFree);

    // Set the return value of the handle to be the allocation address as this is a
    // direct allocation and doesn't support mapping.
    // IMPORTANT!!! We are not returning the actual address but rather the offset
    // from the heap base in the high-order 32 bits. This is done so we can
    // later differentiate between direct memory allocations and VRAM allocation
    *pHandle = TO_AIV_HANDLE(pAivHeap, pFree + 1);

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Free the allocation
 */
DEFINE_HEAP_FREE(aivHeapFree)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PAIV_ALLOCATION_HEADER pAlloc = NULL;
    PVOID pAllocation;
    PAivHeap pAivHeap = (PAivHeap) pHeap;

    CHK(pAivHeap != NULL, STATUS_NULL_ARG);

    // This is a direct memory allocation so the memory pointer is stored as a handle
    // IMPORTANT.. The handle is the offset so we need to convert to the pointer
    pAllocation = FROM_AIV_HANDLE(pAivHeap, handle);
    CHK_ERR(pAllocation != NULL, STATUS_INVALID_HANDLE_ERROR, "Invalid handle passed to free");

    // Perform the de-allocation
    pAlloc = (PAIV_ALLOCATION_HEADER)pAllocation - 1;

    CHK_ERR(pAlloc->state == ALLOCATION_FLAGS_ALLOC && pAlloc->allocSize != 0, STATUS_INVALID_HANDLE_ERROR,
            "Invalid block of memory passed to free.");

    // Call the common heap function
    CHK_STATUS(commonHeapFree(pHeap, handle));

    // Remove from the allocated
    removeAllocatedBlock(pAivHeap, pAlloc);

    // Add to the free blocks
    addFreeBlock(pAivHeap, pAlloc);

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Gets the allocation size
 */
DEFINE_HEAP_GET_ALLOC_SIZE(aivHeapGetAllocSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PAIV_ALLOCATION_HEADER pHeader;
    PVOID pAllocation;
    PAivHeap pAivHeap = (PAivHeap) pHeap;

    CHK(pAivHeap != NULL, STATUS_NULL_ARG);

    // This heap implementation uses a direct memory allocation so no
    // mapping really needed - just conversion from a handle to memory pointer.
    // IMPORTANT.. The handle is the offset so we need to convert to the pointer
    pAllocation = FROM_AIV_HANDLE(pAivHeap, handle);

    // Call the common heap function
    CHK_STATUS(commonHeapGetAllocSize(pHeap, handle, pAllocSize));

    pHeader = (PAIV_ALLOCATION_HEADER)pAllocation - 1;

    // Check for the validity of the allocation
    CHK_ERR(pHeader->state == ALLOCATION_FLAGS_ALLOC && pHeader->allocSize != 0, STATUS_INVALID_HANDLE_ERROR,
            "Invalid handle or previously freed.");

    *pAllocSize = pHeader->allocSize;

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Map the allocation handle
 */
DEFINE_HEAP_MAP(aivHeapMap)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PAIV_ALLOCATION_HEADER pHeader;
    PVOID pAllocation;
    PAivHeap pAivHeap = (PAivHeap) pHeap;

    CHK(pAivHeap != NULL, STATUS_NULL_ARG);

    // This heap implementation uses a direct memory allocation so no
    // mapping really needed - just conversion from a handle to memory pointer.
    // IMPORTANT.. The handle is the offset so we need to convert to the pointer
    pAllocation = FROM_AIV_HANDLE(pAivHeap, handle);

    // Call the common heap function
    CHK_STATUS(commonHeapMap(pHeap, handle, ppAllocation, pSize));

    *ppAllocation = pAllocation;
    pHeader = (PAIV_ALLOCATION_HEADER)pAllocation - 1;

    // Check for the validity of the allocation
    CHK_ERR(pHeader->state == ALLOCATION_FLAGS_ALLOC && pHeader->allocSize != 0, STATUS_INVALID_HANDLE_ERROR,
            "Invalid handle or previously freed.");

    // Set the size from the previously stored requested allocation size as it might be
    // smaller than the actual allocation size.
    *pSize = pHeader->allocSize;

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Un-Maps the allocation handle. In this implementation it doesn't do anything
 */
DEFINE_HEAP_UNMAP(aivHeapUnmap)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    // Delegate the call directly
    CHK_STATUS(commonHeapUnmap(pHeap, pAllocation));

CleanUp:
    LEAVES();
    return retStatus;
}

DEFINE_HEADER_SIZE(aivGetAllocationHeaderSize)
{
    return AIV_ALLOCATION_HEADER_SIZE;
}

DEFINE_FOOTER_SIZE(aivGetAllocationFooterSize)
{
    return AIV_ALLOCATION_FOOTER_SIZE;
}

DEFINE_ALLOC_SIZE(aivGetAllocationSize)
{
    // This is a direct allocation
    PVOID pAllocation = FROM_AIV_HANDLE((PAivHeap) pHeap, handle);
    PAIV_ALLOCATION_HEADER pHeader = (PAIV_ALLOCATION_HEADER)pAllocation - 1;

#ifdef HEAP_DEBUG
    // Check the allocation 'guard band' in debug mode
    if (0 != MEMCMP(((PALLOCATION_HEADER)pHeader)->magic, ALLOCATION_HEADER_MAGIC, SIZEOF(ALLOCATION_HEADER_MAGIC))) {
        DLOGE("Invalid header for allocation %p", pAllocation);
        return INVALID_ALLOCATION_VALUE;
    }

    // Check the footer
    if (0 != MEMCMP((PBYTE)(pHeader + 1) + ((PALLOCATION_HEADER)pHeader)->size, &gAivFooter, AIV_ALLOCATION_FOOTER_SIZE)) {
        DLOGE("Invalid footer for allocation %p", pAllocation);
        return INVALID_ALLOCATION_VALUE;
    }

    // Check the type
    if (AIV_ALLOCATION_TYPE != ((PALLOCATION_HEADER)pHeader)->type) {
        DLOGE("Invalid allocation type 0x%08x", ((PALLOCATION_HEADER)pHeader)->type);
        return INVALID_ALLOCATION_VALUE;
    }

    // Check the allocation size against the overall allocation for the block
    if (pHeader->allocSize > ((PALLOCATION_HEADER)pHeader)->size) {
        DLOGE("Block %p has a requested size of %u which is greater than the allocation size %u", pHeader, pHeader->allocSize, ((PALLOCATION_HEADER)pHeader)->size);
        return INVALID_ALLOCATION_VALUE;
    }
#endif

    return AIV_ALLOCATION_HEADER_SIZE + ((PALLOCATION_HEADER)pHeader)->size + AIV_ALLOCATION_FOOTER_SIZE;
}

DEFINE_HEAP_LIMITS(aivGetHeapLimits)
{
    *pMinHeapSize = MIN_HEAP_SIZE;
    *pMaxHeapSize = MAX_HEAP_SIZE;
}

/**
 * Simple first free block finding
 */
PAIV_ALLOCATION_HEADER getFreeBlock(PAivHeap pAivHeap, UINT32 size)
{
    CHECK(pAivHeap != NULL && size > 0);
    
    PAIV_ALLOCATION_HEADER pFree = pAivHeap->pFree;
    // overall allocation size - the header is already included in the free list but not the footer
    UINT32 allocationSize = size + AIV_ALLOCATION_FOOTER_SIZE;

    // Perform the allocation by looking for the first fit in the free list
    while (pFree != NULL) {
        // check for the fit
        if (((PALLOCATION_HEADER)pFree)->size >= allocationSize) {
            // return the found block which will be further processed later
            return pFree;
        }

        pFree = pFree->pNext;
    }

    return NULL;
}

/**
 * Splits the free block
 */
VOID splitFreeBlock(PAivHeap pAivHeap, PAIV_ALLOCATION_HEADER pBlock, UINT32 size)
{
    CHECK(pAivHeap != NULL && pBlock != NULL && size > 0);

    PAIV_ALLOCATION_HEADER pNewFree = NULL;

    // There are two scenarios - whether the new header + minimal block size will fit or not
    // In case we end up with smaller block then we will just attach that to the allocated
    if (((PALLOCATION_HEADER)pBlock)->size < size + AIV_ALLOCATION_FOOTER_SIZE + AIV_ALLOCATION_HEADER_SIZE +
            MIN_FREE_ALLOCATION_SIZE + AIV_ALLOCATION_FOOTER_SIZE) {
        // use the entire block
        // Fix the next block
        if (pBlock->pNext != NULL) {
            pBlock->pNext->pPrev = pBlock->pPrev;
        }

        // Fix the prev block
        if (pBlock->pPrev != NULL) {
            pBlock->pPrev->pNext = pBlock->pNext;
        } else {
            // this is the case where we need to Fix-up the pAivHeap->pFree
            CHECK_EXT(pAivHeap->pFree == pBlock, "Free block pointer is invalid");
            pAivHeap->pFree = pBlock->pNext;
        }

        // set the footer
        MEMCPY((PBYTE)(pBlock + 1) + ((PALLOCATION_HEADER)pBlock)->size - AIV_ALLOCATION_FOOTER_SIZE,
               &gAivFooter, AIV_ALLOCATION_FOOTER_SIZE);

#ifdef HEAP_DEBUG
        // Null the memory in debug mode
        MEMSET(pBlock + 1, 0x00, ((PALLOCATION_HEADER)pBlock)->size - AIV_ALLOCATION_FOOTER_SIZE);

        // Set the header magic
        MEMCPY(((PALLOCATION_HEADER)pBlock)->magic, ALLOCATION_HEADER_MAGIC, SIZEOF(ALLOCATION_HEADER_MAGIC));
#endif

        // adjust the size to accommodate the footer
        ((PALLOCATION_HEADER)pBlock)->size -= AIV_ALLOCATION_FOOTER_SIZE;

        // we also need to Fix-up the heap size due to larger allocation
        ((PHeap)pAivHeap)->heapSize += ((PALLOCATION_HEADER)pBlock)->size - size;
    } else {
        // split the block
        // Prepare the new free block
        UINT32 freeSizeStart = size + AIV_ALLOCATION_FOOTER_SIZE;
        pNewFree = (PAIV_ALLOCATION_HEADER)((PBYTE)(pBlock + 1) + freeSizeStart);

        // Set the header
        MEMCPY(pNewFree, &gAivHeader, AIV_ALLOCATION_HEADER_SIZE);

        // Set the size and link to the chain
        ((PALLOCATION_HEADER)pNewFree)->size = ((PALLOCATION_HEADER)pBlock)->size - freeSizeStart - AIV_ALLOCATION_HEADER_SIZE;
        pNewFree->pNext = pBlock->pNext;
        pNewFree->pPrev = pBlock->pPrev;

#ifdef HEAP_DEBUG
        // Null the memory in debug mode
        MEMSET(pNewFree + 1, 0x00, ((PALLOCATION_HEADER)pNewFree)->size - AIV_ALLOCATION_FOOTER_SIZE);
#endif

        // Fix the next block
        if (pNewFree->pNext != NULL) {
            pNewFree->pNext->pPrev = pNewFree;
        }

        // Fix the prev block
        if (pNewFree->pPrev != NULL) {
            pNewFree->pPrev->pNext = pNewFree;
        } else {
            // This is the case where we need to Fix-up the pAivHeap->pFree
            CHECK_EXT(pAivHeap->pFree == pBlock, "Free block pointer is invalid");
            pAivHeap->pFree = pNewFree;
        }

        // Set the type of the block
        pNewFree->state = ALLOCATION_FLAGS_FREE;

        // adjust the free block size
        ((PALLOCATION_HEADER)pBlock)->size = size;

        // set the footer - don't need to set the header magic as it's been set in the free list
        MEMCPY((PBYTE)(pBlock + 1) + ((PALLOCATION_HEADER)pBlock)->size, &gAivFooter, AIV_ALLOCATION_FOOTER_SIZE);
    }

    // Nullify the pointers
    pBlock->pNext = pBlock->pPrev = NULL;

    // Set as undefined
    pBlock->state = ALLOCATION_FLAGS_NONE;

    // Set the actual allocation size which might be smaller than the overall block size
    pBlock->allocSize = size;
}

/**
 * Adds the newly allocated block to the head of the allocations.
 */
VOID addAllocatedBlock(PAivHeap pAivHeap, PAIV_ALLOCATION_HEADER pBlock)
{
    CHECK(pAivHeap != NULL &&
                  pBlock != NULL && ((PALLOCATION_HEADER)pBlock)->size > 0 &&
                  pBlock->pNext == NULL && pBlock->pPrev == NULL &&
                  pBlock->state == ALLOCATION_FLAGS_NONE);

    // Set as allocated
    pBlock->state = ALLOCATION_FLAGS_ALLOC;

    if (pAivHeap->pAlloc != NULL) {
        // This is the case when we do have the allocated chain
        pBlock->pNext = pAivHeap->pAlloc;
        pAivHeap->pAlloc->pPrev = pBlock;
    }

    // Set the new head
    pAivHeap->pAlloc = pBlock;
}

/**
 * Removes an allocated block from the allocated list and fixes up the heap
 */
VOID removeAllocatedBlock(PAivHeap pAivHeap, PAIV_ALLOCATION_HEADER pBlock)
{
    CHECK(pAivHeap != NULL && pBlock != NULL &&
                  ((PALLOCATION_HEADER)pBlock)->size > 0);

    // Fix-up the prev
    if (pBlock->pPrev == NULL) {
        // This is the case of the first block
        CHECK_EXT(pAivHeap->pAlloc = pBlock, "Alloc block pointer is invalid");
        pAivHeap->pAlloc = pBlock->pNext;
    } else {
        pBlock->pPrev->pNext = pBlock->pNext;
    }

    // Fix-up the next
    if (pBlock->pNext != NULL) {
        pBlock->pNext->pPrev = pBlock->pPrev;
    }

    // Fix-up the free block to add the footer to the free space
    ((PALLOCATION_HEADER)pBlock)->size += AIV_ALLOCATION_FOOTER_SIZE;

    // Set the status as undefined
    pBlock->state = ALLOCATION_FLAGS_NONE;

    // Nullify the links
    pBlock->pNext = pBlock->pPrev = NULL;

    // Set the requested allocation size to 0
    pBlock->allocSize = 0;
}

/**
 * Adds a free block to the free list
 * IMPORTANT!!! The list should be ordered in order to have
 * a one pass on it and have the blocks coalesce.
 */
VOID addFreeBlock(PAivHeap pAivHeap, PAIV_ALLOCATION_HEADER pBlock)
{
    CHECK(pAivHeap != NULL &&
                  pBlock != NULL && ((PALLOCATION_HEADER)pBlock)->size > 0 &&
                  pBlock->pNext == NULL && pBlock->pPrev == NULL &&
                  pBlock->state == ALLOCATION_FLAGS_NONE);

    // Set as free
    pBlock->state = ALLOCATION_FLAGS_FREE;

    // Find the spot and add the block first
    // and then try to coalesce the neighboring blocks

    // Special case with early return if it's the first block
    if (pAivHeap->pFree == NULL) {
        pAivHeap->pFree = pBlock;
        return;
    }

    // Iterate to find the spot to add
    PAIV_ALLOCATION_HEADER pNext = pAivHeap->pFree;
    PAIV_ALLOCATION_HEADER pLast = pAivHeap->pFree;
    while (pNext != NULL) {
        // Store the last
        pLast = pNext;

#ifdef HEAP_DEBUG
        // the ranges should never overlap
        CHECK_EXT(!checkOverlap(pNext, pBlock), "Invalid range or the free list is corrupt!");
#endif

        if (pNext > pBlock) {
            insertFreeBlockBefore(pAivHeap, pNext, pBlock);

            // The block has been coalesced/processed - early return
            return;
        }

        // Iterate over
        pNext = pNext->pNext;
    }

    // we iterated till the end but didn't insert - insert the last
    insertFreeBlockLast(pLast, pBlock);
}

/**
 * Inserts free block before this free block
 */
VOID insertFreeBlockBefore(PAivHeap pAivHeap, PAIV_ALLOCATION_HEADER pFree, PAIV_ALLOCATION_HEADER pBlock)
{
    CHECK(pAivHeap != NULL && pFree != NULL);

    // There are 4 cases we need to consider
    // 1) The block is adjacent on left with a free block - coalesce
    // 2) The block is adjacent on right with a free block - coalesce
    // 3) The block is adjacent on both sides
    // 4) An independent block - needs to be added to the free list

    // We need to insert the block first
    DLOGS("Insert free block %p (size %d) before %p", pBlock, ((PALLOCATION_HEADER)pBlock)->size, pFree);
    pBlock->pPrev = pFree->pPrev;
    pBlock->pNext = pFree;
    pFree->pPrev = pBlock;
    if (pBlock->pPrev != NULL) {
        pBlock->pPrev->pNext = pBlock;
    } else {
        // This is the case of mFree - fix it up
        CHECK_EXT(pAivHeap->pFree == pFree, "Free pointer is invalid");
        pAivHeap->pFree = pBlock;
    }

    // Try to coalesce the block with the neighbors
    DLOGS("Trying to coalesce block %p (size %d) with neighbors", pBlock, ((PALLOCATION_HEADER)pBlock)->size);
    coalesceFreeBlock(pBlock);
}

/**
 * Inserts free block after the last block
 */
VOID insertFreeBlockLast(PAIV_ALLOCATION_HEADER pFree, PAIV_ALLOCATION_HEADER pBlock)
{
    CHECK(pFree != NULL);

    // There are 2 cases we need to consider
    // 1) The block is adjacent on left with a free block - coalesce
    // 2) An independent block - needs to be added to the free list

    DLOGS("Insert free block %p (size %d) after %p", pBlock, ((PALLOCATION_HEADER)pBlock)->size, pFree);
    pFree->pNext = pBlock;
    pBlock->pPrev = pFree;

    // Try to coalesce the block with the neighbors
    DLOGS("Trying to coalesce block %p (size %d) with neighbors", pBlock, ((PALLOCATION_HEADER)pBlock)->size);
    coalesceFreeBlock(pBlock);
}

/**
 * Tries to coalesce the block to the neighbors
 */
VOID coalesceFreeBlock(PAIV_ALLOCATION_HEADER pBlock)
{
    CHECK(pBlock != NULL && ((PALLOCATION_HEADER)pBlock)->size > 0);

    // Try the next first
    PAIV_ALLOCATION_HEADER pNext = pBlock->pNext;
    if (pNext != NULL &&
            pNext == (PAIV_ALLOCATION_HEADER)((PBYTE)(pBlock + 1) + ((PALLOCATION_HEADER)pBlock)->size)) {
        // Coalesce the blocks by adjusting the size and fixing up the pointers
        DLOGS("Coalescing %p (size %d) with %p (size %d) resulting in size %d",
                pBlock, ((PALLOCATION_HEADER)pBlock)->size, pNext, ((PALLOCATION_HEADER)pNext)->size,
                ((PALLOCATION_HEADER)pBlock)->size + ((PALLOCATION_HEADER)pBlock)->size + AIV_ALLOCATION_HEADER_SIZE);

        // Use pBlock as the new allocation
        ((PALLOCATION_HEADER)pBlock)->size += ((PALLOCATION_HEADER)pNext)->size + AIV_ALLOCATION_HEADER_SIZE;

        // Remove pNext and fix-up the links
        pBlock->pNext = pNext->pNext;
        if (pNext->pNext != NULL) {
            pNext->pNext->pPrev = pBlock;
        }
    }

    // Try the previous block
    PAIV_ALLOCATION_HEADER pPrev = pBlock->pPrev;
    if (pPrev != NULL &&
            pBlock == (PAIV_ALLOCATION_HEADER)((PBYTE)(pPrev + 1) + ((PALLOCATION_HEADER)pPrev)->size)) {
        DLOGS("Coalescing %p (size %u) with %p (size %u) resulting in size %u",
              pPrev, ((PALLOCATION_HEADER)pPrev)->size, pBlock,
              ((PALLOCATION_HEADER)pBlock)->size,
              ((PALLOCATION_HEADER)pBlock)->size + ((PALLOCATION_HEADER)pPrev)->size + AIV_ALLOCATION_HEADER_SIZE);

        // Use pPrev as the new allocation
        ((PALLOCATION_HEADER)pPrev)->size += ((PALLOCATION_HEADER)pBlock)->size + AIV_ALLOCATION_HEADER_SIZE;

        // Remove pBlock and fix-up the links
        pPrev->pNext = pBlock->pNext;
        if (pBlock->pNext != NULL) {
            pBlock->pNext->pPrev = pPrev;
        }
    }
}

/**
 * Checks if the free blocks overlap
 */
BOOL checkOverlap(PAIV_ALLOCATION_HEADER pBlock1, PAIV_ALLOCATION_HEADER pBlock2)
{
    if (pBlock1 < pBlock2) {
        return (PBYTE)(pBlock1 + 1) + ((PALLOCATION_HEADER)pBlock1)->size > (PBYTE)pBlock2;
    } else {
        return (PBYTE)(pBlock2 + 1) + ((PALLOCATION_HEADER)pBlock2)->size > (PBYTE)pBlock1;
    }
}
