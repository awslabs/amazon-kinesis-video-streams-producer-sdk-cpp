/**
 * Implementation of common heap functionality
 */

#define LOG_CLASS "CommonHeap"
#include "Include_i.h"

/**
 * Validates the heap. This simply evaluates to a call to an outer function if we are in debug mode
 */
STATUS validateHeap(PHeap pHeap)
{
#ifdef HEAP_DEBUG
    return heapDebugCheckAllocator(pHeap, FALSE);
#else
    return STATUS_SUCCESS;
#endif

}

/**
 * Debug print analytics information
 */
DEFINE_HEAP_CHK(commonHeapDebugCheckAllocator)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pHeap != NULL, STATUS_NULL_ARG);

    if (dump) {
        DLOGI("Heap is %sinitialized", pHeap->heapLimit == 0 ? "not " : "");
        DLOGI("Heap limit: \t\t\t\t%" PRIu64, pHeap->heapLimit);
        DLOGI("Heap size: \t\t\t\t%" PRIu64, pHeap->heapSize);
        DLOGI("Number of allocations: \t\t\t\t%" PRIu64, pHeap->numAlloc);
    }

    CHK(pHeap->heapLimit >= pHeap->heapSize, STATUS_INTERNAL_ERROR);

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Creates the heap object
 */
STATUS commonHeapCreate(PHeap* ppHeap, UINT32 objectSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    CHK(ppHeap != NULL && objectSize != 0, STATUS_INVALID_ARG);
    *ppHeap = (PHeap) MEMCALLOC(1, objectSize);
    CHK(*ppHeap != NULL, STATUS_NOT_ENOUGH_MEMORY);

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Initialize the heap
 */
DEFINE_INIT_HEAP(commonHeapInit)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 minHeapSize, maxHeapSize;
    PBaseHeap pBaseHeap = (PBaseHeap) pHeap;

    DLOGS("Calling heapInit with limit %" PRIu64, heapLimit);
    // Check the input params
    CHK(pHeap != NULL, STATUS_NULL_ARG);
    pBaseHeap->getHeapLimitsFn(&minHeapSize, &maxHeapSize);
    CHK_ERR(heapLimit >= minHeapSize && heapLimit <= maxHeapSize, STATUS_INVALID_ARG, "Invalid heap limit size %" PRIu64, heapLimit);

    // store the heap limit
    pHeap->heapLimit = heapLimit;

    // zero the current size though it must have been 0 already
    pHeap->heapSize = 0;

    // Nothing has been allocated yet
    pHeap->numAlloc = 0;

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Release the heap
 */
DEFINE_RELEASE_HEAP(commonHeapRelease)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PBaseHeap pBaseHeap = (PBaseHeap) pHeap;

    CHK(pBaseHeap != NULL, STATUS_NULL_ARG);

#ifdef HEAP_DEBUG
    CHK_STATUS(validateHeap(pHeap));
    if (pHeap->numAlloc != 0) {
        DLOGE("The heap is being released with %llu allocations amounting to %llu bytes - possible memory leak",
            pHeap->numAlloc, pHeap->heapSize);
        // We don't want to crash the app - just warn in the log
    }
#endif

    // Zero the values
    pHeap->heapLimit = pHeap->heapSize = pHeap->numAlloc = 0;

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Gets the heap size
 */
DEFINE_GET_HEAP_SIZE(commonHeapGetSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    DLOGS("Getting the heap size");

    // Validate the arguments
    CHK(pHeap != NULL && pHeapSize != NULL, STATUS_NULL_ARG);

    // Check if we are initialized by looking at heap limit
    CHK_ERR(pHeap->heapLimit != 0, STATUS_HEAP_NOT_INITIALIZED, "Heap has not been initialized.");

    *pHeapSize = pHeap->heapSize;

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Gets the allocation size
 */
DEFINE_HEAP_GET_ALLOC_SIZE(commonHeapGetAllocSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    DLOGS("Getting the allocation size for handle 0x%016" PRIx64, handle);

    // Validate the arguments
    CHK(pHeap != NULL && pAllocSize != NULL, STATUS_NULL_ARG);
    CHK(handle != INVALID_ALLOCATION_HANDLE_VALUE, STATUS_INVALID_ARG);

    // Check if we are initialized by looking at heap limit
    CHK_ERR(pHeap->heapLimit != 0, STATUS_HEAP_NOT_INITIALIZED, "Heap has not been initialized.");

    // Validate the heap
    CHK_STATUS(validateHeap(pHeap));

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Allocate from the heap
 */
DEFINE_HEAP_ALLOC(commonHeapAlloc)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 overallSize = 0;
    PBaseHeap pBaseHeap = (PBaseHeap) pHeap;
    DLOGS("Trying to allocate %d bytes", size);

    // Check the input param
    CHK(pHandle != NULL && pHeap != NULL, STATUS_NULL_ARG);

    // Set the pointer to NULL first of all
    *pHandle = INVALID_ALLOCATION_HANDLE_VALUE;

    CHK_ERR(size != 0, STATUS_INVALID_ARG, "Cannot allocate 0 amount of bytes");

    // Check if we are initialized by looking at heap limit
    CHK_ERR(pHeap->heapLimit != 0, STATUS_HEAP_NOT_INITIALIZED, "Heap has not been initialized.");

    // Check if we can allocate the chunk taking into account the allocation header and footer
    overallSize = pBaseHeap->getAllocationHeaderSizeFn() + size + pBaseHeap->getAllocationFooterSizeFn();
    CHK_ERR(overallSize + pHeap->heapSize <= pHeap->heapLimit, STATUS_NOT_ENOUGH_MEMORY,
            "Allocating %d bytes failed due to heap limit", size);

    // Validate the heap
    CHK_STATUS(validateHeap(pHeap));

    // Increment the current allocations size
    incrementUsage(pHeap, overallSize);

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Free the allocation
 */
DEFINE_HEAP_FREE(commonHeapFree)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 overallSize = 0;
    PBaseHeap pBaseHeap = (PBaseHeap) pHeap;
    DLOGS("Freeing allocation handle 0x%016" PRIx64, handle);

    // Validate the arguments
    CHK(pHeap != NULL, STATUS_NULL_ARG);
    CHK(handle != INVALID_ALLOCATION_HANDLE_VALUE, STATUS_INVALID_ARG);

    // Check if we are initialized by looking at heap limit
    CHK_ERR(pHeap->heapLimit != 0, STATUS_HEAP_NOT_INITIALIZED, "Heap has not been initialized.");

    overallSize = pBaseHeap->getAllocationSizeFn(pHeap, handle);

    CHK_ERR(overallSize != INVALID_ALLOCATION_VALUE && pHeap->heapSize - overallSize >= 0,
        STATUS_HEAP_CORRUPTED,
        "Invalid allocation or heap corruption trying to free handle 0x%016" PRIx64,
        handle);

    // Validate the heap
    CHK_STATUS(validateHeap(pHeap));

    // Decrement the heap allocations size
    decrementUsage(pHeap, overallSize);

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Map the allocation
 */
DEFINE_HEAP_MAP(commonHeapMap)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    DLOGS("Mapping handle 0x%016" PRIx64, handle);

    // Check the input params
    CHK(pHeap != NULL && ppAllocation != NULL && pSize != NULL, STATUS_NULL_ARG);
    CHK(handle != INVALID_ALLOCATION_HANDLE_VALUE, STATUS_INVALID_ARG);

    // Set the default values
    *ppAllocation = NULL;
    *pSize = 0;

    // Check if we are initialized by looking at heap limit
    CHK_ERR(pHeap->heapLimit != 0, STATUS_HEAP_NOT_INITIALIZED, "Heap has not been initialized.");

    // Validate the heap
    CHK_STATUS(validateHeap(pHeap));

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Un-Maps the allocation handle.
 */
DEFINE_HEAP_UNMAP(commonHeapUnmap)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    DLOGS("Un-mapping buffer at %p", pAllocation);

    // Check the input param
    CHK(pHeap != NULL && pAllocation != NULL, STATUS_NULL_ARG);

    // Check if we are initialized by looking at heap limit
    CHK_ERR(pHeap->heapLimit != 0, STATUS_HEAP_NOT_INITIALIZED, "Heap has not been initialized.");

    // Validate the heap
    CHK_STATUS(validateHeap(pHeap));

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Increments the heap usage
 */
VOID incrementUsage(PHeap pHeap, UINT32 overallSize) {
    pHeap->heapSize += overallSize;
    pHeap->numAlloc++;
}

/**
 * Decrements the heap usage
 */
VOID decrementUsage(PHeap pHeap, UINT32 overallSize) {
    pHeap->heapSize -= overallSize;
    pHeap->numAlloc--;
}

/**
 * Prints the content of the memory
 */
VOID printMemory(PVOID pMem, UINT32 size)
{
    DLOGE("============================================");
    DLOGE("Dumping memory: %p, size: %d", pMem, size);
    DLOGE("++++++++++++++++++++++++++++++++++++++++++++");
    CHAR buf[256];
    PCHAR pCur;
    pCur = buf;
    PBYTE pByte = (PBYTE) pMem;
    for(UINT32 i = 0; i < size; i++) {
        sprintf(pCur, "%02x ", *pByte++);
        pCur += 3;
        if ((i + 1) % 16 == 0) {
            DLOGE("%s", buf);
            buf[0] = 0;
            pCur = buf;
        }
    }
    DLOGE("++++++++++++++++++++++++++++++++++++++++++++");
    DLOGE("Dumping memory done!");
    DLOGE("============================================");
}
