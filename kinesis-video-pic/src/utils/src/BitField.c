#include "Include_i.h"

/**
 * Create a new bitfield
 */
STATUS bitFieldCreate(UINT32 itemCount, PBitField* ppBitField)
{
    STATUS retStatus = STATUS_SUCCESS;
    PBitField pBitField = NULL;
    UINT32 allocSize;
    UINT32 byteCount;

    CHK(itemCount != 0 && ppBitField != NULL, STATUS_NULL_ARG);

    // Pre-set the default
    *ppBitField = NULL;

    // Calculate the size of the main allocation
    byteCount = GET_BYTE_COUNT_FOR_BITS(itemCount);
    allocSize = SIZEOF(BitField) + byteCount;

    // Allocate the main structure
    pBitField = (PBitField) MEMCALLOC(1, allocSize);
    CHK(pBitField != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Set the values. NOTE: The bit field follows immediately after the main struct
    pBitField->itemCount = itemCount;

    // Finally, set the return value
    *ppBitField = pBitField;

CleanUp:

    // Clean-up on error
    if (STATUS_FAILED(retStatus)) {
        // Free everything
        bitFieldFree(pBitField);
    }

    return retStatus;
}

/**
 * Frees and de-allocates the bit field
 */
STATUS bitFieldFree(PBitField pBitField)
{
    STATUS retStatus = STATUS_SUCCESS;

    // The call is idempotent so we shouldn't fail
    CHK(pBitField != NULL, retStatus);

    // Free the structure itself
    MEMFREE(pBitField);

CleanUp:

    return retStatus;
}

/**
 * Resets the entire bit field to 0 or 1
 */
STATUS bitFieldReset(PBitField pBitField, BOOL isSet)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 byteCount;
    BYTE byteVal = isSet ? 0xff : 0x00;

    CHK(pBitField != NULL, STATUS_NULL_ARG);
    byteCount = GET_BYTE_COUNT_FOR_BITS(pBitField->itemCount);

    // Set the entire bit field
    MEMSET(pBitField + 1, byteVal, byteCount);

CleanUp:

    return retStatus;
}

/**
 * Gets the bit field size in items
 */
STATUS bitFieldGetCount(PBitField pBitField, PUINT32 pItemCount)
{
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pBitField != NULL && pItemCount != NULL, STATUS_NULL_ARG);

    *pItemCount = pBitField->itemCount;

CleanUp:

    return retStatus;
}

/**
 * Gets the value of the bit
 */
STATUS bitFieldGet(PBitField pBitField, UINT32 index, PBOOL pIsSet)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT8 byte;

    CHK(pBitField != NULL && pIsSet != NULL, STATUS_NULL_ARG);
    CHK(index < pBitField->itemCount, STATUS_INVALID_ARG);

    byte = *((PBYTE)(pBitField + 1) + (index >> 3));
    *pIsSet = (byte & (0x80 >> (index % 8))) != 0;

CleanUp:

    return retStatus;
}

/**
 * Sets the value of the bit
 */
STATUS bitFieldSet(PBitField pBitField, UINT32 index, BOOL isSet)
{
    STATUS retStatus = STATUS_SUCCESS;
    PUINT8 pByte;

    CHK(pBitField != NULL, STATUS_NULL_ARG);
    CHK(index < pBitField->itemCount, STATUS_INVALID_ARG);

    pByte = (PBYTE)(pBitField + 1) + (index >> 3);
    if (isSet) {
        *pByte |= (0x80 >> (index % 8));
    } else {
        *pByte &= ~(0x80 >> (index % 8));
    }

CleanUp:

    return retStatus;
}
