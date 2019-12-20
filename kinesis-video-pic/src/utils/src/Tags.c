#include "Include_i.h"

/**
 * Validates the tags
 *
 * @param 1 UINT32 - Number of tags
 * @param 2 PTag - Array of tags
 *
 * @return Status of the function call.
 */
STATUS validateTags(UINT32 tagCount, PTag tags)
{
    UINT32 i;
    STATUS retStatus = STATUS_SUCCESS;

    CHK(tagCount <= MAX_TAG_COUNT, STATUS_UTIL_MAX_TAG_COUNT);

    // If we have tag count not 0 then tags can't be NULL
    CHK(tagCount == 0 || tags != NULL, STATUS_UTIL_TAGS_COUNT_NON_ZERO_TAGS_NULL);
    for (i = 0; i < tagCount; i++) {
        // Validate the tag version
        CHK(tags[i].version <= TAG_CURRENT_VERSION, STATUS_UTIL_INVALID_TAG_VERSION);

        // Validate the tag name
        CHK(STRNLEN(tags[i].name, MAX_TAG_NAME_LEN + 1) <= MAX_TAG_NAME_LEN, STATUS_UTIL_INVALID_TAG_NAME_LEN);

        // Validate the tag value
        CHK(STRNLEN(tags[i].value, MAX_TAG_VALUE_LEN + 1) <= MAX_TAG_VALUE_LEN, STATUS_UTIL_INVALID_TAG_VALUE_LEN);
    }

CleanUp:

    return retStatus;
}

/**
 * Packages the tags in to a destination buffer.
 * NOTE: The tags are assumed to have been validated
 */
STATUS packageTags(UINT32 tagCount, PTag pSrcTags, UINT32 tagsSize, PTag pDstTags, PUINT32 pSize)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i, curSize = tagCount * TAG_FULL_LENGTH, remaining = tagsSize,
        nameSize, valueSize, structSize, alignedNameSize, alignedValueSize;
    PBYTE pCurPtr;

    CHK(tagCount == 0 || pSrcTags != NULL, STATUS_UTIL_TAGS_COUNT_NON_ZERO_TAGS_NULL);

    // Quick check for anything to be done
    CHK(pDstTags != NULL && tagCount != 0, retStatus);

    structSize = SIZEOF(Tag) * tagCount;
    pCurPtr = (PBYTE) pDstTags + structSize;
    CHK(remaining >= structSize, STATUS_NOT_ENOUGH_MEMORY);
    remaining -= structSize;
    curSize = structSize;

    for (i = 0; i < tagCount; i++) {
        // Get the name and value lengths - those should have been validated already
        nameSize = (UINT32) (STRLEN(pSrcTags[i].name) + 1) * SIZEOF(CHAR);
        valueSize = (UINT32) (STRLEN(pSrcTags[i].value) + 1) * SIZEOF(CHAR);
        alignedNameSize = ROUND_UP(nameSize, SIZEOF(SIZE_T));
        alignedValueSize = ROUND_UP(valueSize, SIZEOF(SIZE_T));
        CHK(remaining >= alignedNameSize + alignedValueSize, STATUS_NOT_ENOUGH_MEMORY);

        pDstTags[i].version = pSrcTags[i].version;

        // Fix-up the pointers first then copy
        pDstTags[i].name = (PCHAR) pCurPtr;
        MEMCPY(pDstTags[i].name, pSrcTags[i].name, nameSize);
        pCurPtr += alignedNameSize;

        pDstTags[i].value = (PCHAR) pCurPtr;
        MEMCPY(pDstTags[i].value, pSrcTags[i].value, valueSize);
        pCurPtr += alignedValueSize;

        remaining -= alignedNameSize + alignedValueSize;
        curSize += alignedNameSize + alignedValueSize;
    }

CleanUp:

    if (pSize != NULL) {
        *pSize = curSize;
    }

    return retStatus;
}