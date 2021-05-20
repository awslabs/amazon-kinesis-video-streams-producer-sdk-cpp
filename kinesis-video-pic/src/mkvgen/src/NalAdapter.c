/**
 * Helper functionality for NALU adaptation from Annex-B to AVCC format
 */

#define LOG_CLASS "NalAdapter"
#include "Include_i.h"

/**
 * NALU adaptation from Annex-B to AVCC format
 */
STATUS adaptFrameNalsFromAnnexBToAvcc(PBYTE pFrameData, UINT32 frameDataSize, BOOL removeEpb, PBYTE pAdaptedFrameData, PUINT32 pAdaptedFrameDataSize)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i = 0, zeroCount = 0, runSize = 0;
    BOOL markerFound = FALSE;
    PBYTE pCurPnt = pFrameData, pAdaptedCurPnt = pAdaptedFrameData, pRunStart = NULL;

    CHK(pFrameData != NULL && pAdaptedFrameDataSize != NULL, STATUS_NULL_ARG);

    // Validate only when removeEpb flag is set which is the case when we need to split the NALus for
    // CPD processing. For frame adaptation we might have a certain bloat due to bad encoder adaptation flag
    if (removeEpb && pAdaptedFrameData != NULL && HANDLING_TRAILING_NALU_ZERO) {
        CHK(*pAdaptedFrameDataSize >= frameDataSize, STATUS_INVALID_ARG_LEN);
    }

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
        } else if (zeroCount > MAX_ANNEX_B_ZERO_COUNT) {
            // Ensure we don't have over 3 zero's in a row. However, some broken encoders produce zero-es
            // at the end of NALUs which ends up with 4 zeroes
            CHK(FALSE, STATUS_MKV_INVALID_ANNEXB_NALU_IN_FRAME_DATA);
        } else if (*pCurPnt == 0x01 && (zeroCount >= 2 && zeroCount <= MAX_ANNEX_B_ZERO_COUNT)) {
            // Found the Annex-B NAL
            // Check if we have previous run and fix it up
            if (pRunStart != NULL && pAdaptedFrameData != NULL) {
                // Fix-up the previous run by recording the size in Big-Endian format
                PUT_UNALIGNED_BIG_ENDIAN((PINT32) pRunStart, runSize);
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
        } else if (removeEpb && *pCurPnt == 0x03 && zeroCount == 2 && (i < frameDataSize - 1) &&
                   ((*(pCurPnt + 1) == 0x00) || (*(pCurPnt + 1) == 0x01) || (*(pCurPnt + 1) == 0x02) || (*(pCurPnt + 1) == 0x03))) {
            // Removing the EPB
            pAdaptedCurPnt += zeroCount;

            // If the adapted frame is specified then copy the zeros and then the current byte
            if (pAdaptedFrameData != NULL) {
                while (zeroCount != 0) {
                    *(pAdaptedCurPnt - zeroCount) = 0x00;
                    zeroCount--;
                }
            }

            // Reset the zero count
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
            PUT_UNALIGNED_BIG_ENDIAN((PINT32) pRunStart, runSize);
        }

        // Also, handle the case where there is a last 001/0001 at the end of the frame - we will fill with 0s
        if (markerFound) {
            PUT_UNALIGNED_BIG_ENDIAN((PUINT32) pAdaptedCurPnt - 1, 0);
        }
    }

CleanUp:

    if (STATUS_SUCCEEDED(retStatus) && pAdaptedFrameDataSize != NULL) {
        // NOTE: Due to EPB removal we could in fact make the adaptation buffer smaller than the original
        // We will require at least the original size buffer.
        *pAdaptedFrameDataSize = (removeEpb && HANDLING_TRAILING_NALU_ZERO) ? MAX(frameDataSize, (UINT32)(pAdaptedCurPnt - pAdaptedFrameData))
                                                                            : (UINT32)(pAdaptedCurPnt - pAdaptedFrameData);
    }

    return retStatus;
}

/**
 * NALU adaptation from AVCC to Annex-B format
 *
 * NOTE: The adaptation happens in-place
 */
STATUS adaptFrameNalsFromAvccToAnnexB(PBYTE pFrameData, UINT32 frameDataSize)
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

        runLen = (UINT32) GET_UNALIGNED_BIG_ENDIAN((PUINT32) pCurPnt);

        CHK(pCurPnt + runLen <= pEndPnt, STATUS_MKV_INVALID_AVCC_NALU_IN_FRAME_DATA);

        // Adapt with 4 byte version of the start sequence
        PUT_UNALIGNED_BIG_ENDIAN((PINT32) pCurPnt, 0x0001);

        // Jump to the next NAL
        pCurPnt += runLen + SIZEOF(UINT32);
    }

CleanUp:

    return retStatus;
}

/**
 * Codec Private Data NALu adaptation from Annex-B to AVCC format for H264
 *
 * NOTE: We are processing H264 and a single SPS/PPS only for now.
 */
STATUS adaptH264CpdNalsFromAnnexBToAvcc(PBYTE pCpd, UINT32 cpdSize, PBYTE pAdaptedCpd, PUINT32 pAdaptedCpdSize)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 spsSize, ppsSize, adaptedRawSize, adaptedCpdSize = 0;
    PBYTE pAdaptedBits = NULL, pCurPnt = pAdaptedCpd, pSps, pPps;

    CHK(pCpd != NULL && pAdaptedCpdSize != NULL, STATUS_NULL_ARG);

    // We should have at least 2 NALs with at least single byte data so even the short start code should account for 8
    CHK(cpdSize >= MIN_H264_ANNEXB_CPD_SIZE, STATUS_MKV_MIN_ANNEX_B_CPD_SIZE);

    // Calculate the adapted size first. The size should be an overall Avcc package size + the raw sps and pps sizes.
    // The AnnexB packaged sps pps should be the raw size minus the delimiters which can be 2 three bytes start codes.
    adaptedCpdSize = AVCC_CPD_OVERHEAD + cpdSize - 2 * 3;

    // Quick check for size only
    CHK(pAdaptedCpd != NULL, retStatus);

    // Convert the raw bits
    CHK_STATUS(adaptFrameNalsFromAnnexBToAvcc(pCpd, cpdSize, FALSE, NULL, &adaptedRawSize));

    // If we get a larger buffer size then it's an error
    CHK(adaptedRawSize <= *pAdaptedCpdSize, STATUS_BUFFER_TOO_SMALL);

    // Allocate enough storage to store the data temporarily
    pAdaptedBits = (PBYTE) MEMALLOC(adaptedRawSize);
    CHK(pAdaptedBits != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Get the converted bits
    CHK_STATUS(adaptFrameNalsFromAnnexBToAvcc(pCpd, cpdSize, FALSE, pAdaptedBits, &adaptedRawSize));

    // Retrieve the SPS and PPS
    CHK_STATUS(getH264SpsPpsNalusFromAvccNalus(pAdaptedBits, adaptedRawSize, &pSps, &spsSize, &pPps, &ppsSize));
    CHK(pSps != NULL, STATUS_MKV_MISSING_SPS_FROM_H264_CPD);
    CHK(pPps != NULL, STATUS_MKV_MISSING_PPS_FROM_H264_CPD);

    // See if we are still in the buffer
    CHK(spsSize + 8 <= *pAdaptedCpdSize && spsSize + 4 + 4 <= adaptedRawSize, STATUS_MKV_INVALID_ANNEXB_NALU_IN_CPD);

    // Start converting and copying it to the output
    *pCurPnt++ = AVCC_VERSION_CODE; // Version
    *pCurPnt++ = *(pSps + 1);       // Profile
    *pCurPnt++ = *(pSps + 2);       // Compat
    *pCurPnt++ = *(pSps + 3);       // Level

    *pCurPnt++ = AVCC_NALU_LEN_MINUS_ONE;
    *pCurPnt++ = AVCC_NUMBER_OF_SPS_ONE;

    // Write the SPS size in big-endian format
    PUT_UNALIGNED_BIG_ENDIAN((PINT16) pCurPnt, (UINT16) spsSize);
    pCurPnt += SIZEOF(UINT16);

    // Copy the actual bits
    MEMCPY(pCurPnt, pSps, spsSize);
    pCurPnt += spsSize;

    CHK(spsSize + 8 + 1 + ppsSize <= *pAdaptedCpdSize && spsSize + 4 + 4 + ppsSize <= adaptedRawSize, STATUS_MKV_INVALID_ANNEXB_NALU_IN_CPD);

    // Copy the pps size
    *pCurPnt++ = 1; // One pps nal

    // write the size of the pps
    PUT_UNALIGNED_BIG_ENDIAN((PINT16) pCurPnt, (UINT16) ppsSize);
    pCurPnt += SIZEOF(UINT16);

    // Write the PPS data
    MEMCPY(pCurPnt, pPps, ppsSize);
    pCurPnt += ppsSize;

    // Precise adapted size
    adaptedCpdSize = (UINT32)(pCurPnt - pAdaptedCpd);

CleanUp:

    if (pAdaptedCpdSize != NULL) {
        *pAdaptedCpdSize = adaptedCpdSize;
    }

    if (pAdaptedBits != NULL) {
        MEMFREE(pAdaptedBits);
    }

    return retStatus;
}

/**
 * Codec Private Data NALu adaptation from Annex-B to HEVC format for H265
 *
 * NOTE: We are processing H265 and a single VPS/SPS/PPS/SEI only for now.
 * IMPORTANT: We will store a single NALu per type
 */
STATUS adaptH265CpdNalsFromAnnexBToHvcc(PBYTE pCpd, UINT32 cpdSize, PBYTE pAdaptedCpd, PUINT32 pAdaptedCpdSize)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 naluSize, adaptedRawSize, adaptedCpdSize = 0, naluCount = 0, i;
    PBYTE pAdaptedBits = NULL, pCurPnt = pAdaptedCpd, pSrcPnt;
    H265SpsInfo spsInfo;
    UINT32 naluSizes[3];
    PBYTE naluPtrs[3];
    // Nalu types in order - VPS, SPS, PPS
    BYTE naluTypes[3] = {HEVC_VPS_NALU_TYPE, HEVC_SPS_NALU_TYPE, HEVC_PPS_NALU_TYPE};

    CHK(pCpd != NULL && pAdaptedCpdSize != NULL, STATUS_NULL_ARG);

    // We should have at least 3 NALs with at least single byte data so even the short start code should account for 12
    CHK(cpdSize >= MIN_H_265_ANNEXB_CPD_SIZE, STATUS_MKV_MIN_ANNEX_B_CPD_SIZE);

    // Convert the raw bits
    CHK_STATUS(adaptFrameNalsFromAnnexBToAvcc(pCpd, cpdSize, FALSE, NULL, &adaptedRawSize));

    // Allocate enough storage to store the data temporarily
    pAdaptedBits = (PBYTE) MEMALLOC(adaptedRawSize);
    CHK(pAdaptedBits != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Get the converted bits
    CHK_STATUS(adaptFrameNalsFromAnnexBToAvcc(pCpd, cpdSize, FALSE, pAdaptedBits, &adaptedRawSize));

    // Set the source pointer to walk the adapted data
    pSrcPnt = pAdaptedBits;

    // Get the NALu count and store the pointer to SPS
    // It should be VPS/SPS/PPS
    // In some cases the PPS might be missing
    while (naluCount < ARRAY_SIZE(naluTypes) && (UINT32)(pSrcPnt - pAdaptedBits) < adaptedRawSize) {
        CHK(pSrcPnt - pAdaptedBits + SIZEOF(UINT32) <= adaptedRawSize, STATUS_MKV_INVALID_ANNEXB_CPD_NALUS);
        naluSize = (UINT32) GET_UNALIGNED_BIG_ENDIAN((PUINT32) pSrcPnt);

        // Store the NALU pointer
        naluPtrs[naluCount] = pSrcPnt + SIZEOF(UINT32);
        naluSizes[naluCount] = naluSize;

        pSrcPnt += SIZEOF(UINT32) + naluSize;
        CHK((UINT32)(pSrcPnt - pAdaptedBits) <= adaptedRawSize, STATUS_MKV_INVALID_ANNEXB_CPD_NALUS);
        naluCount++;
    }

    CHK(naluCount >= 2, STATUS_MKV_ANNEXB_CPD_MISSING_NALUS);

    // Limit NALus to 3
    naluCount = MIN(3, naluCount);

    // Calculate the required size
    adaptedCpdSize = HEVC_CPD_HEADER_OVERHEAD + naluCount * HEVC_CPD_ENTRY_OVERHEAD;

    // Add the raw size
    for (i = 0; i < naluCount; i++) {
        adaptedCpdSize += naluSizes[i];
    }

    // Quick check for size only
    CHK(pAdaptedCpd != NULL, retStatus);

    // If we get a larger buffer size then it's an error
    CHK(adaptedCpdSize <= *pAdaptedCpdSize, STATUS_BUFFER_TOO_SMALL);

    // Parse the SPS
    CHK_STATUS(parseH265Sps(naluPtrs[1], naluSizes[1], &spsInfo));

    // Start converting and copying it to the output

    // configurationVersion
    *pCurPnt++ = HEVC_CONFIG_VERSION_CODE;

    // general_profile_space, general_tier_flag, general_profile_idc
    *pCurPnt++ = (UINT8)(spsInfo.general_profile_space << 6 | (spsInfo.general_tier_flag & 0x01) << 5 | (spsInfo.general_profile_idc & 0x1f));

    // general_profile_compatibility_flags
    *pCurPnt++ = spsInfo.general_profile_compatibility_flags[0];
    *pCurPnt++ = spsInfo.general_profile_compatibility_flags[1];
    *pCurPnt++ = spsInfo.general_profile_compatibility_flags[2];
    *pCurPnt++ = spsInfo.general_profile_compatibility_flags[3];

    // general_constraint_indicator_flags
    *pCurPnt++ = spsInfo.general_constraint_indicator_flags[0];
    *pCurPnt++ = spsInfo.general_constraint_indicator_flags[1];
    *pCurPnt++ = spsInfo.general_constraint_indicator_flags[2];
    *pCurPnt++ = spsInfo.general_constraint_indicator_flags[3];
    *pCurPnt++ = spsInfo.general_constraint_indicator_flags[4];
    *pCurPnt++ = spsInfo.general_constraint_indicator_flags[5];

    // general_level_idc
    *pCurPnt++ = spsInfo.general_level_idc;

    // min_spatial_segmentation_idc
    *pCurPnt++ = 0xf0;
    *pCurPnt++ = 0x00;

    // parallelismType
    *pCurPnt++ = 0xfc;

    // chroma_format_idc
    *pCurPnt++ = (UINT8)(0xfc | (spsInfo.chroma_format_idc & 0x03));

    // bit_depth_luma_minus8
    *pCurPnt++ = (UINT8)(0xf8 | (spsInfo.bit_depth_luma_minus8 & 0x07));

    // bit_depth_luma_minus8
    *pCurPnt++ = (UINT8)(0xf8 | (spsInfo.bit_depth_chroma_minus8 & 0x07));

    // avgFrameRate
    *pCurPnt++ = 0x00;
    *pCurPnt++ = 0x00;

    // constantFrameRate, numTemporalLayers, temporalIdNested, lengthSizeMinusOne
    *pCurPnt++ = 0x0f;

    // numOfArrays
    *pCurPnt++ = (UINT8) naluCount;

    for (i = 0; i < naluCount; i++) {
        // array_completeness, reserved, NAL_unit_type
        *pCurPnt++ = naluTypes[i];

        // numNalus
        *pCurPnt++ = 0x00;
        *pCurPnt++ = 0x01;

        // nalUnitLength
        PUT_UNALIGNED_BIG_ENDIAN((PINT16) pCurPnt, (UINT16) naluSizes[i]);
        pCurPnt += SIZEOF(UINT16);

        // Write the NALu data
        MEMCPY(pCurPnt, naluPtrs[i], naluSizes[i]);
        pCurPnt += naluSizes[i];
    }

CleanUp:

    if (pAdaptedCpdSize != NULL) {
        *pAdaptedCpdSize = adaptedCpdSize;
    }

    if (pAdaptedBits != NULL) {
        MEMFREE(pAdaptedBits);
    }

    return retStatus;
}
