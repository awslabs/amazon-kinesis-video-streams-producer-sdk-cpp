/**
 * Implementation of a heap based on Hybrid heap
 */

#define LOG_CLASS "HybridHeap"
#include "Include_i.h"

#ifdef HEAP_DEBUG
ALLOCATION_HEADER gVramHeader = {0, VRAM_ALLOCATION_TYPE, 0, ALLOCATION_HEADER_MAGIC};
ALLOCATION_FOOTER gVramFooter = {1, ALLOCATION_FOOTER_MAGIC};

#define VRAM_ALLOCATION_FOOTER_SIZE SIZEOF(gVramFooter)

#else
ALLOCATION_HEADER gVramHeader = {0, VRAM_ALLOCATION_TYPE, 0};
ALLOCATION_FOOTER gVramFooter = {0};

#define VRAM_ALLOCATION_FOOTER_SIZE 0

#endif

#define VRAM_ALLOCATION_HEADER_SIZE SIZEOF(gVramHeader)

STATUS hybridCreateHeap(PHeap pHeap, UINT32 spillRatio, UINT32 behaviorFlags, PHybridHeap* ppHybridHeap)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PHybridHeap pHybridHeap = NULL;
    PBaseHeap pBaseHeap = NULL;
    PVOID handle = NULL;
    VramInit vramInit = NULL;
    VramAlloc vramAlloc = NULL;
    VramFree vramFree = NULL;
    VramLock vramLock = NULL;
    VramUnlock vramUnlock = NULL;
    VramUninit vramUninit = NULL;
    VramGetMax vramGetMax = NULL;
    BOOL reopenVramLibrary = ((behaviorFlags & FLAGS_REOPEN_VRAM_LIBRARY) != HEAP_FLAGS_NONE);

    CHK(pHeap != NULL, STATUS_NULL_ARG);
    CHK(spillRatio <= 100, STATUS_INVALID_ARG);

    // Load the library.
    // NOTE: The library will only be present on VRAM allocation capable devices
    // We will try to load the library with the name first and then with full path
    if (NULL == (handle = DLOPEN((PCHAR) VRAM_LIBRARY_NAME, RTLD_NOW | RTLD_GLOBAL)) &&
        NULL == (handle = DLOPEN((PCHAR) VRAM_LIBRARY_FULL_PATH, RTLD_NOW | RTLD_GLOBAL))) {
        CHK_ERR(FALSE, STATUS_HEAP_VRAM_LIB_MISSING, "Failed to load library %s with %s", VRAM_LIBRARY_NAME, DLERROR());
    }

    // HACK Reopening the vram library to increment the ref count, because for some unknown reason in heapRelease we
    // get a SIGSEGV on DLCLOSE as the library seems to be already closed, https://jira2.amazon.com/browse/AIVPLAYERS-5111.
    if (reopenVramLibrary && NULL == (handle = DLOPEN((PCHAR) VRAM_LIBRARY_NAME, RTLD_NOW | RTLD_GLOBAL)) &&
        NULL == (handle = DLOPEN((PCHAR) VRAM_LIBRARY_FULL_PATH, RTLD_NOW | RTLD_GLOBAL))) {
        CHK_ERR(FALSE, STATUS_HEAP_VRAM_LIB_REOPEN, "Failed to re-open library %s with %s", VRAM_LIBRARY_NAME, DLERROR());
    }

    // Load the functions and store the pointers
    CHK_ERR(NULL != (vramInit = (VramInit) DLSYM(handle, (PCHAR) VRAM_INIT_FUNC_SYMBOL_NAME)), STATUS_HEAP_VRAM_INIT_FUNC_SYMBOL,
            "Failed to load exported function %s with %s", VRAM_INIT_FUNC_SYMBOL_NAME, DLERROR());
    CHK_ERR(NULL != (vramAlloc = (VramAlloc) DLSYM(handle, (PCHAR) VRAM_ALLOC_FUNC_SYMBOL_NAME)), STATUS_HEAP_VRAM_ALLOC_FUNC_SYMBOL,
            "Failed to load exported function %s with %s", VRAM_ALLOC_FUNC_SYMBOL_NAME, DLERROR());
    CHK_ERR(NULL != (vramFree = (VramFree) DLSYM(handle, (PCHAR) VRAM_FREE_FUNC_SYMBOL_NAME)), STATUS_HEAP_VRAM_FREE_FUNC_SYMBOL,
            "Failed to load exported function %s with %s", VRAM_FREE_FUNC_SYMBOL_NAME, DLERROR());
    CHK_ERR(NULL != (vramLock = (VramLock) DLSYM(handle, (PCHAR) VRAM_LOCK_FUNC_SYMBOL_NAME)), STATUS_HEAP_VRAM_LOCK_FUNC_SYMBOL,
            "Failed to load exported function %s with %s", VRAM_LOCK_FUNC_SYMBOL_NAME, DLERROR());
    CHK_ERR(NULL != (vramUnlock = (VramUnlock) DLSYM(handle, (PCHAR) VRAM_UNLOCK_FUNC_SYMBOL_NAME)), STATUS_HEAP_VRAM_UNLOCK_FUNC_SYMBOL,
            "Failed to load exported function %s with %s", VRAM_UNLOCK_FUNC_SYMBOL_NAME, DLERROR());
    CHK_ERR(NULL != (vramUninit = (VramUninit) DLSYM(handle, (PCHAR) VRAM_UNINIT_FUNC_SYMBOL_NAME)), STATUS_HEAP_VRAM_UNINIT_FUNC_SYMBOL,
            "Failed to load exported function %s with %s", VRAM_UNINIT_FUNC_SYMBOL_NAME, DLERROR());
    CHK_ERR(NULL != (vramGetMax = (VramGetMax) DLSYM(handle, (PCHAR) VRAM_GETMAX_FUNC_SYMBOL_NAME)), STATUS_HEAP_VRAM_GETMAX_FUNC_SYMBOL,
            "Failed to load exported function %s with %s", VRAM_GETMAX_FUNC_SYMBOL_NAME, DLERROR());

    DLOGS("Creating hybrid heap with spill ratio %d", spillRatio);

    CHK_STATUS(commonHeapCreate((PHeap*) &pHybridHeap, SIZEOF(HybridHeap)));

    // Set the values
    pHybridHeap->pMemHeap = (PBaseHeap) pHeap;
    pHybridHeap->spillRatio = (DOUBLE) spillRatio / 100;
    pHybridHeap->vramInit = vramInit;
    pHybridHeap->vramAlloc = vramAlloc;
    pHybridHeap->vramFree = vramFree;
    pHybridHeap->vramLock = vramLock;
    pHybridHeap->vramUnlock = vramUnlock;
    pHybridHeap->vramUninit = vramUninit;
    pHybridHeap->vramGetMax = vramGetMax;
    pHybridHeap->libHandle = handle;
    pHybridHeap->vramInitialized = FALSE;

    // Return hybrid heap
    *ppHybridHeap = pHybridHeap;

    // Set the function pointers
    pBaseHeap = (PBaseHeap) pHybridHeap;
    pBaseHeap->heapInitializeFn = hybridHeapInit;
    pBaseHeap->heapReleaseFn = hybridHeapRelease;
    pBaseHeap->heapGetSizeFn = commonHeapGetSize; // Use the common heap functionality
    pBaseHeap->heapAllocFn = hybridHeapAlloc;
    pBaseHeap->heapFreeFn = hybridHeapFree;
    pBaseHeap->heapGetAllocSizeFn = hybridHeapGetAllocSize;
    pBaseHeap->heapSetAllocSizeFn = hybridHeapSetAllocSize;
    pBaseHeap->heapMapFn = hybridHeapMap;
    pBaseHeap->heapUnmapFn = hybridHeapUnmap;
    pBaseHeap->heapDebugCheckAllocatorFn = hybridHeapDebugCheckAllocator;
    pBaseHeap->getAllocationSizeFn = hybridGetAllocationSize;
    pBaseHeap->getAllocationHeaderSizeFn = hybridGetAllocationHeaderSize;
    pBaseHeap->getAllocationFooterSizeFn = hybridGetAllocationFooterSize;
    pBaseHeap->getAllocationAlignedSizeFn = hybridGetAllocationAlignedSize;
    pBaseHeap->getHeapLimitsFn = hybridGetHeapLimits;

CleanUp:
    if (STATUS_FAILED(retStatus)) {
        if (handle != NULL) {
            DLCLOSE(handle);
        }

        if (pHybridHeap != NULL) {
            // Ensure it doesn't get closed again
            pHybridHeap->libHandle = NULL;

            // Base heap will be released by the common heap
            pHybridHeap->pMemHeap = NULL;

            hybridHeapRelease((PHeap) pHybridHeap);
        }
    }

    LEAVES();
    return retStatus;
}

/**
 * Debug print analytics information
 */
DEFINE_HEAP_CHK(hybridHeapDebugCheckAllocator)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PHybridHeap pHybridHeap = (PHybridHeap) pHeap;

    // Try the contained heap first
    CHK_STATUS(pHybridHeap->pMemHeap->heapDebugCheckAllocatorFn((PHeap) pHybridHeap->pMemHeap, dump));

    // Delegate the call directly
    CHK_STATUS(commonHeapDebugCheckAllocator(pHeap, dump));

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Initialize the heap
 */
DEFINE_INIT_HEAP(hybridHeapInit)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PHybridHeap pHybridHeap = (PHybridHeap) pHeap;
    UINT32 ret, memHeapLimit, vramHeapLimit, maxVramSize;

    // Delegate the call directly
    CHK_STATUS(commonHeapInit(pHeap, heapLimit));

    // Calculate the in-memory and vram based heap sizes
    memHeapLimit = (UINT32)(heapLimit * pHybridHeap->spillRatio);
    vramHeapLimit = (UINT32)(heapLimit - memHeapLimit);

    // Try to see if we can allocate enough vram
    maxVramSize = pHybridHeap->vramGetMax();
    CHK_ERR(maxVramSize >= vramHeapLimit, STATUS_NOT_ENOUGH_MEMORY, "Can't reserve VRAM with size %u. Max allowed is %u bytes", vramHeapLimit,
            maxVramSize);

    // Initialize the encapsulated heap
    CHK_STATUS_ERR(pHybridHeap->pMemHeap->heapInitializeFn((PHeap) pHybridHeap->pMemHeap, memHeapLimit), STATUS_HEAP_DIRECT_MEM_INIT,
                   "Failed to initialize the in-memory heap with limit size %u", memHeapLimit);

    // Initialize the VRAM
    CHK_ERR(0 == (ret = pHybridHeap->vramInit()), STATUS_HEAP_VRAM_INIT_FAILED, "Failed to initialize the vcsm heap. Error returned %u", ret);

    pHybridHeap->vramInitialized = TRUE;

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Free the hybrid heap
 */
DEFINE_RELEASE_HEAP(hybridHeapRelease)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    STATUS memHeapStatus = STATUS_SUCCESS;
    STATUS dlCloseStatus = STATUS_SUCCESS;
    STATUS vramUninitStatus = STATUS_SUCCESS;
    PHybridHeap pHybridHeap = (PHybridHeap) pHeap;
    INT32 dlCloseRet, vramUninitRet;

    // The call should be idempotent
    CHK(pHeap != NULL, STATUS_SUCCESS);

    // Regardless of the status (heap might be corrupted) we still want to free the memory
    retStatus = commonHeapRelease(pHeap);

    // Release the direct memory heap
    if (pHybridHeap->pMemHeap != NULL && STATUS_SUCCESS != (memHeapStatus = pHybridHeap->pMemHeap->heapReleaseFn((PHeap) pHybridHeap->pMemHeap))) {
        DLOGW("Failed to release in-memory heap with 0x%08x", memHeapStatus);
    }

    // Release the vcsm heap
    if (pHybridHeap->vramInitialized && (0 != (vramUninitRet = pHybridHeap->vramUninit()))) {
        vramUninitStatus = STATUS_HEAP_VRAM_UNINIT_FAILED;
        DLOGW("Failed to uninitialize the vram library with %d", vramUninitRet);
    }

    // Close the library handle
    if (pHybridHeap->libHandle != NULL && (0 != (dlCloseRet = DLCLOSE(pHybridHeap->libHandle)))) {
        dlCloseStatus = STATUS_HEAP_LIBRARY_FREE_FAILED;
        DLOGW("Failed to close the library with %d", dlCloseRet);
    }

    // Free the allocation itself
    MEMFREE(pHeap);

CleanUp:
    LEAVES();
    // return the combination. This works as STATUS_SUCCESS is defined as 0
    return retStatus | memHeapStatus | dlCloseStatus | vramUninitStatus;
}

/**
 * Allocate from the heap
 *
 * IMPORTANT: If we fail to allocate from the in-memory heap (most likely due to oom condition)
 * we fall back and try to allocate from VRAM. We need to later make sure we know whether
 * the allocated handle came from the in-memory heap or from the VRAM heap. In order to
 * differentiate the allocation we assume that if the handle value is less than 8 byte
 * aligned then it's a VRAM allocation. VRAM uses the high-order 32 bits to store the
 * actual value of the handle and use the low bits to indicate that it's a VRAM allocation.
 * All in-memory allocators will allocate at least 8-byte aligned memory.
 */
DEFINE_HEAP_ALLOC(hybridHeapAlloc)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PHybridHeap pHybridHeap = (PHybridHeap) pHeap;
    PALLOCATION_HEADER pHeader = NULL;

    // overall allocation size
    UINT64 allocationSize = size + VRAM_ALLOCATION_HEADER_SIZE + VRAM_ALLOCATION_FOOTER_SIZE;
    PVOID pAlloc = NULL;
    UINT32 handle = 0;
    ALLOCATION_HANDLE retHandle;

    // Call the base class for the accounting
    retStatus = commonHeapAlloc(pHeap, size, pHandle);
    CHK(retStatus == STATUS_NOT_ENOUGH_MEMORY || retStatus == STATUS_SUCCESS, retStatus);
    if (retStatus == STATUS_NOT_ENOUGH_MEMORY) {
        // If we are out of memory then we don't need to return a failure - just
        // Early return with success
        CHK(FALSE, STATUS_SUCCESS);
    }

    DLOGS("Trying to allocate from direct memory heap.");
    // Try to allocate from the memory first. pHandle is not NULL if we have passed the base checks
    // Check for the pHandle not being null for successful allocation
    CHK_STATUS(pHybridHeap->pMemHeap->heapAllocFn((PHeap) pHybridHeap->pMemHeap, size, pHandle));

    // See if we succeeded allocating the buffer
    if (*pHandle != INVALID_ALLOCATION_HANDLE_VALUE) {
        // We have successfully allocated memory from the in-memory heap
        DLOGS("Successfully allocated from direct memory.");

        // Early exit with success
        CHK(FALSE, STATUS_SUCCESS);
    }

    DLOGS("Allocating from VRAM");

    // Now, we are going to try to allocate from VRAM
    // We are not going to fail here as 0 is going to indicate a no-alloc
    // Need to map and add the metadata
    // Validate that the allocation is not greater than 32 bit max
    CHK_ERR(allocationSize < MAX_UINT32, STATUS_HEAP_VRAM_ALLOC_FAILED, "Can not allocate more than 4G from VRAM");
    CHK_ERR(INVALID_VRAM_HANDLE != (handle = pHybridHeap->vramAlloc((UINT32) allocationSize)), STATUS_HEAP_VRAM_ALLOC_FAILED,
            "Failed to allocate %u bytes from VRAM", allocationSize);

    // Map the allocation
    if (NULL == (pAlloc = pHybridHeap->vramLock(handle))) {
        DLOGE("Failed to map the VRAM handle %08x", handle);

        // Adjust the heap usage
        decrementUsage(pHeap, allocationSize);

        CHK(FALSE, STATUS_NOT_ENOUGH_MEMORY);
    }

    // Guard-band the allocation
#ifdef HEAP_DEBUG
    // Null the memory in debug mode
    MEMSET(pAlloc, 0x00, (UINT32) allocationSize);
#endif
    // Set up the header and footer
    pHeader = (PALLOCATION_HEADER) pAlloc;
    MEMCPY(pHeader, &gVramHeader, VRAM_ALLOCATION_HEADER_SIZE);
    MEMCPY((PBYTE) pHeader + VRAM_ALLOCATION_HEADER_SIZE + size, &gVramFooter, VRAM_ALLOCATION_FOOTER_SIZE);

    // Fix-up the allocation size
    pHeader->size = size;

    // Store the handle
    pHeader->vramHandle = handle;

    // Un-map the range
    if (0 != pHybridHeap->vramUnlock(handle)) {
        // We shouldn't really fail here
        DLOGW("Failed to unmap 0x%08x", handle);
    }

    // Need to convert the VRAM handle
    retHandle = FROM_VRAM_HANDLE(handle);
    DLOGS("Allocated VRAM handle 0x%016" PRIx64, retHandle);
    *pHandle = retHandle;

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Free the allocation
 */
DEFINE_HEAP_FREE(hybridHeapFree)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PHybridHeap pHybridHeap = (PHybridHeap) pHeap;
    UINT32 vramHandle;
    UINT32 ret;

    // Calling the base first - this should do the accounting
    CHK_STATUS(commonHeapFree(pHeap, handle));

    // If this is a direct allocation then we handle that separately
    if (IS_DIRECT_ALLOCATION_HANDLE(handle)) {
        DLOGS("Direct allocation");
        CHK_STATUS(pHybridHeap->pMemHeap->heapFreeFn((PHeap) pHybridHeap->pMemHeap, handle));

        // Exit on success
        CHK(FALSE, STATUS_SUCCESS);
    }

    // In case it's a direct allocation handle use the encapsulated direct memory heap
    DLOGS("Indirect allocation");
    // Convert the handle
    vramHandle = TO_VRAM_HANDLE(handle);
    CHK_ERR(0 == (ret = pHybridHeap->vramFree(vramHandle)), STATUS_HEAP_VRAM_FREE_FAILED, "Failed to free VRAM handle %08x with %lu", vramHandle,
            ret);

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Gets the allocation size
 */
DEFINE_HEAP_GET_ALLOC_SIZE(hybridHeapGetAllocSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PHybridHeap pHybridHeap = (PHybridHeap) pHeap;
    PALLOCATION_HEADER pHeader;
    UINT32 vramHandle;

    // Call the base class to ensure the params are ok and set the default ret values
    CHK_STATUS(commonHeapGetAllocSize(pHeap, handle, pAllocSize));

    // In case it's a direct allocation handle use the encapsulated direct memory heap
    if (IS_DIRECT_ALLOCATION_HANDLE(handle)) {
        DLOGS("Direct allocation 0x%016" PRIx64, handle);
        CHK_STATUS(pHybridHeap->pMemHeap->heapGetAllocSizeFn((PHeap) pHybridHeap->pMemHeap, handle, pAllocSize));

        // Exit on success
        CHK(FALSE, STATUS_SUCCESS);
    }

    // Convert the handle
    vramHandle = TO_VRAM_HANDLE(handle);
    DLOGS("VRAM allocation. Handle 0x%016" PRIx64 " VRAM handle 0x%08x", handle, vramHandle);

    CHK_ERR(NULL != (pHeader = (PALLOCATION_HEADER) pHybridHeap->vramLock(vramHandle)), STATUS_HEAP_VRAM_MAP_FAILED, "Failed to map VRAM handle %08x",
            vramHandle);

    // Set the values and return
    *pAllocSize = pHeader->size;

    // Un-lock the allocation
    if (0 != pHybridHeap->vramUnlock(vramHandle)) {
        // Shouldn't fail here
        DLOGW("Failed to unmap 0x%08x", vramHandle);
    }

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Sets the allocation size
 */
DEFINE_HEAP_SET_ALLOC_SIZE(hybridHeapSetAllocSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PHybridHeap pHybridHeap = (PHybridHeap) pHeap;
    ALLOCATION_HANDLE existingAllocationHandle, newAllocationHandle = INVALID_ALLOCATION_HANDLE_VALUE;
    PVOID pExistingBuffer = NULL, pNewBuffer = NULL;

    // Call the base class to ensure the params are ok and set the default ret values
    CHK_STATUS(commonHeapSetAllocSize(pHeap, pHandle, size, newSize));

    existingAllocationHandle = *pHandle;
    // In case it's a direct allocation handle use the encapsulated direct memory heap
    if (IS_DIRECT_ALLOCATION_HANDLE(existingAllocationHandle)) {
        DLOGS("Direct allocation 0x%016" PRIx64, existingAllocationHandle);
        retStatus = pHybridHeap->pMemHeap->heapSetAllocSizeFn((PHeap) pHybridHeap->pMemHeap, pHandle, size, newSize);
        CHK(retStatus == STATUS_SUCCESS || retStatus == STATUS_HEAP_REALLOC_ERROR, retStatus);

        // Exit on success
        CHK(!STATUS_SUCCEEDED(retStatus), STATUS_SUCCESS);

        // Reset the status
        retStatus = STATUS_SUCCESS;
    }

    // Perform the VRAM slow re-allocation
    // 1) Allocating new buffer
    // 2) Mapping the existing allocation
    // 3) Mapping the new allocation
    // 4) Copying the data from old to new
    // 5) Unmapping the existing allocation
    // 6) Unmapping the new allocation
    // 7) Free-ing the old allocation

    DLOGS("Sets new allocation size %\" PRIu64 \" for handle 0x%016" PRIx64, newSize, existingAllocationHandle);

    CHK_STATUS(hybridHeapAlloc(pHeap, newSize, &newAllocationHandle));
    CHK(IS_VALID_ALLOCATION_HANDLE(newAllocationHandle), STATUS_NOT_ENOUGH_MEMORY);
    CHK_STATUS(hybridHeapMap(pHeap, existingAllocationHandle, &pExistingBuffer, &size));
    CHK_STATUS(hybridHeapMap(pHeap, newAllocationHandle, &pNewBuffer, &newSize));
    MEMCPY(pNewBuffer, pExistingBuffer, MIN(size, newSize));
    CHK_STATUS(hybridHeapUnmap(pHeap, pExistingBuffer));
    CHK_STATUS(hybridHeapUnmap(pHeap, pNewBuffer));
    CHK_STATUS(hybridHeapFree(pHeap, existingAllocationHandle));

    // Set the return
    *pHandle = newAllocationHandle;

CleanUp:

    // Clean-up in case of failure
    if (STATUS_FAILED(retStatus) && !IS_VALID_ALLOCATION_HANDLE(newAllocationHandle)) {
        // Free the allocation before returning
        hybridHeapFree(pHeap, newAllocationHandle);
    }

    LEAVES();
    return retStatus;
}

/**
 * Map the allocation.
 * IMPORTANT: We will determine whether this is direct allocation by checking the last 2 bits being 0.
 * This works because all our allocators are at least 8 byte aligned so the handle will have it's
 * least significant two bits set as 0. The VRAM handle will have it set to 1s.
 */
DEFINE_HEAP_MAP(hybridHeapMap)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PHybridHeap pHybridHeap = (PHybridHeap) pHeap;
    PALLOCATION_HEADER pHeader;
    UINT32 vramHandle;

    // Call the base class to ensure the params are ok and set the default ret values
    CHK_STATUS(commonHeapMap(pHeap, handle, ppAllocation, pSize));

    // In case it's a direct allocation handle use the encapsulated direct memory heap
    if (IS_DIRECT_ALLOCATION_HANDLE(handle)) {
        DLOGS("Direct allocation 0x%016" PRIx64, handle);
        CHK_STATUS(pHybridHeap->pMemHeap->heapMapFn((PHeap) pHybridHeap->pMemHeap, handle, ppAllocation, pSize));

        // Exit on success
        CHK(FALSE, STATUS_SUCCESS);
    }

    // Convert the handle
    vramHandle = TO_VRAM_HANDLE(handle);
    DLOGS("VRAM allocation. Handle 0x%016" PRIx64 " VRAM handle 0x%08x", handle, vramHandle);

    CHK_ERR(NULL != (pHeader = (PALLOCATION_HEADER) pHybridHeap->vramLock(vramHandle)), STATUS_HEAP_VRAM_MAP_FAILED, "Failed to map VRAM handle %08x",
            vramHandle);

    // Set the values and return
    *ppAllocation = pHeader + 1;
    *pSize = pHeader->size;

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Un-Maps the allocation handle.
 */
DEFINE_HEAP_UNMAP(hybridHeapUnmap)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PHybridHeap pHybridHeap = (PHybridHeap) pHeap;
    PALLOCATION_HEADER pHeader = (PALLOCATION_HEADER) pAllocation - 1;
    UINT32 ret;

    // Call the base class to ensure the params are ok
    CHK_STATUS(commonHeapUnmap(pHeap, pAllocation));

    // Check if this is a direct allocation by examining the type
    if (VRAM_ALLOCATION_TYPE != pHeader->type) {
        DLOGS("Direct allocation");

        // This is a direct allocation - call the encapsulated heap
        CHK_STATUS(pHybridHeap->pMemHeap->heapUnmapFn((PHeap) pHybridHeap->pMemHeap, pAllocation));

        // Exit on success
        CHK(FALSE, STATUS_SUCCESS);
    }

    DLOGS("Indirect allocation");
    // Un-map from the vram
    CHK_ERR(0 == (ret = pHybridHeap->vramUnlock(pHeader->vramHandle)), STATUS_HEAP_VRAM_UNMAP_FAILED,
            "Failed to un-map handle 0x%08x. Error returned %u", pHeader->vramHandle, ret);

CleanUp:
    LEAVES();
    return retStatus;
}

DEFINE_HEADER_SIZE(hybridGetAllocationHeaderSize)
{
    return VRAM_ALLOCATION_HEADER_SIZE;
}

DEFINE_FOOTER_SIZE(hybridGetAllocationFooterSize)
{
    return VRAM_ALLOCATION_FOOTER_SIZE;
}

DEFINE_ALIGNED_SIZE(hybridGetAllocationAlignedSize)
{
    return size;
}

DEFINE_ALLOC_SIZE(hybridGetAllocationSize)
{
    PHybridHeap pHybridHeap = (PHybridHeap) pHeap;
    UINT32 vramHandle;
    PALLOCATION_HEADER pAllocation;
    UINT64 memSizes, vramSizes, memHeapAllocationSize;

    CHECK_EXT(pHeap != NULL, "Internal error with VRAM heap being null");

    // Check if this is a direct allocation
    if (IS_DIRECT_ALLOCATION_HANDLE(handle)) {
        // Get the allocation header and footer in order to compensate the accounting for vram header and footer.
        memSizes = pHybridHeap->pMemHeap->getAllocationHeaderSizeFn() + pHybridHeap->pMemHeap->getAllocationFooterSizeFn();
        vramSizes = hybridGetAllocationHeaderSize() + hybridGetAllocationFooterSize();
        memHeapAllocationSize = pHybridHeap->pMemHeap->getAllocationSizeFn((PHeap) pHybridHeap->pMemHeap, handle);
        return memHeapAllocationSize - memSizes + vramSizes;
    }

    // In case of VRAM allocation we need to map the memory first to access the size info
    vramHandle = TO_VRAM_HANDLE(handle);

    // Map the allocation
    if (NULL == (pAllocation = (PALLOCATION_HEADER) pHybridHeap->vramLock(vramHandle))) {
        // This is the failure sentinel
        DLOGE("Failed to map VRAM handle 0x%08x", vramHandle);
        return INVALID_ALLOCATION_VALUE;
    }

#ifdef HEAP_DEBUG
    // Check the allocation 'guard band' in debug mode
    if (0 != MEMCMP(pAllocation->magic, ALLOCATION_HEADER_MAGIC, SIZEOF(ALLOCATION_HEADER_MAGIC))) {
        DLOGE("Invalid header for allocation %p", pAllocation);
        return INVALID_ALLOCATION_VALUE;
    }

    // Check the footer
    if (0 != MEMCMP((PBYTE)(pAllocation + 1) + pAllocation->size, &gVramFooter, VRAM_ALLOCATION_FOOTER_SIZE)) {
        DLOGE("Invalid footer for allocation %p", pAllocation);
        return INVALID_ALLOCATION_VALUE;
    }

    // Check the type
    if (VRAM_ALLOCATION_TYPE != pAllocation->type) {
        DLOGE("Invalid allocation type 0x%08x", pAllocation->type);
        return INVALID_ALLOCATION_VALUE;
    }
#endif

    // Un-map the range
    if (0 != pHybridHeap->vramUnlock(vramHandle)) {
        // Shouldn't fail here
        DLOGW("Failed to unmap 0x%08x", vramHandle);
    }

    return VRAM_ALLOCATION_HEADER_SIZE + pAllocation->size + VRAM_ALLOCATION_FOOTER_SIZE;
}

DEFINE_HEAP_LIMITS(hybridGetHeapLimits)
{
    *pMinHeapSize = MIN_HEAP_SIZE;
    *pMaxHeapSize = MAX_HEAP_SIZE;
}
