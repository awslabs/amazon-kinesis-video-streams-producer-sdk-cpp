#include "gtest/gtest.h"
#include <com/amazonaws/kinesis/video/utils/Include.h>

#pragma GCC diagnostic ignored "-Wwrite-strings"

#define TEMP_TEST_DIRECTORY_PATH "temp"

/**
 * Helper to print out directory info
 */
STATUS printDirInfo(UINT64 callerData, DIR_ENTRY_TYPES entryType, PCHAR path, PCHAR name)
{
    CHAR typeName[100];
    switch (entryType) {
        case DIR_ENTRY_TYPE_DIRECTORY:
            STRCPY(typeName, "Directory");

            if (0 != STRNCMP(name, "tmp_dir_", 8)) {
                return STATUS_INVALID_OPERATION;
            }

            break;
        case DIR_ENTRY_TYPE_FILE:
            STRCPY(typeName, "File");

            if (0 != STRNCMP(name, "file_", 5)) {
                return STATUS_INVALID_OPERATION;
            }

            break;
        case DIR_ENTRY_TYPE_LINK:
            STRCPY(typeName, "Link");
            break;
        case DIR_ENTRY_TYPE_UNKNOWN:
            STRCPY(typeName, "Unknown");
            break;
        default:
            STRCPY(typeName, "Other");
    }

    printf("Caller data %p, Type %s, \t Path %s \t Name %s\n", (PUINT64) callerData, typeName, path, name);

    PUINT64 pCount = (PUINT64) callerData;
    (*pCount)++;

    return STATUS_SUCCESS;
}

/**
 * Helper to create a directory structure with a number of files
 */
STATUS createSubDirStruct(PCHAR dirPath, UINT32 depth, UINT32 numberOfFiles, UINT32 numberOfDirectories)
{
    CHAR temp[100];
    if (depth == 0) {
        return STATUS_SUCCESS;
    }

    UINT32 strLen = STRLEN(dirPath);

    // Create the files first
    for (UINT32 i = 0; i < numberOfFiles; i++) {
        // Create the file path
        sprintf(temp, "/file_%03d.tmp", i);
        STRCAT(dirPath, temp);

        // Create/write the file
        STATUS status = writeFile(dirPath, FALSE, (PBYTE) temp, 100);
        if (status != STATUS_SUCCESS) {
            return status;
        }

        // clear the string
        dirPath[strLen] = '\0';
    }

    // Create the directories and iterate
    for (UINT32 i = 0; i < numberOfDirectories; i++) {
        // Create the file path
        sprintf(temp, "/tmp_dir_%03d", i);
        STRCAT(dirPath, temp);

        // Create the dir
        if (0 != FMKDIR(dirPath, 0777)) {
            return STATUS_INVALID_OPERATION;
        }

        // Iterate into it
        STATUS status = createSubDirStruct(dirPath, depth - 1, numberOfFiles, numberOfDirectories);
        if (status != STATUS_SUCCESS) {
            return status;
        }

        // clear the string
        dirPath[strLen] = '\0';
    }

    return STATUS_SUCCESS;
}

STATUS createDirStruct()
{
    CHAR tempStr[MAX_PATH_LEN];

    // Remove the existing
    removeDirectory(TEMP_TEST_DIRECTORY_PATH);

    system("rm -rf " TEMP_TEST_DIRECTORY_PATH);

    // Start creating
    FMKDIR(TEMP_TEST_DIRECTORY_PATH, 0777);

    STRCPY(tempStr, TEMP_TEST_DIRECTORY_PATH);

    return createSubDirStruct(tempStr, 3, 3, 3);
}

TEST(NegativeInvalidInput, DirectoryTraverseRemoveGetSize)
{
    CHAR tempBuf[10];
    tempBuf[0] = '\0';
    UINT64 size;

    EXPECT_NE(STATUS_SUCCESS, traverseDirectory(NULL, 0, TRUE, NULL));
    EXPECT_NE(STATUS_SUCCESS, traverseDirectory(NULL, 0, TRUE, printDirInfo));
    EXPECT_NE(STATUS_SUCCESS, traverseDirectory(tempBuf, 0, TRUE, printDirInfo));
    EXPECT_NE(STATUS_SUCCESS, traverseDirectory(TEMP_TEST_DIRECTORY_PATH, 0, TRUE, NULL));
    EXPECT_NE(STATUS_SUCCESS, traverseDirectory(NULL, 0, FALSE, NULL));
    EXPECT_NE(STATUS_SUCCESS, traverseDirectory(NULL, 0, FALSE, printDirInfo));
    EXPECT_NE(STATUS_SUCCESS, traverseDirectory(tempBuf, 0, FALSE, printDirInfo));
    EXPECT_NE(STATUS_SUCCESS, traverseDirectory(TEMP_TEST_DIRECTORY_PATH, 0, FALSE, NULL));

    EXPECT_NE(STATUS_SUCCESS, removeDirectory(NULL));
    EXPECT_NE(STATUS_SUCCESS, removeDirectory(tempBuf));
    EXPECT_NE(STATUS_SUCCESS, getDirectorySize(NULL, &size));
    EXPECT_NE(STATUS_SUCCESS, getDirectorySize("/data/data", NULL));
    EXPECT_NE(STATUS_SUCCESS, getDirectorySize(tempBuf, &size));
    EXPECT_NE(STATUS_SUCCESS, getDirectorySize(tempBuf, NULL));
}

TEST(FunctionalTest, CreateTraverseRemoveDirs)
{
    UINT64 count;
    EXPECT_EQ(STATUS_SUCCESS, createDirStruct());

    count = 0;
    EXPECT_EQ(STATUS_SUCCESS, traverseDirectory(TEMP_TEST_DIRECTORY_PATH, (UINT64) &count, TRUE, printDirInfo));
    EXPECT_EQ((3 + 3 + 3 * (3 + 3  + 3 * (3 + 3))), count);

    count = 0;
    EXPECT_EQ(STATUS_SUCCESS, traverseDirectory(TEMP_TEST_DIRECTORY_PATH, (UINT64) &count, FALSE, printDirInfo));
    EXPECT_EQ(3 + 3, count);

    EXPECT_EQ(STATUS_SUCCESS, removeDirectory(TEMP_TEST_DIRECTORY_PATH));

    // Perform the same with the ending separator
    EXPECT_EQ(STATUS_SUCCESS, createDirStruct());

    count = 0;
    EXPECT_EQ(STATUS_SUCCESS, traverseDirectory(TEMP_TEST_DIRECTORY_PATH "/", (UINT64) &count, TRUE, printDirInfo));
    EXPECT_EQ((3 + 3 + 3 * (3 + 3  + 3 * (3 + 3))), count);

    count = 0;
    EXPECT_EQ(STATUS_SUCCESS, traverseDirectory(TEMP_TEST_DIRECTORY_PATH "/", (UINT64) &count, FALSE, printDirInfo));
    EXPECT_EQ(3 + 3, count);

    EXPECT_EQ(STATUS_SUCCESS, removeDirectory(TEMP_TEST_DIRECTORY_PATH "/"));
}

TEST(FunctionalTest, CreateTraverseGetDirSize)
{
    UINT64 size;
    EXPECT_EQ(STATUS_SUCCESS, createDirStruct());
    EXPECT_EQ(STATUS_SUCCESS, traverseDirectory(TEMP_TEST_DIRECTORY_PATH, (UINT64) &size, TRUE, printDirInfo));
    EXPECT_EQ(STATUS_SUCCESS, getDirectorySize(TEMP_TEST_DIRECTORY_PATH, &size));
    EXPECT_EQ((((100 * 3) * 3 + 3 * 100) * 3) + 3 * 100, size);

    // Perform the same with the ending separator
    EXPECT_EQ(STATUS_SUCCESS, createDirStruct());
    EXPECT_EQ(STATUS_SUCCESS, traverseDirectory(TEMP_TEST_DIRECTORY_PATH "/", (UINT64) &size, TRUE, printDirInfo));
    EXPECT_EQ(STATUS_SUCCESS, getDirectorySize(TEMP_TEST_DIRECTORY_PATH "/", &size));
    EXPECT_EQ((((100 * 3) * 3 + 3 * 100) * 3) + 3 * 100, size);

    // Remove the directories
    EXPECT_EQ(STATUS_SUCCESS, removeDirectory(TEMP_TEST_DIRECTORY_PATH));
}
