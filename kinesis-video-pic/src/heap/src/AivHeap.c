/**
 * Implementation of a heap based on AIV heap
 */

#define LOG_CLASS "AIVHeap"
#include "Include_i.h"

#ifdef HEAP_DEBUG
struct AIV_ALLOCATION_HEADER gAivHeader = {{0, AIV_ALLOCATION_TYPE, ALLOCATION_FLAGS_FREE, ALLOCATION_HEADER_MAGIC}, 0, NULL, NULL};
ALLOCATION_FOOTER gAivFooter = {0, ALLOCATION_FOOTER_MAGIC};
#else
struct AIV_ALLOCATION_HEADER gAivHeader = {{0, AIV_ALLOCATION_TYPE, ALLOCATION_FLAGS_FREE}, 0, NULL, NULL};
ALLOCATION_FOOTER gAivFooter = {0};
#endif

#define AIV_ALLOCATION_HEADER_SIZE SIZEOF(gAivHeader)
#define AIV_ALLOCATION_FOOTER_SIZE SIZEOF(gAivFooter)

/**
 * Minimal free allocation size - if we end up with a free block of lesser size we will coalesce this to the allocated block
 */
#define MIN_FREE_BLOCK_SIZE (AIV_ALLOCATION_HEADER_SIZE + MIN_FREE_ALLOCATION_SIZE + AIV_ALLOCATION_FOOTER_SIZE)

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
        DLOGV("Allocated blocks pointer: \t\t\t\t%p", pAivHeap->pAlloc);
        DLOGV("*******************************************");
    }

    pBlock = pAivHeap->pAlloc;
    // walk the allocated blocks
    while (pBlock != NULL) {
        if (dump) {
            DLOGV("Block:\t%p\t\trequested size:\t%d\t\tsize:\t%d", pBlock, pBlock->allocSize, GET_AIV_ALLOCATION_SIZE(pBlock));
        }

        if (pBlock->allocSize > GET_AIV_ALLOCATION_SIZE(pBlock)) {
            DLOGE("Block %p has a requested size of %" PRIu64 " which is greater than the entire allocation size %" PRIu64, pBlock, pBlock->allocSize,
                  GET_AIV_ALLOCATION_SIZE(pBlock));
            retStatus = STATUS_HEAP_CORRUPTED;
        }

        if (pBlock->header.flags != ALLOCATION_FLAGS_ALLOC) {
            DLOGE("Block %p is in allocated list but doesn't have it's flag set as allocated", pBlock);
            retStatus = STATUS_HEAP_CORRUPTED;
        }

        if (GET_AIV_ALLOCATION_SIZE(pBlock) != GET_AIV_ALLOCATION_FOOTER_SIZE(pBlock)) {
            DLOGE("Block %p header and footer allocation sizes mismatch", pBlock);
            retStatus = STATUS_HEAP_CORRUPTED;
        }

#ifdef HEAP_DEBUG
        // Check the allocation 'guard band' in debug mode
        if (0 != MEMCMP(((PALLOCATION_HEADER) pBlock)->magic, ALLOCATION_HEADER_MAGIC, SIZEOF(ALLOCATION_HEADER_MAGIC))) {
            DLOGE("Invalid header for allocation %p", pBlock);
            retStatus = STATUS_HEAP_CORRUPTED;
        }

        // Check the footer
        if (0 != MEMCMP(GET_AIV_ALLOCATION_FOOTER(pBlock)->magic, ALLOCATION_FOOTER_MAGIC, SIZEOF(ALLOCATION_FOOTER_MAGIC))) {
            DLOGE("Invalid footer for allocation %p", pBlock);
            retStatus = STATUS_HEAP_CORRUPTED;
        }
#endif
        pBlock = pBlock->pNext;
    }

    if (dump) {
        DLOGV("*******************************************");
        DLOGV("Free blocks pointer: \t\t\t\t%p", pAivHeap->pFree);
        DLOGV("*******************************************");
    }

    pBlock = pAivHeap->pFree;
    // walk the free blocks
    while (pBlock != NULL) {
        if (dump) {
            DLOGV("Block:\t%p\t\tsize:\t%" PRIu64, pBlock, GET_AIV_ALLOCATION_SIZE(pBlock));
        }

        if (pBlock->header.flags != ALLOCATION_FLAGS_FREE) {
            DLOGE("Block %p is in free list but doesn't have it's flag set as free", pBlock);
            retStatus = STATUS_HEAP_CORRUPTED;
        }

        if (GET_AIV_ALLOCATION_SIZE(pBlock) != GET_AIV_ALLOCATION_FOOTER_SIZE(pBlock)) {
            DLOGE("Block %p header and footer allocation sizes mismatch", pBlock);
            retStatus = STATUS_HEAP_CORRUPTED;
        }
#ifdef HEAP_DEBUG
        // Check the allocation 'guard band' in debug mode
        if (0 != MEMCMP(((PALLOCATION_HEADER) pBlock)->magic, ALLOCATION_HEADER_MAGIC, SIZEOF(ALLOCATION_HEADER_MAGIC))) {
            DLOGE("Invalid header for allocation %p", pBlock);
            retStatus = STATUS_HEAP_CORRUPTED;
        }

        // Check the footer
        if (0 != MEMCMP(GET_AIV_ALLOCATION_FOOTER(pBlock)->magic, ALLOCATION_FOOTER_MAGIC, SIZEOF(ALLOCATION_FOOTER_MAGIC))) {
            DLOGE("Invalid footer for allocation %p", pBlock);
            retStatus = STATUS_HEAP_CORRUPTED;
        }
#endif
        pBlock = pBlock->pNext;
    }

    if (dump) {
        DLOGV("*******************************************");
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
    pBaseHeap->heapSetAllocSizeFn = aivHeapSetAllocSize;
    pBaseHeap->heapMapFn = aivHeapMap;
    pBaseHeap->heapUnmapFn = aivHeapUnmap;
    pBaseHeap->heapDebugCheckAllocatorFn = aivHeapDebugCheckAllocator;
    pBaseHeap->getAllocationSizeFn = aivGetAllocationSize;
    pBaseHeap->getAllocationHeaderSizeFn = aivGetAllocationHeaderSize;
    pBaseHeap->getAllocationFooterSizeFn = aivGetAllocationFooterSize;
    pBaseHeap->getAllocationAlignedSizeFn = aivGetAllocationAlignedSize;
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

    // We will align heap limit to ensure the allocations are always aligned
    heapLimit = HEAP_PACKED_SIZE(heapLimit);

    // Call the base functionality
    CHK_STATUS(commonHeapInit(pHeap, heapLimit));

    // Allocate the entire heap backed by the process default heap.
    pAivHeap->pAllocation = MEMALLOC((SIZE_T) heapLimit);
    CHK_ERR(pAivHeap->pAllocation != NULL, STATUS_NOT_ENOUGH_MEMORY, "Failed to allocate heap with limit size %" PRIu64, heapLimit);

#ifdef HEAP_DEBUG
    // Null the memory in debug mode
    MEMSET(pAivHeap->pAllocation, 0x00, (SIZE_T) heapLimit);
#endif

    // Initialize the allocation as a single free block
    MEMCPY(pAivHeap->pAllocation, &gAivHeader, AIV_ALLOCATION_HEADER_SIZE);
    MEMCPY((PBYTE) pAivHeap->pAllocation + pHeap->heapLimit - AIV_ALLOCATION_FOOTER_SIZE, &gAivFooter, AIV_ALLOCATION_FOOTER_SIZE);

    // Set the sizes. The size will only contain the raw size with no service field.
    SET_AIV_ALLOCATION_SIZE(pAivHeap->pAllocation, pHeap->heapLimit - AIV_ALLOCATION_HEADER_SIZE - AIV_ALLOCATION_FOOTER_SIZE);
    SET_AIV_ALLOCATION_FOOTER_SIZE(pAivHeap->pAllocation);

    // Set the free pointer
    pAivHeap->pFree = (PAIV_ALLOCATION_HEADER) pAivHeap->pAllocation;

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
    CHK(pHeap != NULL, STATUS_SUCCESS);

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
    if (NULL == pFree) {
        // Make sure we decrement the counters by calling decrement
        decrementUsage(pHeap, AIV_ALLOCATION_HEADER_SIZE + HEAP_PACKED_SIZE(size) + AIV_ALLOCATION_FOOTER_SIZE);

        CHK(FALSE, STATUS_SUCCESS);
    }

    // Split the free block
    splitFreeBlock(pAivHeap, pFree, size);

    // Add the block to the allocated list
    addAllocatedBlock(pAivHeap, pFree);

    // Set the return value of the handle to be the allocation address as this is a
    // direct allocation and doesn't support mapping.
    // IMPORTANT!!! We are not returning the actual address but rather the offset
    // from the heap base in the high-order 32 bits. This is done so we can
    // later differentiate between direct memory allocations and hybrid heap allocation
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
    CHK_AIV_ALLOCATION(pAivHeap, pAllocation);

    // Perform the de-allocation
    pAlloc = (PAIV_ALLOCATION_HEADER) pAllocation - 1;

    CHK_ERR(pAlloc->header.flags == ALLOCATION_FLAGS_ALLOC && pAlloc->allocSize != 0, STATUS_INVALID_HANDLE_ERROR,
            "Invalid block of memory passed to free.");

    // Call the common heap function
    CHK_STATUS(commonHeapFree(pHeap, handle));

    // Remove from the allocated
    removeChainedBlock(pAivHeap, pAlloc);

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
    CHK_AIV_ALLOCATION(pAivHeap, pAllocation);

    // Call the common heap function
    CHK_STATUS(commonHeapGetAllocSize(pHeap, handle, pAllocSize));

    pHeader = (PAIV_ALLOCATION_HEADER) pAllocation - 1;

    // Check for the validity of the allocation
    CHK_ERR(pHeader->header.flags == ALLOCATION_FLAGS_ALLOC && pHeader->allocSize != 0, STATUS_INVALID_HANDLE_ERROR,
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
    CHK_AIV_ALLOCATION(pAivHeap, pAllocation);

    // Call the common heap function
    CHK_STATUS(commonHeapMap(pHeap, handle, ppAllocation, pSize));

    *ppAllocation = pAllocation;
    pHeader = (PAIV_ALLOCATION_HEADER) pAllocation - 1;

    // Check for the validity of the allocation
    CHK_ERR(pHeader->header.flags == ALLOCATION_FLAGS_ALLOC && pHeader->allocSize != 0, STATUS_INVALID_HANDLE_ERROR,
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

DEFINE_ALIGNED_SIZE(aivGetAllocationAlignedSize)
{
    return HEAP_PACKED_SIZE(size);
}

DEFINE_ALLOC_SIZE(aivGetAllocationSize)
{
    // This is a direct allocation
    PVOID pAllocation = FROM_AIV_HANDLE((PAivHeap) pHeap, handle);
    PAIV_ALLOCATION_HEADER pHeader = GET_AIV_ALLOCATION_HEADER(pAllocation);

#ifdef HEAP_DEBUG
    // Check the allocation 'guard band' in debug mode
    if (0 != MEMCMP(((PALLOCATION_HEADER) pHeader)->magic, ALLOCATION_HEADER_MAGIC, SIZEOF(ALLOCATION_HEADER_MAGIC))) {
        DLOGE("Invalid header for allocation %p", pAllocation);
        return INVALID_ALLOCATION_VALUE;
    }

    // Check the footer
    if (0 != MEMCMP(GET_AIV_ALLOCATION_FOOTER(pHeader)->magic, ALLOCATION_FOOTER_MAGIC, SIZEOF(ALLOCATION_FOOTER_MAGIC))) {
        DLOGE("Invalid footer for allocation %p", pAllocation);
        return INVALID_ALLOCATION_VALUE;
    }

    // Check the type
    if (AIV_ALLOCATION_TYPE != ((PALLOCATION_HEADER) pHeader)->type) {
        DLOGE("Invalid allocation type 0x%08x", ((PALLOCATION_HEADER) pHeader)->type);
        return INVALID_ALLOCATION_VALUE;
    }

    // Check the allocation size against the overall allocation for the block
    if (pHeader->allocSize > GET_AIV_ALLOCATION_SIZE(pHeader)) {
        DLOGE("Block %p has a requested size of %u which is greater than the allocation size %u", pHeader, pHeader->allocSize,
              GET_AIV_ALLOCATION_SIZE(pHeader));
        return INVALID_ALLOCATION_VALUE;
    }
#endif

    return AIV_ALLOCATION_HEADER_SIZE + GET_AIV_ALLOCATION_SIZE(pHeader) + AIV_ALLOCATION_FOOTER_SIZE;
}

/**
 * Sets the allocation size
 */
DEFINE_HEAP_SET_ALLOC_SIZE(aivHeapSetAllocSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PAIV_ALLOCATION_HEADER pExistingHeader, pCheckFree;
    PVOID pAllocation = NULL;
    PAivHeap pAivHeap = (PAivHeap) pHeap;
    UINT64 blockDiffSize, diffSize;
    ALLOCATION_HANDLE existingAllocationHandle, newAllocationHandle = INVALID_ALLOCATION_HANDLE_VALUE;
    PVOID pExistingBuffer = NULL, pNewBuffer = NULL;

    // Call the common heap function
    CHK_STATUS(commonHeapSetAllocSize(pHeap, pHandle, size, newSize));

    existingAllocationHandle = *pHandle;

    // Diff of the size
    diffSize = newSize > size ? newSize - size : size - newSize;

    // This heap implementation uses a direct memory allocation so no mapping really needed - just conversion from a handle to memory pointer
    pAllocation = FROM_AIV_HANDLE(pAivHeap, existingAllocationHandle);
    CHK_AIV_ALLOCATION(pAivHeap, pAllocation);

    pExistingHeader = GET_AIV_ALLOCATION_HEADER(pAllocation);

    // Check if we are trying to allocate a slightly larger block and it fits in the existing allocation
    // or if we are trying to allocate a slightly smaller block size and we will try to fit into existing allocation
    blockDiffSize = GET_AIV_ALLOCATION_SIZE(pExistingHeader) - pExistingHeader->allocSize;
    if ((newSize > size && diffSize <= blockDiffSize) || (newSize < size && (blockDiffSize + diffSize <= MIN_FREE_BLOCK_SIZE))) {
        // No need to set the allocation handle.
        pExistingHeader->allocSize = newSize;

        // Early exit
        CHK(FALSE, retStatus);
    }

    // Check if we are trying to allocate a smaller block
    if (newSize < size) {
        // Set the existing allocation size first
        pExistingHeader->allocSize = newSize;

        // Splitting the existing block
        splitAllocatedBlock(pAivHeap, pExistingHeader, newSize);

        // Early exit
        CHK(FALSE, retStatus);
    }

    // Get the right block and see if we are within the range and if it's free
    pCheckFree = getRightBlock(pAivHeap, pExistingHeader);
    if (pCheckFree != NULL && pCheckFree->header.flags == ALLOCATION_FLAGS_FREE &&
        (GET_AIV_ALLOCATION_SIZE(pCheckFree) + AIV_ALLOCATION_FOOTER_SIZE + AIV_ALLOCATION_HEADER_SIZE + blockDiffSize >= diffSize)) {
        // Append the part of the free memory to the right
        coalesceFreeToAllocatedBlock(pAivHeap, pExistingHeader, pCheckFree, diffSize);

        // Early exit
        CHK(FALSE, retStatus);
    }

    // Otherwise, we are actually allocating a larger block
    // 1) Allocating new buffer
    // 2) Mapping the existing allocation
    // 3) Mapping the new allocation
    // 4) Copying the data from old to new
    // 5) Unmapping the existing allocation
    // 6) Unmapping the new allocation
    // 7) Free-ing the old allocation

    DLOGS("Sets new allocation size %\" PRIu64 \" for handle 0x%016" PRIx64, newSize, existingAllocationHandle);

    CHK_STATUS(aivHeapAlloc(pHeap, newSize, &newAllocationHandle));
    CHK(IS_VALID_ALLOCATION_HANDLE(newAllocationHandle), STATUS_NOT_ENOUGH_MEMORY);
    CHK_STATUS(aivHeapMap(pHeap, existingAllocationHandle, &pExistingBuffer, &size));
    CHK_STATUS(aivHeapMap(pHeap, newAllocationHandle, &pNewBuffer, &newSize));
    MEMCPY(pNewBuffer, pExistingBuffer, MIN(size, newSize));
    CHK_STATUS(aivHeapUnmap(pHeap, pExistingBuffer));
    CHK_STATUS(aivHeapUnmap(pHeap, pNewBuffer));
    CHK_STATUS(aivHeapFree(pHeap, existingAllocationHandle));

    // Set the return
    *pHandle = newAllocationHandle;

CleanUp:

    // Clean-up in case of failure
    if (STATUS_FAILED(retStatus) && IS_VALID_ALLOCATION_HANDLE(newAllocationHandle)) {
        // Free the allocation before returning
        aivHeapFree(pHeap, newAllocationHandle);
    }

    LEAVES();
    return retStatus;
}

DEFINE_HEAP_LIMITS(aivGetHeapLimits)
{
    *pMinHeapSize = MIN_HEAP_SIZE;
    *pMaxHeapSize = MAX_HEAP_SIZE;
}

/**
 * Simple first free block finding
 */
PAIV_ALLOCATION_HEADER getFreeBlock(PAivHeap pAivHeap, UINT64 size)
{
    PAIV_ALLOCATION_HEADER pFree = pAivHeap->pFree;

    // Perform the allocation by looking for the first fit in the free list
    while (pFree != NULL) {
        // check for the fit
        if (GET_AIV_ALLOCATION_SIZE(pFree) >= HEAP_PACKED_SIZE(size)) {
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
VOID splitFreeBlock(PAivHeap pAivHeap, PAIV_ALLOCATION_HEADER pBlock, UINT64 size)
{
    PAIV_ALLOCATION_HEADER pNewFree = NULL;
    UINT64 alignedSize = HEAP_PACKED_SIZE(size);

    // There are two scenarios - whether the new header + minimal block size will fit or not
    // In case we end up with smaller block then we will just attach that to the allocated
    if (GET_AIV_ALLOCATION_SIZE(pBlock) < alignedSize + MIN_FREE_BLOCK_SIZE) {
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

#ifdef HEAP_DEBUG
        // Zero the memory in debug mode
        MEMSET(pBlock + 1, 0x00, (SIZE_T) GET_AIV_ALLOCATION_SIZE(pBlock));
#endif

        // we also need to fix-up the heap size due to larger allocation
        ((PHeap) pAivHeap)->heapSize += GET_AIV_ALLOCATION_SIZE(pBlock) - alignedSize;
    } else {
        // split the block
        // Prepare the new free block
        pNewFree = (PAIV_ALLOCATION_HEADER)((PBYTE)(pBlock + 1) + alignedSize + AIV_ALLOCATION_FOOTER_SIZE);

        // Set the header
        MEMCPY(pNewFree, &gAivHeader, AIV_ALLOCATION_HEADER_SIZE);

        // Set the header and footer size and link to the chain
        SET_AIV_ALLOCATION_SIZE(pNewFree, GET_AIV_ALLOCATION_SIZE(pBlock) - alignedSize - AIV_ALLOCATION_HEADER_SIZE - AIV_ALLOCATION_FOOTER_SIZE);
        SET_AIV_ALLOCATION_FOOTER_SIZE(pNewFree);

        // Set the type of the block
        pNewFree->header.flags = ALLOCATION_FLAGS_FREE;

        // Linking it in
        pNewFree->pNext = pBlock->pNext;
        pNewFree->pPrev = pBlock->pPrev;

#ifdef HEAP_DEBUG
        // Null the memory in debug mode
        MEMSET(pNewFree + 1, 0x00, (SIZE_T) GET_AIV_ALLOCATION_SIZE(pNewFree));
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

        // adjust the free block size
        SET_AIV_ALLOCATION_SIZE(pBlock, alignedSize);

        // set the existing allocations footer
        MEMCPY((PBYTE)(pBlock + 1) + GET_AIV_ALLOCATION_SIZE(pBlock), &gAivFooter, AIV_ALLOCATION_FOOTER_SIZE);
        SET_AIV_ALLOCATION_FOOTER_SIZE(pBlock);
    }

    // Nullify the pointers
    pBlock->pNext = pBlock->pPrev = NULL;

    // Set as undefined
    pBlock->header.flags = ALLOCATION_FLAGS_NONE;

    // Set the actual allocation size which might be smaller than the overall block size
    pBlock->allocSize = size;
}

/**
 * Splits the allocated block and insert the free block to the free chain.
 *
 * IMPORTANT: The checks have been already performed for the validity.
 */
VOID splitAllocatedBlock(PAivHeap pAivHeap, PAIV_ALLOCATION_HEADER pBlock, UINT64 size)
{
    PAIV_ALLOCATION_HEADER pNewFree = NULL;
    UINT64 alignedSize = HEAP_PACKED_SIZE(size);

    CHECK_EXT(GET_AIV_ALLOCATION_SIZE(pBlock) >= size + MIN_FREE_BLOCK_SIZE, "Invalid block size to split.");

    // Prepare the new free block
    pNewFree = (PAIV_ALLOCATION_HEADER)((PBYTE)(pBlock + 1) + alignedSize + AIV_ALLOCATION_FOOTER_SIZE);

    // Set the header
    MEMCPY(pNewFree, &gAivHeader, AIV_ALLOCATION_HEADER_SIZE);

    // Set the size and link to the chain
    // Set the header and footer size and link to the chain
    SET_AIV_ALLOCATION_SIZE(pNewFree, GET_AIV_ALLOCATION_SIZE(pBlock) - alignedSize - AIV_ALLOCATION_HEADER_SIZE - AIV_ALLOCATION_FOOTER_SIZE);
    SET_AIV_ALLOCATION_FOOTER_SIZE(pNewFree);

    pNewFree->pNext = pNewFree->pPrev = NULL;

    // Set the type of the block to NONE
    pNewFree->header.flags = ALLOCATION_FLAGS_NONE;

    // Set the blocks sizes
    pBlock->allocSize = size;
    SET_AIV_ALLOCATION_SIZE(pBlock, alignedSize);

    // set the existing allocations footer
    MEMCPY((PBYTE)(pBlock + 1) + GET_AIV_ALLOCATION_SIZE(pBlock), &gAivFooter, AIV_ALLOCATION_FOOTER_SIZE);
    SET_AIV_ALLOCATION_FOOTER_SIZE(pBlock);

    // Add the remainder to the free chain
    addFreeBlock(pAivHeap, pNewFree);

#ifdef HEAP_DEBUG
    // Null the memory in debug mode
    MEMSET(pNewFree + 1, 0x00, (SIZE_T) GET_AIV_ALLOCATION_SIZE(pNewFree));
#endif
}

/**
 * Coalesce a right free block to the allocation, potentially splitting it.
 */
VOID coalesceFreeToAllocatedBlock(PAivHeap pAivHeap, PAIV_ALLOCATION_HEADER pAlloc, PAIV_ALLOCATION_HEADER pFree, UINT64 diffSize)
{
    PAIV_ALLOCATION_HEADER pNewFree = NULL, pNext, pPrev;
    UINT64 freeSize = GET_AIV_ALLOCATION_SIZE(pFree);
    UINT64 blockSize = GET_AIV_ALLOCATION_SIZE(pAlloc);
    UINT64 alignedDiffSize = HEAP_PACKED_SIZE(diffSize);

    // There are two scenarios - whether the new header + minimal block size will fit or not
    // In case we end up with smaller block then we will just attach that to the allocated
    if (freeSize < alignedDiffSize + MIN_FREE_ALLOCATION_SIZE) {
        // use the entire block
        // Fix the next block
        if (pFree->pNext != NULL) {
            pFree->pNext->pPrev = pFree->pPrev;
        }

        // Fix the prev block
        if (pFree->pPrev != NULL) {
            pFree->pPrev->pNext = pFree->pNext;
        } else {
            // this is the case where we need to Fix-up the pAivHeap->pFree
            CHECK_EXT(pAivHeap->pFree == pFree, "Free block pointer is invalid");
            pAivHeap->pFree = pFree->pNext;
        }

#ifdef HEAP_DEBUG
        // Null the memory in debug mode
        MEMSET((PBYTE)(pFree + 1), 0x00, (SIZE_T) freeSize);
#endif

        SET_AIV_ALLOCATION_SIZE(pAlloc, blockSize + freeSize + AIV_ALLOCATION_HEADER_SIZE + AIV_ALLOCATION_FOOTER_SIZE);
        pAlloc->allocSize += diffSize;

        // Footer has already been set so we need to adjust the size
        SET_AIV_ALLOCATION_FOOTER_SIZE(pAlloc);
    } else {
        // split the block
        // Prepare the new free block without accounting for the header.
        // The new footer will be accounted for from the allocated footer
        // to move to the new position and fixed up.
        pNewFree = (PAIV_ALLOCATION_HEADER)((PBYTE) pFree + alignedDiffSize);

        // Store the existing next and prev
        pNext = pFree->pNext;
        pPrev = pFree->pPrev;

        // Set the header for the new free block
        MEMCPY(pNewFree, &gAivHeader, AIV_ALLOCATION_HEADER_SIZE);

        // Re-chain the new free block
        pNewFree->pNext = pNext;
        pNewFree->pPrev = pPrev;

        // Fix the next block
        if (pNewFree->pNext != NULL) {
            pNewFree->pNext->pPrev = pNewFree;
        }

        // Fix the prev block
        if (pNewFree->pPrev != NULL) {
            pNewFree->pPrev->pNext = pNewFree;
        } else {
            // This is the case where we need to Fix-up the pAivHeap->pFree
            CHECK_EXT(pAivHeap->pFree == pFree, "Free block pointer is invalid");
            pAivHeap->pFree = pNewFree;
        }

        // Set the size and link to the chain
        SET_AIV_ALLOCATION_SIZE(pNewFree, freeSize - alignedDiffSize);
        SET_AIV_ALLOCATION_FOOTER_SIZE(pNewFree);

        // Set the type of the block
        pNewFree->header.flags = ALLOCATION_FLAGS_FREE;

        // set the new footer for the allocated
        MEMCPY((PBYTE) pNewFree - AIV_ALLOCATION_FOOTER_SIZE, &gAivFooter, AIV_ALLOCATION_FOOTER_SIZE);

        // Set the alloc sizes
        pAlloc->allocSize += diffSize;
        SET_AIV_ALLOCATION_SIZE(pAlloc, blockSize + alignedDiffSize);
        SET_AIV_ALLOCATION_FOOTER_SIZE(pAlloc);

#ifdef HEAP_DEBUG
        // Null the memory in debug mode
        MEMSET((PBYTE)(pAlloc + 1) + blockSize, 0x00, (SIZE_T) alignedDiffSize);
#endif
    }
}

/**
 * Adds the newly allocated block to the head of the allocations.
 */
VOID addAllocatedBlock(PAivHeap pAivHeap, PAIV_ALLOCATION_HEADER pBlock)
{
    CHECK(pAivHeap != NULL && pBlock != NULL && ((PALLOCATION_HEADER) pBlock)->size > 0 && pBlock->pNext == NULL && pBlock->pPrev == NULL &&
          pBlock->header.flags == ALLOCATION_FLAGS_NONE);

    // Set as allocated
    pBlock->header.flags = ALLOCATION_FLAGS_ALLOC;

    if (pAivHeap->pAlloc != NULL) {
        // This is the case when we do have the allocated chain
        pBlock->pNext = pAivHeap->pAlloc;
        pAivHeap->pAlloc->pPrev = pBlock;
    }

    // Set the new head
    pAivHeap->pAlloc = pBlock;
}

/**
 * Removes a free block from the free or allocated list by checking the block flags
 */
VOID removeChainedBlock(PAivHeap pAivHeap, PAIV_ALLOCATION_HEADER pBlock)
{
    CHECK(pAivHeap != NULL && pBlock != NULL && pBlock->header.flags != ALLOCATION_FLAGS_NONE && GET_AIV_ALLOCATION_SIZE(pBlock) > 0);

    // Fix-up the prev
    if (pBlock->pPrev == NULL) {
        // This is the case of the first block
        if (pBlock->header.flags == ALLOCATION_FLAGS_FREE) {
            CHECK_EXT(pAivHeap->pFree == pBlock, "Free Block pointer is invalid");
            pAivHeap->pFree = pBlock->pNext;
        } else {
            CHECK_EXT(pAivHeap->pAlloc == pBlock, "Alloc Block pointer is invalid");
            pAivHeap->pAlloc = pBlock->pNext;
        }
    } else {
        pBlock->pPrev->pNext = pBlock->pNext;
    }

    // Fix-up the next
    if (pBlock->pNext != NULL) {
        pBlock->pNext->pPrev = pBlock->pPrev;
    }

    // Set the status as undefined
    pBlock->header.flags = ALLOCATION_FLAGS_NONE;

    // Nullify the links
    pBlock->pNext = pBlock->pPrev = NULL;

    // Set the requested allocation size to 0
    pBlock->allocSize = 0;
}

/**
 * Add a new block to the free list and handle coalescence
 */
VOID addFreeBlock(PAivHeap pAivHeap, PAIV_ALLOCATION_HEADER pBlock)
{
    CHECK(pAivHeap != NULL && pBlock != NULL && GET_AIV_ALLOCATION_SIZE(pBlock) > 0 && pBlock->pNext == NULL && pBlock->pPrev == NULL &&
          pBlock->header.flags == ALLOCATION_FLAGS_NONE);

    // Find the spot and add the block first
    // and then try to coalesce the neighboring blocks

    // Special case with early return if it's the first block
    if (pAivHeap->pFree == NULL) {
        // Set as free
        pBlock->header.flags = ALLOCATION_FLAGS_FREE;

        // Set the first free block
        pAivHeap->pFree = pBlock;
        return;
    }

    UINT64 blockSize = GET_AIV_ALLOCATION_SIZE(pBlock);
    UINT64 overallSize;

    // Get the left block
    PAIV_ALLOCATION_HEADER pLeft = getLeftBlock(pAivHeap, pBlock);

    // Check for coalescence
    if (pLeft != NULL && pLeft->header.flags == ALLOCATION_FLAGS_FREE) {
        overallSize = GET_AIV_ALLOCATION_SIZE(pLeft) + blockSize + AIV_ALLOCATION_HEADER_SIZE + AIV_ALLOCATION_FOOTER_SIZE;
        SET_AIV_ALLOCATION_SIZE(pLeft, overallSize);
        SET_AIV_ALLOCATION_FOOTER_SIZE(pLeft);

        // Reset the variables
        pBlock = pLeft;
        blockSize = overallSize;
    }

    // Get the right block
    PAIV_ALLOCATION_HEADER pRight = getRightBlock(pAivHeap, pBlock);
    if (pRight != NULL && pRight->header.flags == ALLOCATION_FLAGS_FREE) {
        // Remove the right block from free
        removeChainedBlock(pAivHeap, pRight);

        overallSize = GET_AIV_ALLOCATION_SIZE(pRight) + blockSize + AIV_ALLOCATION_HEADER_SIZE + AIV_ALLOCATION_FOOTER_SIZE;
        SET_AIV_ALLOCATION_SIZE(pBlock, overallSize);
        SET_AIV_ALLOCATION_FOOTER_SIZE(pBlock);
    }

    // Chain it in if needed hasn't been chained in
    if (pBlock->header.flags == ALLOCATION_FLAGS_NONE) {
        pBlock->header.flags = ALLOCATION_FLAGS_FREE;
        pBlock->pNext = pAivHeap->pFree;
        pBlock->pPrev = NULL;
        if (pBlock->pNext != NULL) {
            pBlock->pNext->pPrev = pBlock;
        }

        // Set the new free header
        pAivHeap->pFree = pBlock;
    }
}

/**
 * Checks if the free blocks overlap
 */
BOOL checkOverlap(PAIV_ALLOCATION_HEADER pBlock1, PAIV_ALLOCATION_HEADER pBlock2)
{
    if (pBlock1 < pBlock2) {
        return (PBYTE)(pBlock1 + 1) + GET_AIV_ALLOCATION_SIZE(pBlock1) + AIV_ALLOCATION_FOOTER_SIZE > (PBYTE) pBlock2;
    } else {
        return (PBYTE)(pBlock2 + 1) + GET_AIV_ALLOCATION_SIZE(pBlock2) + AIV_ALLOCATION_FOOTER_SIZE > (PBYTE) pBlock1;
    }
}

/**
 * Gets the left block. NULL if out of boundaries
 */
PAIV_ALLOCATION_HEADER getLeftBlock(PAivHeap pAivHeap, PAIV_ALLOCATION_HEADER pBlock)
{
    // Assuming all the parameters have been sanitized as this is an internal functionality

    // Check for the boundary
    if ((PBYTE) pAivHeap->pAllocation >= (PBYTE) pBlock) {
        return NULL;
    }

    PALLOCATION_FOOTER pFooter = ((PALLOCATION_FOOTER) pBlock) - 1;

    UINT64 blockSize = pFooter->size;

    PAIV_ALLOCATION_HEADER pHeader = (PAIV_ALLOCATION_HEADER)((PBYTE) pFooter - blockSize - AIV_ALLOCATION_HEADER_SIZE);

    CHECK_EXT((PBYTE) pHeader >= (PBYTE) pAivHeap->pAllocation, "Heap corrupted or invalid allocation");

    return pHeader;
}

/**
 * Gets the right block. NULL if out of boundaries
 */
PAIV_ALLOCATION_HEADER getRightBlock(PAivHeap pAivHeap, PAIV_ALLOCATION_HEADER pBlock)
{
    // Assuming all the parameters have been sanitized as this is an internal functionality
    UINT64 blockSize = GET_AIV_ALLOCATION_SIZE(pBlock);
    PAIV_ALLOCATION_HEADER pHeader = (PAIV_ALLOCATION_HEADER)((PBYTE)(pBlock + 1) + blockSize + AIV_ALLOCATION_FOOTER_SIZE);
    PBYTE pHeapLimit = (PBYTE) pAivHeap->pAllocation + ((PHeap) pAivHeap)->heapLimit;

    // Check for the boundary
    if (pHeapLimit <= (PBYTE) pHeader) {
        return NULL;
    }

    CHECK_EXT(pHeapLimit >= (PBYTE) GET_AIV_ALLOCATION_FOOTER(pHeader) + AIV_ALLOCATION_FOOTER_SIZE, "Heap corrupted or invalid allocation");

    return pHeader;
}
