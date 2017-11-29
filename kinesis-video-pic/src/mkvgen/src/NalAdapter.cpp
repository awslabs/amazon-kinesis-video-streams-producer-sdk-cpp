/**
 * Helper functionality for NALU adaptation from Annex-B to AVCC format
 */

#define LOG_CLASS "NalAdapter"
#include "Include_i.h"

/**
 * NALU adaptation from Annex-B to AVCC format
 */
STATUS adaptFrameNalsFromAnnexBToAvcc(PBYTE pFrameData,
                                      UINT32 frameDataSize,
                                      PBYTE pAdaptedFrameData,
                                      PUINT32 pAdaptedFrameDataSize)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i = 0, zeroCount = 0, runSize = 0;
    BOOL markerFound = FALSE;
    PBYTE pCurPnt = pFrameData, pAdaptedCurPnt = pAdaptedFrameData, pRunStart = NULL;

    CHK(pFrameData != NULL && pAdaptedFrameDataSize != NULL, STATUS_NULL_ARG);
    CHK(pAdaptedFrameData == NULL || *pAdaptedFrameDataSize >= frameDataSize, STATUS_INVALID_ARG_LEN);

    // Quick check for small size
    CHK(frameDataSize != 0, retStatus);

    // Calculate the size after adaptation
    // NOTE: We will not check for the correct buffer size for performance reasons.
    for (i = 0; i < frameDataSize; i++) {
        if (*pCurPnt == 0x00) {
            // Found a zero - increment the consecutive zero count
            zeroCount++;

            // Reset the last marker
            markerFound = FALSE;

            // Ensure we don't have over 3 zero's in a row
            CHK(zeroCount <= 3, STATUS_MKV_INVALID_ANNEXB_NALU_IN_FRAME_DATA);
        } else if (*pCurPnt == 0x01 && (zeroCount == 2 || zeroCount == 3)) {
            // Found the Annex-B NAL
            // Check if we have previous run and fix it up
            if (pRunStart != NULL && pAdaptedFrameData != NULL) {
                // Fix-up the previous run by recording the size in Big-Endian format
                putInt32((PINT32) pRunStart, runSize);
            }

            // Store the beginning of the run
            pRunStart = pAdaptedCurPnt;

            // Increment the adapted pointer to 4 bytes
            pAdaptedCurPnt += 4;

            // Store an indicator that we have a marker
            markerFound = TRUE;

            // Start the new run
            zeroCount = 0;
            runSize = 0;
        } else {
            // Advance the current pointer and the run size to the number of zeros so far
            // as well as add the zeros to the adapted buffer if any
            runSize += zeroCount + 1; // for the current byte
            pAdaptedCurPnt += zeroCount + 1;

            // If the adapted frame is specified then copy the zeros and then the current byte
            if (pAdaptedFrameData != NULL) {
                *(pAdaptedCurPnt - 1) = *pCurPnt;

                while (zeroCount != 0) {
                    *(pAdaptedCurPnt - 1 - zeroCount) = 0x00;
                    zeroCount--;
                }
            }

            zeroCount = 0;
            markerFound = FALSE;
        }

        // Iterate the current
        pCurPnt++;
    }

    // We could still have last few zeros at the end of the frame data and need to fix-up the last NAL
    pAdaptedCurPnt += zeroCount;
    if (pAdaptedFrameData != NULL) {

        // The last remaining zeros should go towards the run size
        runSize += zeroCount;

        while (zeroCount != 0) {
            *(pAdaptedCurPnt - zeroCount) = 0x00;
            zeroCount--;
        }

        // Fix-up the last run
        if (pRunStart != NULL) {
            putInt32((PINT32) pRunStart, runSize);
        }

        // Also, handle the case where there is a last 001/0001 at the end of the frame - we will fill with 0s
        if (markerFound) {
            *((PUINT32) pAdaptedCurPnt - 1) = 0;
        }
    }

CleanUp:

    if (pAdaptedFrameDataSize != NULL) {
        *pAdaptedFrameDataSize = pAdaptedCurPnt - pAdaptedFrameData;
    }

    return retStatus;
}

/**
 * NALU adaptation from AVCC to Annex-B format
 *
 * NOTE: The adaptation happens in-place
 */
STATUS adaptFrameNalsFromAvccToAnnexB(PBYTE pFrameData,
                                      UINT32 frameDataSize)
{
    STATUS retStatus = STATUS_SUCCESS;
    PBYTE pCurPnt = pFrameData, pEndPnt;
    UINT32 runLen = 0;

    CHK(pFrameData != NULL, STATUS_NULL_ARG);

    // Quick check for size
    CHK(frameDataSize > 3, STATUS_MKV_INVALID_AVCC_NALU_IN_FRAME_DATA);

    pEndPnt = pCurPnt + frameDataSize;

    while (pCurPnt != pEndPnt) {
        // Check if we can still read 32 bit
        CHK(pCurPnt + SIZEOF(UINT32) <= pEndPnt, STATUS_MKV_INVALID_AVCC_NALU_IN_FRAME_DATA);

        runLen = getInt32(*(PUINT32) pCurPnt);

        CHK(pCurPnt + runLen <= pEndPnt, STATUS_MKV_INVALID_AVCC_NALU_IN_FRAME_DATA);

        // Adapt with 4 byte version of the start sequence
        putInt32((PINT32) pCurPnt, 0x0001);

        // Jump to the next NAL
        pCurPnt += runLen + SIZEOF(UINT32);
    }

CleanUp:

    return retStatus;
}

/**
 * Codec Private Data NALU adaptation from Annex-B to AVCC format for H264
 *
 * NOTE: We are processing H264 and a single SPS/PPS only for now.
 */
STATUS adaptCpdNalsFromAnnexBToAvcc(PBYTE pCpd,
                                    UINT32 cpdSize,
                                    PBYTE pAdaptedCpd,
                                    PUINT32 pAdaptedCpdSize)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 spsSize, ppsSize, adaptedRawSize, adaptedCpdSize = 0;
    PBYTE pAdaptedBits = NULL, pCurPnt = pAdaptedCpd, pSrcPnt;

    CHK(pCpd != NULL && pAdaptedCpdSize != NULL, STATUS_NULL_ARG);

    // We should have at least 2 NALs with at least single byte data so even the short start code shuold account for 8
    CHK(cpdSize >= MIN_ANNEXB_CPD_SIZE, STATUS_INVALID_ARG_LEN);

    // Calculate the adapted size first. The size should be an overall Avcc package size + the raw sps and pps sizes.
    // The AnnexB packaged sps pps should be the raw size minus the delimiters which can be 2 three bytes start codes.
    adaptedCpdSize = AVCC_CPD_OVERHEAD + cpdSize - 2 * 3;

    // Quick check for size only
    CHK(pAdaptedCpd != NULL, retStatus);

    // Convert the raw bits
    CHK_STATUS(adaptFrameNalsFromAnnexBToAvcc(pCpd, cpdSize, NULL, &adaptedRawSize));

    // If we get a larger buffer size then it's an error
    CHK(adaptedRawSize <= *pAdaptedCpdSize, STATUS_BUFFER_TOO_SMALL);

    // Allocate enough storage to store the data temporarily
    pAdaptedBits = (PBYTE) MEMALLOC(adaptedRawSize);
    CHK(pAdaptedBits != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Get the converted bits
    CHK_STATUS(adaptFrameNalsFromAnnexBToAvcc(pCpd, cpdSize, pAdaptedBits, &adaptedRawSize));

    // Set the source pointer to walk the adapted data
    pSrcPnt = pAdaptedBits;

    // Get the size of the run
    spsSize = getInt32(*(PUINT32) pSrcPnt);
    pSrcPnt += SIZEOF(UINT32);

    // See if we are still in the buffer
    CHK(spsSize + 8 <= *pAdaptedCpdSize && spsSize + 4 + 4 <= adaptedRawSize, STATUS_MKV_INVALID_ANNEXB_NALU_IN_CPD);

    // Start converting and copying it to the output
    *pCurPnt++ = AVCC_VERSION_CODE; // Version
    *pCurPnt++ = *(pSrcPnt + 1); // Profile
    *pCurPnt++ = *(pSrcPnt + 2); // Compat
    *pCurPnt++ = *(pSrcPnt + 3); // Level

    *pCurPnt++ = AVCC_NALU_LEN_MINUS_ONE;
    *pCurPnt++ = AVCC_NUMBER_OF_SPS_ONE;

    // Write the SPS size in big-endian format
    putInt16((PINT16) pCurPnt, (UINT16) spsSize);
    pCurPnt += SIZEOF(UINT16);

    // Copy the actual bits
    MEMCPY(pCurPnt, pSrcPnt, spsSize);
    pCurPnt += spsSize;

    // Move the source pointer to the next nal
    pSrcPnt += spsSize;

    // Get the pps size
    ppsSize = getInt32(*(PUINT32) pSrcPnt);
    pSrcPnt += SIZEOF(UINT32);

    CHK(spsSize + 8 + 1 + ppsSize <= *pAdaptedCpdSize && spsSize + 4 + 4 + ppsSize <= adaptedRawSize, STATUS_MKV_INVALID_ANNEXB_NALU_IN_CPD);

    // Copy the pps size
    *pCurPnt++ = 1; // One pps nal

    // write the size of the pps
    putInt16((PINT16) pCurPnt, (UINT16) ppsSize);
    pCurPnt += SIZEOF(UINT16);

    // Write the PPS data
    MEMCPY(pCurPnt, pSrcPnt, ppsSize);
    pCurPnt += ppsSize;

    // Precise adapted size
    adaptedCpdSize = pCurPnt - pAdaptedCpd;

CleanUp:

    if (pAdaptedCpdSize != NULL) {
        *pAdaptedCpdSize = adaptedCpdSize;
    }

    if (pAdaptedBits != NULL) {
        MEMFREE(pAdaptedBits);
    }

    return retStatus;
}
