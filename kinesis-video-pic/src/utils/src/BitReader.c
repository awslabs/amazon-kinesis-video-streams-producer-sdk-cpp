#include "Include_i.h"

/**
 * Resets the reader
 */
STATUS bitReaderReset(PBitReader pBitReader, PBYTE buffer, UINT32 bitBufferSize)
{
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pBitReader != NULL && buffer != NULL, STATUS_NULL_ARG);

    pBitReader->buffer = buffer;
    pBitReader->bitBufferSize = bitBufferSize;
    pBitReader->currentBit = 0;

CleanUp:

    return retStatus;
}

STATUS bitReaderSetCurrent(PBitReader pBitReader, UINT32 current)
{
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pBitReader != NULL, STATUS_NULL_ARG);
    CHK(pBitReader->bitBufferSize > current, STATUS_BIT_READER_OUT_OF_RANGE);

    pBitReader->currentBit = current;

CleanUp:

    return retStatus;
}

STATUS bitReaderReadBit(PBitReader pBitReader, PUINT32 pRead)
{
    STATUS retStatus = STATUS_SUCCESS;
    BYTE byte, shift;

    CHK(pBitReader != NULL && pRead != NULL, STATUS_NULL_ARG);
    CHK(pBitReader->currentBit < pBitReader->bitBufferSize, STATUS_BIT_READER_OUT_OF_RANGE);

    // Get the byte
    byte = *(pBitReader->buffer + (pBitReader->currentBit >> 3));

    // Offset of the bit
    shift = 8 - (pBitReader->currentBit % 8 + 1);

    *pRead = (byte >> shift) & 0x01;

    // Increment the current pointer
    pBitReader->currentBit++;

CleanUp:

    return retStatus;
}

STATUS bitReaderReadBits(PBitReader pBitReader, UINT32 bitCount, PUINT32 pRead)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i = 0, retVal = 0, readBit;

    // The res will be verified by the bit read routine
    CHK(pRead != NULL, STATUS_NULL_ARG);
    CHK(bitCount <= 32, STATUS_BIT_READER_INVALID_SIZE);

    for (i = 0; i < bitCount; i++) {
        CHK_STATUS(bitReaderReadBit(pBitReader, &readBit));
        retVal |= readBit << (bitCount - i - 1);
    }

    *pRead = retVal;

CleanUp:

    return retStatus;
}

STATUS bitReaderReadExpGolomb(PBitReader pBitReader, PUINT32 pRead)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i = 0, retVal = 0, readBit, shifted;
    BOOL readZeros = TRUE;

    // The rest will be verified by the called routines
    CHK(pRead != NULL, STATUS_NULL_ARG);

    while (readZeros && (i < 32)) {
        CHK_STATUS(bitReaderReadBit(pBitReader, &readBit));

        if (readBit != 0) {
            readZeros = FALSE;
        } else {
            i++;
        }
    }

    CHK_STATUS(bitReaderReadBits(pBitReader, i, &retVal));

    shifted = (UINT32)((UINT64) 1 << i);
    *pRead = retVal + shifted - 1;

CleanUp:

    return retStatus;
}

STATUS bitReaderReadExpGolombSe(PBitReader pBitReader, PINT32 pRead)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 exGolomb = 0;
    INT32 retVal;

    // The rest will be checked by called routines
    CHK(pRead != NULL, STATUS_NULL_ARG);

    CHK_STATUS(bitReaderReadExpGolomb(pBitReader, &exGolomb));

    if (exGolomb & 0x01) {
        retVal = (exGolomb + 1) / 2;
    } else {
        retVal = -((INT64)(exGolomb / 2));
    }

    *pRead = retVal;

CleanUp:

    return retStatus;
}
