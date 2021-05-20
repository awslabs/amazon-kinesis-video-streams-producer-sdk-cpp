/**
 * Implementation of a heap based on Process'es System heap
 */

#define LOG_CLASS "SystemHeap"
#include "Include_i.h"

#ifdef HEAP_DEBUG
ALLOCATION_HEADER gSysHeader = {0, SYS_ALLOCATION_TYPE, 0, ALLOCATION_HEADER_MAGIC};
ALLOCATION_FOOTER gSysFooter = {1, ALLOCATION_FOOTER_MAGIC};

#define SYS_ALLOCATION_FOOTER_SIZE SIZEOF(gSysFooter)

#else
ALLOCATION_HEADER gSysHeader = {0, SYS_ALLOCATION_TYPE, 0};
ALLOCATION_FOOTER gSysFooter = {0};

#define SYS_ALLOCATION_FOOTER_SIZE 0

#endif

#define SYS_ALLOCATION_HEADER_SIZE SIZEOF(gSysHeader)

/**
 * Debug print analytics information
 */
DEFINE_HEAP_CHK(sysHeapDebugCheckAllocator)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    // Delegate the call directly
    CHK_STATUS(commonHeapDebugCheckAllocator(pHeap, dump));

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Creates the heap
 */
DEFINE_CREATE_HEAP(sysHeapCreate)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PBaseHeap pBaseHeap = NULL;
    CHK_STATUS(commonHeapCreate(ppHeap, SIZEOF(BaseHeap)));

    // Set the function pointers
    pBaseHeap = (PBaseHeap) *ppHeap;
    pBaseHeap->heapInitializeFn = sysHeapInit;
    pBaseHeap->heapReleaseFn = sysHeapRelease;
    pBaseHeap->heapGetSizeFn = commonHeapGetSize; // Use the common heap functionality
    pBaseHeap->heapAllocFn = sysHeapAlloc;
    pBaseHeap->heapFreeFn = sysHeapFree;
    pBaseHeap->heapGetAllocSizeFn = sysHeapGetAllocSize;
    pBaseHeap->heapSetAllocSizeFn = sysHeapSetAllocSize;
    pBaseHeap->heapMapFn = sysHeapMap;
    pBaseHeap->heapUnmapFn = sysHeapUnmap;
    pBaseHeap->heapDebugCheckAllocatorFn = sysHeapDebugCheckAllocator;
    pBaseHeap->getAllocationSizeFn = sysGetAllocationSize;
    pBaseHeap->getAllocationHeaderSizeFn = sysGetAllocationHeaderSize;
    pBaseHeap->getAllocationFooterSizeFn = sysGetAllocationFooterSize;
    pBaseHeap->getAllocationAlignedSizeFn = sysGetAllocationAlignedSize;
    pBaseHeap->getHeapLimitsFn = sysGetHeapLimits;

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Initialize the heap
 */
DEFINE_INIT_HEAP(sysHeapInit)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    // Delegate the call directly
    CHK_STATUS(commonHeapInit(pHeap, heapLimit));

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Releases the heap
 */
DEFINE_RELEASE_HEAP(sysHeapRelease)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    // Delegate the call directly
    CHK_STATUS(commonHeapRelease(pHeap));

    // Free the object itself
    MEMFREE(pHeap);

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Allocate from the heap
 */
DEFINE_HEAP_ALLOC(sysHeapAlloc)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 overallSize = 0;
    PALLOCATION_HEADER pHeader = NULL;

    // Call the common heap function
    retStatus = commonHeapAlloc(pHeap, size, pHandle);
    CHK(retStatus == STATUS_NOT_ENOUGH_MEMORY || retStatus == STATUS_SUCCESS, retStatus);
    if (retStatus == STATUS_NOT_ENOUGH_MEMORY) {
        // If we are out of memory then we don't need to return a failure - just
        // Early return with success
        CHK(FALSE, STATUS_SUCCESS);
    }

    // overall allocation size
    overallSize = SYS_ALLOCATION_HEADER_SIZE + size + SYS_ALLOCATION_FOOTER_SIZE;

    // Validate for 32 bit systems
    CHK(CHECK_64_BIT || overallSize <= MAX_UINT32, STATUS_INVALID_ALLOCATION_SIZE);

    // Perform the allocation
    if (NULL == (pHeader = (PALLOCATION_HEADER) MEMALLOC((SIZE_T) overallSize))) {
        DLOGV("Failed to allocate %" PRIu64 "bytes from the heap", overallSize);

        // Make sure we decrement the counters by calling decrement
        decrementUsage(pHeap, overallSize);

        // Early return with success
        CHK(FALSE, STATUS_SUCCESS);
    }

#ifdef HEAP_DEBUG
    // Null the memory in debug mode
    MEMSET(pHeader, 0x00, (SIZE_T) overallSize);
#endif
    // Set up the header and footer
    MEMCPY(pHeader, &gSysHeader, SYS_ALLOCATION_HEADER_SIZE);
    MEMCPY((PBYTE) pHeader + SYS_ALLOCATION_HEADER_SIZE + size, &gSysFooter, SYS_ALLOCATION_FOOTER_SIZE);

    // Fix-up the allocation size
    pHeader->size = size;

    // Setting the return value
    *pHandle = (ALLOCATION_HANDLE)((PBYTE) pHeader + SYS_ALLOCATION_HEADER_SIZE);

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Free the allocation
 */
DEFINE_HEAP_FREE(sysHeapFree)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PBYTE pAllocation;
    PALLOCATION_HEADER pHeader;

    // Call the common heap function
    CHK_STATUS(commonHeapFree(pHeap, handle));

    // This is a direct memory allocation so the memory pointer is stored as a handle
    pAllocation = HANDLE_TO_POINTER(handle);
    pHeader = (PALLOCATION_HEADER) pAllocation - 1;

#ifdef HEAP_DEBUG
    // Null the memory in debug mode
    MEMSET(pHeader, 0x00, (SIZE_T)(SYS_ALLOCATION_HEADER_SIZE + pHeader->size + SYS_ALLOCATION_FOOTER_SIZE));
#endif

    // Perform the de-allocation - will cause corruption if invalid pointer is passed in
    MEMFREE(pHeader);

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Gets the allocation size
 */
DEFINE_HEAP_GET_ALLOC_SIZE(sysHeapGetAllocSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PALLOCATION_HEADER pHeader;

    // This heap implementation uses a direct memory allocation so no mapping really needed - just conversion from a handle to memory pointer
    PVOID pAllocation = (PVOID) HANDLE_TO_POINTER(handle);

    // Call the common heap function
    CHK_STATUS(commonHeapGetAllocSize(pHeap, handle, pAllocSize));

    pHeader = (PALLOCATION_HEADER) pAllocation - 1;

    // Set the size
    *pAllocSize = pHeader->size;

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Sets the allocation size
 */
DEFINE_HEAP_SET_ALLOC_SIZE(sysHeapSetAllocSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PALLOCATION_HEADER pExistingHeader, pNewHeader;
    PVOID pAllocation = NULL;
    UINT64 overallSize;

    // Call the common heap function
    CHK_STATUS(commonHeapSetAllocSize(pHeap, pHandle, size, newSize));

    // overall allocation size
    overallSize = SYS_ALLOCATION_HEADER_SIZE + size + SYS_ALLOCATION_FOOTER_SIZE;

    // This heap implementation uses a direct memory allocation so no mapping really needed - just conversion from a handle to memory pointer
    pAllocation = (PVOID) HANDLE_TO_POINTER(*pHandle);

    pExistingHeader = (PALLOCATION_HEADER) pAllocation - 1;

    // Re-allocation might return a different pointer
    if (NULL == (pNewHeader = (PALLOCATION_HEADER) MEMREALLOC(pExistingHeader, (SIZE_T) overallSize))) {
        DLOGV("Failed to reallocate %" PRIu64 "bytes from the heap", overallSize);

        // Make sure we reset the overall size on failure
        if (newSize > size) {
            decrementUsage(pHeap, newSize - size);
        } else {
            incrementUsage(pHeap, size - newSize);
        }

        // Early return with success
        CHK(FALSE, STATUS_HEAP_REALLOC_ERROR);
    }

#ifdef HEAP_DEBUG
    // Null the memory in debug mode
    MEMSET(pNewHeader, 0x00, (SIZE_T) overallSize);
#endif
    // Set up the header and footer
    MEMCPY(pNewHeader, &gSysHeader, SYS_ALLOCATION_HEADER_SIZE);
    MEMCPY((PBYTE) pNewHeader + SYS_ALLOCATION_HEADER_SIZE + newSize, &gSysFooter, SYS_ALLOCATION_FOOTER_SIZE);

    // Fix-up the allocation size
    pNewHeader->size = newSize;

    // Setting the return value
    *pHandle = (ALLOCATION_HANDLE)((PBYTE) pNewHeader + SYS_ALLOCATION_HEADER_SIZE);

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Map the allocation. This is simply a casting operation as we have direct allocation and non vRAM backed
 */
DEFINE_HEAP_MAP(sysHeapMap)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PALLOCATION_HEADER pHeader;

    // This heap implementation uses a direct memory allocation so no mapping really needed - just conversion from a handle to memory pointer
    PVOID pAllocation = (PVOID) HANDLE_TO_POINTER(handle);

    // Call the common heap function
    CHK_STATUS(commonHeapMap(pHeap, handle, ppAllocation, pSize));

    *ppAllocation = pAllocation;
    pHeader = (PALLOCATION_HEADER) pAllocation - 1;

    // Set the size
    *pSize = pHeader->size;

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Un-Maps the allocation handle. In this implementation it doesn't do anything
 */
DEFINE_HEAP_UNMAP(sysHeapUnmap)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    // Delegate the call directly
    CHK_STATUS(commonHeapUnmap(pHeap, pAllocation));

CleanUp:
    LEAVES();
    return retStatus;
}

DEFINE_HEADER_SIZE(sysGetAllocationHeaderSize)
{
    return SYS_ALLOCATION_HEADER_SIZE;
}

DEFINE_FOOTER_SIZE(sysGetAllocationFooterSize)
{
    return SYS_ALLOCATION_FOOTER_SIZE;
}

DEFINE_ALIGNED_SIZE(sysGetAllocationAlignedSize)
{
    return size;
}

DEFINE_ALLOC_SIZE(sysGetAllocationSize)
{
    // This is a direct allocation
    PVOID pAllocation = (PVOID) HANDLE_TO_POINTER(handle);
    PALLOCATION_HEADER pHeader = (PALLOCATION_HEADER) pAllocation - 1;

#ifdef HEAP_DEBUG
    // Check the allocation 'guard band' in debug mode
    if (0 != MEMCMP(pHeader->magic, ALLOCATION_HEADER_MAGIC, SIZEOF(ALLOCATION_HEADER_MAGIC))) {
        DLOGE("Invalid header for allocation %p", pAllocation);
        return INVALID_ALLOCATION_VALUE;
    }

    // Check the footer
    if (0 != MEMCMP((PBYTE)(pHeader + 1) + pHeader->size, &gSysFooter, SYS_ALLOCATION_FOOTER_SIZE)) {
        DLOGE("Invalid footer for allocation %p", pAllocation);
        return INVALID_ALLOCATION_VALUE;
    }

    // Check the type
    if (SYS_ALLOCATION_TYPE != pHeader->type) {
        DLOGE("Invalid allocation type 0x%08x", pHeader->type);
        return INVALID_ALLOCATION_VALUE;
    }
#endif

    return SYS_ALLOCATION_HEADER_SIZE + pHeader->size + SYS_ALLOCATION_FOOTER_SIZE;
}

DEFINE_HEAP_LIMITS(sysGetHeapLimits)
{
    *pMinHeapSize = MIN_HEAP_SIZE;
    *pMaxHeapSize = MAX_HEAP_SIZE;
}
