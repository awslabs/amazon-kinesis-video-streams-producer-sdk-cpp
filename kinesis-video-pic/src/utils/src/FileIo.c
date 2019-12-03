#include "Include_i.h"

/**
 * Read a file from the given full/relative filePath into the memory area pointed to by pBuffer.
 * Specifying NULL in pBuffer will return the size of the file.
 *
 * Parameters:
 *     filePath - file path to read from
 *     binMode  - TRUE to read file stream as binary; FALSE to read as a normal text file
 *     pBuffer  - buffer to write contents of the file to. If NULL return the size in pSize.
 *     pSize    - destination PUINT64 to store the size of the file when pBuffer is NULL;
 */
STATUS readFile(PCHAR filePath, BOOL binMode, PBYTE pBuffer, PUINT64 pSize)
{
    UINT64 fileLen;
    STATUS retStatus = STATUS_SUCCESS;
    FILE *fp = NULL;

    CHK(filePath != NULL && pSize != NULL, STATUS_NULL_ARG);

    fp = FOPEN(filePath, binMode ? "rb" : "r");

    CHK(fp != NULL, STATUS_OPEN_FILE_FAILED);

    // Get the size of the file
    FSEEK(fp, 0, SEEK_END);
    fileLen = FTELL(fp);

    if (pBuffer == NULL) {
        // requested the length - set and early return
        *pSize = fileLen;
        CHK(FALSE, STATUS_SUCCESS);
    }

    // Validate the buffer size
    CHK(fileLen <= *pSize, STATUS_BUFFER_TOO_SMALL);

    // Read the file into memory buffer
    FSEEK(fp, 0, SEEK_SET);
    CHK(FREAD(pBuffer, (SIZE_T) fileLen, 1, fp) == 1, STATUS_READ_FILE_FAILED);

CleanUp:

    if (fp != NULL) {
        FCLOSE(fp);
        fp = NULL;
    }

    return retStatus;
}

/**
 * Read a section of the file from the given full/relative filePath into the memory area pointed to by pBuffer.
 *
 * NOTE: The buffer should be large enough to read the section.
 *
 * Parameters:
 *     filePath - file path to read from
 *     binMode  - TRUE to read file stream as binary; FALSE to read as a normal text file
 *     pBuffer  - buffer to write contents of the file to. Non-null
 *     offset   - Offset into the file to start reading from.
 *     readSize - The number of bytes to read from the file.
 */
STATUS readFileSegment(PCHAR filePath, BOOL binMode, PBYTE pBuffer, UINT64 offset, UINT64 readSize)
{
    UINT64 fileLen;
    STATUS retStatus = STATUS_SUCCESS;
    FILE *fp = NULL;
    INT32 result = 0;

    CHK(filePath != NULL && pBuffer != NULL && readSize != 0, STATUS_NULL_ARG);

    fp = FOPEN(filePath, binMode ? "rb" : "r");

    CHK(fp != NULL, STATUS_OPEN_FILE_FAILED);

    // Get the size of the file
    FSEEK(fp, 0, SEEK_END);
    fileLen = FTELL(fp);

    // Check if we are trying to read past the end of the file
    CHK(offset + readSize <= fileLen, STATUS_READ_FILE_FAILED);

    // Set the offset and read the file content
    result = FSEEK(fp, (UINT32) offset, SEEK_SET);
    CHK(result && (FREAD(pBuffer, (SIZE_T) readSize, 1, fp) == 1), STATUS_READ_FILE_FAILED);

CleanUp:

    if (fp != NULL) {
        FCLOSE(fp);
        fp = NULL;
    }

    return retStatus;
}

/**
 * Write contents pointed to by pBuffer to the given filePath.
 *
 * Parameters:
 *     filePath - file path to write to
 *     binMode  - TRUE to read file stream as binary; FALSE to read as a normal text file
 *     append   - TRUE to append; FALSE to overwrite
 *     pBuffer  - memory location whose contents should be written to the file
 *     size     - number of bytes that should be written to the file
 */
STATUS writeFile(PCHAR filePath, BOOL binMode, BOOL append, PBYTE pBuffer, UINT64 size)
{
    STATUS retStatus = STATUS_SUCCESS;
    FILE *fp = NULL;

    CHK(filePath != NULL && pBuffer != NULL, STATUS_NULL_ARG);

    fp = FOPEN(filePath, binMode ? (append ? "ab" : "wb") : (append ? "a" : "w"));

    CHK(fp != NULL, STATUS_OPEN_FILE_FAILED);

    // Write the buffer to the file
    CHK(FWRITE(pBuffer, (SIZE_T) size, 1, fp) == 1, STATUS_WRITE_TO_FILE_FAILED);

CleanUp:

    if (fp != NULL) {
        FCLOSE(fp);
        fp = NULL;
    }

    return retStatus;
}

/**
 * Gets the file length of the given filePath.
 *
 * Parameters:
 *     filePath - file path whose file length should be computed
 *     pLength  - Returns the size of the file in bytes
 *
 * Returns:
 *     STATUS of the operation
 */
STATUS getFileLength(PCHAR filePath, PUINT64 pLength)
{
    STATUS retStatus = STATUS_SUCCESS;

    CHK_STATUS(readFile(filePath, TRUE, NULL, pLength));

CleanUp:

    return retStatus;
}

/**
 * Checks if the file or directory exists with a given full or relative path
 *
 * Parameters:
 *      filePath - file path to check
 *      pExists - TRUE if the file exists
 */
STATUS fileExists(PCHAR filePath, PBOOL pExists)
{
    if (filePath == NULL || pExists == NULL) {
        return STATUS_NULL_ARG;
    }

    struct GLOBAL_STAT st;
    INT32 result = FSTAT(filePath, &st);
    *pExists = (result == 0);

    return STATUS_SUCCESS;
}

/**
 * Creates/overwrites a new file with a given size
 *
 * Parameters:
 *      filePath - file path to check
 *      size - The size of the newly created file
 */
STATUS createFile(PCHAR filePath, UINT64 size)
{
    STATUS retStatus = STATUS_SUCCESS;
    FILE *fp = NULL;

    CHK(filePath != NULL, STATUS_NULL_ARG);

    fp = FOPEN(filePath, "w+b");
    CHK(fp != NULL, STATUS_OPEN_FILE_FAILED);

    if (size != 0) {
        CHK(0 == FSEEK(fp, (UINT32) (size - 1), SEEK_SET), STATUS_INVALID_OPERATION);
        CHK(0 == FPUTC(0, fp), STATUS_INVALID_OPERATION);
    }

CleanUp:

    if (fp != NULL) {
        FCLOSE(fp);
        fp = NULL;
    }

    return retStatus;
}
