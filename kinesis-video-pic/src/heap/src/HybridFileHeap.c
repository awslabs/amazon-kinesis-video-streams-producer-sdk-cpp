/**
 * Implementation of a heap based on Hybrid file-based heap
 */

#define LOG_CLASS "HybridFileHeap"
#include "Include_i.h"

#ifdef HEAP_DEBUG
ALLOCATION_HEADER gFileHeader = {0, FILE_ALLOCATION_TYPE, 0, ALLOCATION_HEADER_MAGIC};
#else
ALLOCATION_HEADER gFileHeader = {0, FILE_ALLOCATION_TYPE, 0};
#define FILE_ALLOCATION_FOOTER_SIZE 0
#endif

// No footer for the file allocations
ALLOCATION_FOOTER gFileFooter = {0};

#define FILE_ALLOCATION_HEADER_SIZE SIZEOF(gFileHeader)

STATUS hybridFileCreateHeap(PHeap pHeap, UINT32 spillRatio, PCHAR pRootDirectory, PHybridFileHeap* ppHybridHeap)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PHybridFileHeap pHybridHeap = NULL;
    PBaseHeap pBaseHeap = NULL;
    BOOL exists;

    CHK(pHeap != NULL, STATUS_NULL_ARG);
    CHK(spillRatio <= 100, STATUS_INVALID_ARG);

    DLOGS("Creating hybrid file heap with spill ratio %d", spillRatio);

    CHK_STATUS(commonHeapCreate((PHeap*) &pHybridHeap, SIZEOF(HybridFileHeap)));

    // Set the values
    pHybridHeap->pMemHeap = (PBaseHeap) pHeap;
    pHybridHeap->spillRatio = (DOUBLE) spillRatio / 100;

    // IMPORTANT: We should start from a non-zero handle num to avoid a collision with the invalid handle value
    pHybridHeap->handleNum = FILE_HEAP_STARTING_FILE_INDEX;

    // Set the root path. Use default if not specified
    if (pRootDirectory == NULL || pRootDirectory[0] == '\0') {
        MEMCPY(pHybridHeap->rootDirectory, FILE_HEAP_DEFAULT_ROOT_DIRECTORY, SIZEOF(FILE_HEAP_DEFAULT_ROOT_DIRECTORY));
    } else {
        CHK(STRNLEN(pRootDirectory, MAX_PATH_LEN + 1) <= FILE_HEAP_MAX_ROOT_DIR_LEN, STATUS_INVALID_ARG_LEN);
        STRCPY(pHybridHeap->rootDirectory, pRootDirectory);
    }

    // Check if the directory exists
    CHK_STATUS(fileExists(pHybridHeap->rootDirectory, &exists));
    CHK(exists, STATUS_NOT_FOUND);

    // Return hybrid heap
    *ppHybridHeap = pHybridHeap;

    // Set the function pointers
    pBaseHeap = (PBaseHeap) pHybridHeap;
    pBaseHeap->heapInitializeFn = hybridFileHeapInit;
    pBaseHeap->heapReleaseFn = hybridFileHeapRelease;
    pBaseHeap->heapGetSizeFn = commonHeapGetSize; // Use the common heap functionality
    pBaseHeap->heapAllocFn = hybridFileHeapAlloc;
    pBaseHeap->heapFreeFn = hybridFileHeapFree;
    pBaseHeap->heapGetAllocSizeFn = hybridFileHeapGetAllocSize;
    pBaseHeap->heapSetAllocSizeFn = hybridFileHeapSetAllocSize;
    pBaseHeap->heapMapFn = hybridFileHeapMap;
    pBaseHeap->heapUnmapFn = hybridFileHeapUnmap;
    pBaseHeap->heapDebugCheckAllocatorFn = hybridFileHeapDebugCheckAllocator;
    pBaseHeap->getAllocationSizeFn = hybridFileGetAllocationSize;
    pBaseHeap->getAllocationHeaderSizeFn = hybridFileGetAllocationHeaderSize;
    pBaseHeap->getAllocationFooterSizeFn = hybridFileGetAllocationFooterSize;
    pBaseHeap->getAllocationAlignedSizeFn = hybridFileGetAllocationAlignedSize;
    pBaseHeap->getHeapLimitsFn = hybridFileGetHeapLimits;

CleanUp:

    if (STATUS_FAILED(retStatus) && pHybridHeap != NULL) {
        // Set the base heap to NULL to avoid releasing again in the caller common function
        pHybridHeap->pMemHeap = NULL;

        hybridFileHeapRelease((PHeap) pHybridHeap);
    }

    LEAVES();
    return retStatus;
}

/**
 * Debug print analytics information
 */
DEFINE_HEAP_CHK(hybridFileHeapDebugCheckAllocator)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PHybridFileHeap pHybridHeap = (PHybridFileHeap) pHeap;

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
DEFINE_INIT_HEAP(hybridFileHeapInit)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PHybridFileHeap pHybridHeap = (PHybridFileHeap) pHeap;
    UINT64 memHeapLimit, fileHeapLimit;

    // Calling base "class" functionality first
    CHK_STATUS(commonHeapInit(pHeap, heapLimit));

    // Calculate the in-memory and file based heap sizes
    memHeapLimit = (UINT64)(heapLimit * pHybridHeap->spillRatio);
    fileHeapLimit = heapLimit - memHeapLimit;

    CHK_ERR(fileHeapLimit < MAX_LARGE_HEAP_SIZE, STATUS_NOT_ENOUGH_MEMORY,
            "Can't reserve File heap with size %" PRIu64 ". Max allowed is %" PRIu64 " bytes", fileHeapLimit, MAX_LARGE_HEAP_SIZE);

    // Initialize the encapsulated heap
    CHK_STATUS_ERR(pHybridHeap->pMemHeap->heapInitializeFn((PHeap) pHybridHeap->pMemHeap, memHeapLimit), STATUS_HEAP_DIRECT_MEM_INIT,
                   "Failed to initialize the in-memory heap with limit size %" PRIu64, memHeapLimit);

    // Initialize the file hybrid heap

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Free the hybrid heap
 */
DEFINE_RELEASE_HEAP(hybridFileHeapRelease)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    STATUS memHeapStatus = STATUS_SUCCESS;
    STATUS hybridHeapStatus = STATUS_SUCCESS;
    PHybridFileHeap pHybridHeap = (PHybridFileHeap) pHeap;

    // The call should be idempotent
    CHK(pHeap != NULL, STATUS_SUCCESS);

    // Regardless of the status (heap might be corrupted) we still want to free the memory
    retStatus = commonHeapRelease(pHeap);

    // Release the direct memory heap
    if (pHybridHeap->pMemHeap != NULL && STATUS_FAILED(memHeapStatus = pHybridHeap->pMemHeap->heapReleaseFn((PHeap) pHybridHeap->pMemHeap))) {
        DLOGW("Failed to release in-memory heap with 0x%08x", memHeapStatus);
    }

    // Remove all of the lingering files if any
    if (STATUS_FAILED(hybridHeapStatus = traverseDirectory(pHybridHeap->rootDirectory, (UINT64) pHybridHeap, FALSE, removeHeapFile))) {
        DLOGW("Failed to clear file heap remaining files with 0x%08x", hybridHeapStatus);
    }

    // Free the allocation itself
    MEMFREE(pHeap);

CleanUp:
    LEAVES();
    return retStatus | memHeapStatus | hybridHeapStatus;
}

/**
 * Allocate from the heap
 */
DEFINE_HEAP_ALLOC(hybridFileHeapAlloc)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PHybridFileHeap pHybridHeap = (PHybridFileHeap) pHeap;

    // overall allocation size
    UINT64 allocationSize = size + FILE_ALLOCATION_HEADER_SIZE + FILE_ALLOCATION_FOOTER_SIZE;
    CHAR filePath[MAX_PATH_LEN + 1];
    ALLOCATION_HEADER allocationHeader;
    ALLOCATION_HANDLE handle;

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

    DLOGS("Allocating from File heap");

    // Try to allocate from file storage
    SPRINTF(filePath, "%s%c%u" FILE_HEAP_FILE_EXTENSION, pHybridHeap->rootDirectory, FPATHSEPARATOR, pHybridHeap->handleNum);

    // Create a file with the overall size
    CHK_STATUS(createFile(filePath, allocationSize));

    // Return the file handle number and increment it
    handle = FROM_FILE_HANDLE(pHybridHeap->handleNum);

    // map, write, unmap
    // Set the header - no footer for file heap
    allocationHeader = gFileHeader;
    allocationHeader.size = size;
    allocationHeader.fileHandle = pHybridHeap->handleNum;
    CHK_STATUS(updateFile(filePath, TRUE, (PBYTE) &allocationHeader, 0, FILE_ALLOCATION_HEADER_SIZE));

    // Setting the return handle
    *pHandle = handle;

    // Increment the handle number
    pHybridHeap->handleNum++;

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Free the allocation
 */
DEFINE_HEAP_FREE(hybridFileHeapFree)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    CHAR filePath[MAX_PATH_LEN + 1];
    UINT32 fileHandle;
    INT32 retCode;
    PHybridFileHeap pHybridHeap = (PHybridFileHeap) pHeap;

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
    // Convert the handle and create the file path
    fileHandle = TO_FILE_HANDLE(handle);
    SPRINTF(filePath, "%s%c%u" FILE_HEAP_FILE_EXTENSION, pHybridHeap->rootDirectory, FPATHSEPARATOR, fileHandle);

    retCode = FREMOVE(filePath);

    if (retCode != 0) {
        switch (errno) {
            case EACCES:
                retStatus = STATUS_DIRECTORY_ACCESS_DENIED;
                break;

            case ENOENT:
                retStatus = STATUS_DIRECTORY_MISSING_PATH;
                break;

            default:
                retStatus = STATUS_REMOVE_FILE_FAILED;
        }
    }

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Gets the allocation size
 */
DEFINE_HEAP_GET_ALLOC_SIZE(hybridFileHeapGetAllocSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PHybridFileHeap pHybridHeap = (PHybridFileHeap) pHeap;
    ALLOCATION_HEADER allocationHeader;
    CHAR filePath[MAX_PATH_LEN + 1];
    UINT32 fileHandle;

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
    fileHandle = TO_FILE_HANDLE(handle);
    DLOGS("File heap allocation. Handle 0x%016" PRIx64 " File handle 0x%08x", handle, fileHandle);

    SPRINTF(filePath, "%s%c%u" FILE_HEAP_FILE_EXTENSION, pHybridHeap->rootDirectory, FPATHSEPARATOR, fileHandle);
    CHK_STATUS(readFileSegment(filePath, TRUE, (PBYTE) &allocationHeader, 0, FILE_ALLOCATION_HEADER_SIZE));

    // Set the values and return
    *pAllocSize = allocationHeader.size;

CleanUp:
    LEAVES();
    return retStatus;
}

/**
 * Sets the allocation size
 */
DEFINE_HEAP_SET_ALLOC_SIZE(hybridFileHeapSetAllocSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PHybridFileHeap pHybridHeap = (PHybridFileHeap) pHeap;
    ALLOCATION_HANDLE handle;
    ALLOCATION_HEADER allocationHeader;
    CHAR filePath[MAX_PATH_LEN + 1];
    UINT32 fileHandle;
    UINT64 overallSize;

    // Call the base class to ensure the params are ok and set the default ret values
    CHK_STATUS(commonHeapSetAllocSize(pHeap, pHandle, size, newSize));

    // overall allocation size
    overallSize = FILE_ALLOCATION_HEADER_SIZE + newSize + FILE_ALLOCATION_FOOTER_SIZE;

    handle = *pHandle;
    // In case it's a direct allocation handle use the encapsulated direct memory heap
    if (IS_DIRECT_ALLOCATION_HANDLE(handle)) {
        DLOGS("Direct allocation 0x%016" PRIx64, handle);
        retStatus = pHybridHeap->pMemHeap->heapSetAllocSizeFn((PHeap) pHybridHeap->pMemHeap, pHandle, size, newSize);
        CHK(retStatus == STATUS_SUCCESS || retStatus == STATUS_HEAP_REALLOC_ERROR, retStatus);

        // Exit on success
        CHK(!STATUS_SUCCEEDED(retStatus), STATUS_SUCCESS);

        // Reset the status
        retStatus = STATUS_SUCCESS;
    }

    // Perform in-place file size change
    fileHandle = TO_FILE_HANDLE(handle);
    DLOGS("Sets new allocation size %\" PRIu64 \" for handle 0x%016" PRIx64, newSize, handle);

    SPRINTF(filePath, "%s%c%u" FILE_HEAP_FILE_EXTENSION, pHybridHeap->rootDirectory, FPATHSEPARATOR, fileHandle);

    // Set the file size
    CHK_STATUS(setFileLength(filePath, overallSize));

    // Write the new size
    allocationHeader = gFileHeader;
    allocationHeader.size = newSize;

    CHK_STATUS(updateFile(filePath, TRUE, (PBYTE) &allocationHeader, 0, FILE_ALLOCATION_HEADER_SIZE));

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Map the allocation.
 * IMPORTANT: We will determine whether this is direct allocation by checking the last 2 bits being 0.
 * This works because all our allocators are at least 8 byte aligned so the handle will have it's
 * least significant two bits set as 0. The File heap handle will have it set to 1s.
 *
 * NOTE: In order to map a particular allocation, we will allocate from the common heap for now.
 * A more advanced implementation that requires a tight control over the memory and allocation might
 * choose to reserve a certain amount of memory from the direct RAM heap and allocate from there.
 */
DEFINE_HEAP_MAP(hybridFileHeapMap)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PHybridFileHeap pHybridHeap = (PHybridFileHeap) pHeap;
    CHAR filePath[MAX_PATH_LEN + 1];
    UINT32 fileHandle;
    PALLOCATION_HEADER pAllocation = NULL;
    UINT64 fileLength;

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
    fileHandle = TO_FILE_HANDLE(handle);
    DLOGS("File heap allocation. Handle 0x%016" PRIx64 " File handle 0x%08x", handle, fileHandle);

    SPRINTF(filePath, "%s%c%u" FILE_HEAP_FILE_EXTENSION, pHybridHeap->rootDirectory, FPATHSEPARATOR, fileHandle);

    // Get the file size, allocate and read the entire file into memory
    CHK_STATUS(getFileLength(filePath, &fileLength));

    CHK(NULL != (pAllocation = (PALLOCATION_HEADER) MEMALLOC(fileLength)), STATUS_NOT_ENOUGH_MEMORY);

    CHK_STATUS(readFile(filePath, TRUE, (PBYTE) pAllocation, &fileLength));
    CHK(pAllocation->type == FILE_ALLOCATION_TYPE && pAllocation->size == fileLength - FILE_ALLOCATION_HEADER_SIZE,
        STATUS_HEAP_FILE_HEAP_FILE_CORRUPT);

    // Set the values and return
    *ppAllocation = pAllocation + 1;
    *pSize = pAllocation->size;

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        SAFE_MEMFREE(pAllocation);
    }

    LEAVES();
    return retStatus;
}

/**
 * Un-Maps the allocation handle.
 */
DEFINE_HEAP_UNMAP(hybridFileHeapUnmap)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PHybridFileHeap pHybridHeap = (PHybridFileHeap) pHeap;
    PALLOCATION_HEADER pHeader = (PALLOCATION_HEADER) pAllocation - 1;
    CHAR filePath[MAX_PATH_LEN + 1];

    // Call the base class to ensure the params are ok
    CHK_STATUS(commonHeapUnmap(pHeap, pAllocation));

    // Check if this is a direct allocation by examining the type
    if (FILE_ALLOCATION_TYPE != pHeader->type) {
        DLOGS("Direct allocation");

        // This is a direct allocation - call the encapsulated heap
        CHK_STATUS(pHybridHeap->pMemHeap->heapUnmapFn((PHeap) pHybridHeap->pMemHeap, pAllocation));

        // Exit on success
        CHK(FALSE, STATUS_SUCCESS);
    }

    DLOGS("Indirect allocation");
    SPRINTF(filePath, "%s%c%u" FILE_HEAP_FILE_EXTENSION, pHybridHeap->rootDirectory, FPATHSEPARATOR, pHeader->fileHandle);

    // Un-maping in this case is simply writing the content into the file storage and releasing the mapped memory
    CHK_STATUS(writeFile(filePath, TRUE, FALSE, (PBYTE) pHeader, pHeader->size + FILE_ALLOCATION_HEADER_SIZE));

    SAFE_MEMFREE(pHeader);

CleanUp:
    LEAVES();
    return retStatus;
}

DEFINE_HEADER_SIZE(hybridFileGetAllocationHeaderSize)
{
    return FILE_ALLOCATION_HEADER_SIZE;
}

DEFINE_FOOTER_SIZE(hybridFileGetAllocationFooterSize)
{
    return FILE_ALLOCATION_FOOTER_SIZE;
}

DEFINE_ALIGNED_SIZE(hybridFileGetAllocationAlignedSize)
{
    return size;
}

DEFINE_ALLOC_SIZE(hybridFileGetAllocationSize)
{
    PHybridFileHeap pHybridHeap = (PHybridFileHeap) pHeap;
    CHAR filePath[MAX_PATH_LEN + 1];
    UINT32 fileHandle;
    ALLOCATION_HEADER allocationHeader;
    UINT64 memSizes, fileSizes, memHeapAllocationSize;

    CHECK_EXT(pHeap != NULL, "Internal error with file heap being null");

    // Check if this is a direct allocation
    if (IS_DIRECT_ALLOCATION_HANDLE(handle)) {
        // Get the allocation header and footer in order to compensate the accounting for file header and footer.
        memSizes = pHybridHeap->pMemHeap->getAllocationHeaderSizeFn() + pHybridHeap->pMemHeap->getAllocationFooterSizeFn();
        fileSizes = hybridFileGetAllocationHeaderSize() + hybridFileGetAllocationFooterSize();
        memHeapAllocationSize = pHybridHeap->pMemHeap->getAllocationSizeFn((PHeap) pHybridHeap->pMemHeap, handle);
        return memHeapAllocationSize - memSizes + fileSizes;
    }

    // In case of File allocation we need to read the header and get the size
    fileHandle = TO_FILE_HANDLE(handle);
    SPRINTF(filePath, "%s%c%u" FILE_HEAP_FILE_EXTENSION, pHybridHeap->rootDirectory, FPATHSEPARATOR, fileHandle);

    // Read the header to get the size info so we can allocate enough storage
    if (STATUS_FAILED(readFileSegment(filePath, TRUE, (PBYTE) &allocationHeader, 0, FILE_ALLOCATION_HEADER_SIZE))) {
        DLOGW("Failed to read the allocation header from file handle %lu", fileHandle);
        allocationHeader.size = 0;
    }

#ifdef HEAP_DEBUG
    // Check the allocation 'guard band' in debug mode
    if (0 != MEMCMP(allocationHeader.magic, ALLOCATION_HEADER_MAGIC, SIZEOF(ALLOCATION_HEADER_MAGIC))) {
        DLOGE("Invalid header for file allocation handle %lu", fileHandle);
        return INVALID_ALLOCATION_VALUE;
    }

    // We don't use the footer for file allocation for perf purpose

    // Check the type
    if (FILE_ALLOCATION_TYPE != allocationHeader.type) {
        DLOGE("Invalid allocation type 0x%08x for file handle %lu", allocationHeader.type, fileHandle);
        return INVALID_ALLOCATION_VALUE;
    }
#endif

    return FILE_ALLOCATION_HEADER_SIZE + allocationHeader.size + FILE_ALLOCATION_FOOTER_SIZE;
}

DEFINE_HEAP_LIMITS(hybridFileGetHeapLimits)
{
    *pMinHeapSize = MIN_HEAP_SIZE;
    *pMaxHeapSize = MAX_LARGE_HEAP_SIZE;
}

STATUS removeHeapFile(UINT64 callerData, DIR_ENTRY_TYPES entryType, PCHAR path, PCHAR name)
{
    STATUS retStatus = STATUS_SUCCESS;
    INT32 retCode;
    SIZE_T strLen;

    UNUSED_PARAM(callerData);

    // Early return on non-file types
    CHK(entryType == DIR_ENTRY_TYPE_FILE, retStatus);

    // Check if the file ends with the extension for the hybrid file heap
    strLen = STRLEN(name);
    CHK(strLen >= ARRAY_SIZE(FILE_HEAP_FILE_EXTENSION) &&
            (0 == STRNCMP(FILE_HEAP_FILE_EXTENSION, name + strLen - ARRAY_SIZE(FILE_HEAP_FILE_EXTENSION) + 1, ARRAY_SIZE(FILE_HEAP_FILE_EXTENSION))),
        retStatus);

    DLOGS("Removing heap file %s", path);
    retCode = FREMOVE(path);

    if (retCode != 0) {
        switch (errno) {
            case EACCES:
                retStatus = STATUS_DIRECTORY_ACCESS_DENIED;
                break;

            case ENOENT:
                retStatus = STATUS_DIRECTORY_MISSING_PATH;
                break;

            default:
                retStatus = STATUS_REMOVE_FILE_FAILED;
        }
    }

CleanUp:
    return retStatus;
}
