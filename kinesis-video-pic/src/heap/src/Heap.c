/**
 * Implementation of abstract heap
 */

#define LOG_CLASS "Heap"
#include "Include_i.h"

/**
 * Debug print analytics information
 *
 * Param:
 *      @pHeap - Heap pointer
 *      @dump - Whether to dump detailed info
 */
STATUS heapDebugCheckAllocator(PHeap pHeap, BOOL dump)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PBaseHeap pBase = (PBaseHeap) pHeap;

    CHK(pBase != NULL, STATUS_NULL_ARG);
    CHK_STATUS(pBase->heapDebugCheckAllocatorFn(pHeap, dump));

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Initialize and returns the heap object
 *
 * Param:
 *      @heapLimit - The overall size of the heap
 *      @spillRatio - Spill ratio in percentage of direct allocation RAM vs. vRAM in the hybrid heap scenario
 *      @behaviorFlags - Flags controlling the behavior/type of the heap
 *      @pRootDirectoru - Optional path to the root directory in case of the file-based heap
 *      @ppHeap - The returned pointer to the Heap object
 */
STATUS heapInitialize(UINT64 heapLimit, UINT32 spillRatio, UINT32 behaviorFlags, PCHAR pRootDirectory, PHeap* ppHeap)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PHeap pHeap = NULL;
    PHybridHeap pHybridHeap = NULL;
    PHybridFileHeap pFileHeap = NULL;
    UINT32 heapTypeFlags = (behaviorFlags & (FLAGS_USE_AIV_HEAP | FLAGS_USE_SYSTEM_HEAP));

    CHK(ppHeap != NULL, STATUS_NULL_ARG);
    CHK(heapLimit >= MIN_HEAP_SIZE, STATUS_INVALID_ARG);
    CHK(spillRatio <= 100, STATUS_INVALID_ARG);

    // Flags should have either system or AIV heap specified but not both at the same time
    CHK(heapTypeFlags != HEAP_FLAGS_NONE && heapTypeFlags != (FLAGS_USE_AIV_HEAP | FLAGS_USE_SYSTEM_HEAP), STATUS_HEAP_FLAGS_ERROR);

    DLOGI("Initializing native heap with limit size %" PRIu64 ", spill ratio %u%% and flags 0x%08x", heapLimit, spillRatio, behaviorFlags);

    // Need to dynamically decide the heap implementation
    // The logic is to check if we are allowed to use the hybrid implementation and
    // whether the system libraries are present.
    // We will fallback to AIV heap implementation otherwise
    // First, check whether we need to use system or AIV heap
    if ((behaviorFlags & FLAGS_USE_SYSTEM_HEAP) != HEAP_FLAGS_NONE) {
        DLOGI("Creating system heap.");
        CHK_STATUS(sysHeapCreate(&pHeap));
    } else {
        DLOGI("Creating AIV heap.");
        CHK_STATUS(aivHeapCreate(&pHeap));
    }

    // See if we have hybrid heap specified and if vcsm libs are present
    if ((behaviorFlags & FLAGS_USE_HYBRID_VRAM_HEAP) != HEAP_FLAGS_NONE) {
        DLOGI("Creating hybrid heap with flags: 0x%08x", behaviorFlags);
        CHK_STATUS(hybridCreateHeap(pHeap, spillRatio, behaviorFlags, &pHybridHeap));

        // Store the hybrid heap as the returned heap object
        pHeap = (PHeap) pHybridHeap;
    } else if ((behaviorFlags & FLAGS_USE_HYBRID_FILE_HEAP) != HEAP_FLAGS_NONE) {
        DLOGI("Creating hybrid file heap with flags: 0x%08x", behaviorFlags);
        CHK_STATUS(hybridFileCreateHeap(pHeap, spillRatio, pRootDirectory, &pFileHeap));

        // Store the file hybrid heap as the returned heap object
        pHeap = (PHeap) pFileHeap;
    }

    // Just in case - validate the final heap object
    CHK(pHeap != NULL, STATUS_INTERNAL_ERROR);

    // Initialize the heap
    CHK_STATUS(((PBaseHeap) pHeap)->heapInitializeFn(pHeap, heapLimit));

    DLOGI("Heap is initialized OK");

    // Set the return pointer
    *ppHeap = pHeap;

CleanUp:

    // Clean up if we failed
    if (STATUS_FAILED(retStatus)) {
        DLOGE("Failed to initialize native heap.");
        // The call is idempotent anyway
        heapRelease(pHeap);
    }

    LEAVES();
    return retStatus;
}

/**
 * Releases the underlying heap.
 * IMPORANT!!! Some heaps will not release the underlying memory allocations.
 *
 * The call is indempotent
 *
 * Param:
 *      @pHeap - The heap pointer
 */
STATUS heapRelease(PHeap pHeap)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PBaseHeap pBase = (PBaseHeap) pHeap;

    // Release is idempotent
    DLOGV("Freeing native heap.");

    if (pBase != NULL) {
        CHK_STATUS(pBase->heapReleaseFn(pHeap));
    }

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Returns the current utilization size of the heap.
 *
 * Param:
 *      @pHeap - The heap pointer
 *      @pHeapSize - Returns the size of the heap current utilization
 */
STATUS heapGetSize(PHeap pHeap, PUINT64 pHeapSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PBaseHeap pBase = (PBaseHeap) pHeap;

    CHK(pBase != NULL && pHeapSize != NULL, STATUS_NULL_ARG);

    DLOGS("Getting the size of the heap.");
    CHK_STATUS(pBase->heapGetSizeFn(pHeap, pHeapSize));

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Allocates a chunk of memory and returns the handle
 *
 * Param:
 *      @pHeap - The heap pointer
 *      @size - The size of the allocation
 *      @pHandle - The handle to the allocated memory
 */
STATUS heapAlloc(PHeap pHeap, UINT64 size, PALLOCATION_HANDLE pHandle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PBaseHeap pBase = (PBaseHeap) pHeap;

    CHK(pBase != NULL && pHandle != NULL, STATUS_NULL_ARG);
    CHK(size != 0, STATUS_INVALID_ARG);

    DLOGS("Allocating %" PRIu64 " bytes", size);
    CHK_STATUS(pBase->heapAllocFn(pHeap, size, pHandle));

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Frees the previously allocated memory handle.
 *
 * Param:
 *      @pHeap - The heap pointer
 *      @handle - The allocated memory handle
 */
STATUS heapFree(PHeap pHeap, ALLOCATION_HANDLE handle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PBaseHeap pBase = (PBaseHeap) pHeap;

    CHK(pBase != NULL, STATUS_NULL_ARG);
    CHK(IS_VALID_ALLOCATION_HANDLE(handle), STATUS_INVALID_ARG);

    DLOGS("Freeing allocation handle 0x%016" PRIx64, handle);
    CHK_STATUS(pBase->heapFreeFn(pHeap, handle));

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Gets the allocation size.
 *
 * Param:
 *      @pHeap - The heap pointer
 *      @handle - The allocated memory handle
 *      @pAllocSize - Returns the size of the allocation
 */
STATUS heapGetAllocSize(PHeap pHeap, ALLOCATION_HANDLE handle, PUINT64 pAllocSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PBaseHeap pBase = (PBaseHeap) pHeap;

    CHK(pBase != NULL && pAllocSize != NULL, STATUS_NULL_ARG);
    CHK(IS_VALID_ALLOCATION_HANDLE(handle), STATUS_INVALID_ARG);

    DLOGS("Getting allocation size for handle 0x%016" PRIx64, handle);
    CHK_STATUS(pBase->heapGetAllocSizeFn(pHeap, handle, pAllocSize));

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Sets the allocation size.
 *
 * Param:
 *      @pHeap - The heap pointer
 *      @pHandle - IN/OUT - The allocated memory handle to set and return a new handle
 *      @allocSize - The new allocation size
 */
STATUS heapSetAllocSize(PHeap pHeap, PALLOCATION_HANDLE pHandle, UINT64 allocSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PBaseHeap pBase = (PBaseHeap) pHeap;
    UINT64 existingSize;

    CHK(pBase != NULL && pHandle != NULL, STATUS_NULL_ARG);
    CHK(allocSize != 0 && IS_VALID_ALLOCATION_HANDLE(*pHandle), STATUS_INVALID_ARG);

    // Check if we need to do anything. Early return if the sizes are the same
    CHK_STATUS(pBase->heapGetAllocSizeFn(pHeap, *pHandle, &existingSize));
    CHK(existingSize != allocSize, retStatus);

    DLOGS("Setting new allocation size %\" PRIu64 \" for handle 0x%016" PRIx64, allocSize, *pHandle);
    CHK_STATUS(pBase->heapSetAllocSizeFn(pHeap, pHandle, existingSize, allocSize));

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Maps the previously allocated memory handle.
 *
 * Param:
 *      @pHeap - The heap pointer
 *      @handle - The allocated memory handle
 *      @ppAllocation - The returned memory pointer
 *      @pSize - The returned size of the allocation
 */
STATUS heapMap(PHeap pHeap, ALLOCATION_HANDLE handle, PVOID* ppAllocation, PUINT64 pSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PBaseHeap pBase = (PBaseHeap) pHeap;

    CHK(pBase != NULL && ppAllocation != NULL, STATUS_NULL_ARG);
    CHK(IS_VALID_ALLOCATION_HANDLE(handle), STATUS_INVALID_ARG);

    DLOGS("Mapping handle 0x%016" PRIx64, handle);
    CHK_STATUS(pBase->heapMapFn(pHeap, handle, ppAllocation, pSize));

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Un maps the previously mapped allocation.
 *
 * Param:
 *      @pHeap - The heap pointer
 *      @pAllocation - The mapped allocation pointer
 */
STATUS heapUnmap(PHeap pHeap, PVOID pAllocation)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PBaseHeap pBase = (PBaseHeap) pHeap;

    CHK(pBase != NULL && pAllocation != NULL, STATUS_NULL_ARG);

    DLOGS("Un-mapping buffer: %p", pAllocation);
    CHK_STATUS(pBase->heapUnmapFn(pHeap, pAllocation));

CleanUp:
    LEAVES();
    return retStatus;
}
