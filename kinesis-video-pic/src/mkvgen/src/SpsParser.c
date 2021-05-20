/**
 * Helper functionality for SPS parsing
 */

#define LOG_CLASS "SpsParser"

#include "Include_i.h"

/**
 * Extracts the video width and height from the SPS
 */
STATUS getVideoWidthAndHeightFromH264Sps(PBYTE codecPrivateData, UINT32 codecPrivateDataSize, PUINT16 pWidth, PUINT16 pHeight)
{
    STATUS retStatus = STATUS_SUCCESS;
    PBYTE pSps, pPps;
    UINT32 size, ppsSize, adaptedSize;
    BYTE start3ByteCode[] = {0x00, 0x00, 0x01};
    BYTE start4ByteCode[] = {0x00, 0x00, 0x00, 0x01};
    BYTE start5ByteCode[] = {0x00, 0x00, 0x00, 0x00, 0x01};
    PBYTE pAdaptedBits = NULL;

    CHK(codecPrivateData != NULL && pWidth != NULL && pHeight != NULL, STATUS_NULL_ARG);
    CHK(codecPrivateDataSize >= MIN_H264_H265_CPD_SIZE, STATUS_MKV_INVALID_H264_H265_CPD);

    // First of all, we need to determine what format the CPD is in - Annex-B, Avcc or raw
    // NOTE: Some "bad" encoders encode an extra 0 at the end of the NALu resulting in
    // an extra zero interfering with the Annex-B start code so we check for 4 zeroes and 1
    if ((0 == MEMCMP(codecPrivateData, start5ByteCode, SIZEOF(start5ByteCode))) ||
        (0 == MEMCMP(codecPrivateData, start4ByteCode, SIZEOF(start4ByteCode))) ||
        (0 == MEMCMP(codecPrivateData, start3ByteCode, SIZEOF(start3ByteCode)))) {
        // Convert to AvCC format to iterate over NALus easier
        CHK_STATUS(adaptFrameNalsFromAnnexBToAvcc(codecPrivateData, codecPrivateDataSize, FALSE, NULL, &adaptedSize));

        // Allocate enough storage to store the data temporarily
        pAdaptedBits = (PBYTE) MEMALLOC(adaptedSize);
        CHK(pAdaptedBits != NULL, STATUS_NOT_ENOUGH_MEMORY);

        // Get the converted bits
        CHK_STATUS(adaptFrameNalsFromAnnexBToAvcc(codecPrivateData, codecPrivateDataSize, FALSE, pAdaptedBits, &adaptedSize));

        // Retrieve the SPS
        CHK_STATUS(getH264SpsPpsNalusFromAvccNalus(pAdaptedBits, adaptedSize, &pSps, &size, &pPps, &ppsSize));
        CHK(pSps != NULL, STATUS_MKV_INVALID_ANNEXB_CPD_NALUS);
    } else if (codecPrivateData[0] == AVCC_VERSION_CODE && codecPrivateData[4] == AVCC_NALU_LEN_MINUS_ONE &&
               codecPrivateData[5] == AVCC_NUMBER_OF_SPS_ONE) {
        // Avcc encoded sps
        size = (UINT32) GET_UNALIGNED_BIG_ENDIAN((PINT16) &codecPrivateData[6]);
        pSps = codecPrivateData + AVCC_SPS_OFFSET;
    } else {
        // Must be raw SPS
        size = codecPrivateDataSize;
        pSps = codecPrivateData;
    }

    CHK(size <= codecPrivateDataSize, STATUS_MKV_INVALID_H264_H265_CPD);

    CHK_STATUS(parseH264SpsGetResolution(pSps, size, pWidth, pHeight));

CleanUp:

    SAFE_MEMFREE(pAdaptedBits);

    return retStatus;
}

/**
 * Extracts the video width and height from the H265 SPS
 */
STATUS getVideoWidthAndHeightFromH265Sps(PBYTE codecPrivateData, UINT32 codecPrivateDataSize, PUINT16 pWidth, PUINT16 pHeight)
{
    STATUS retStatus = STATUS_SUCCESS;
    PBYTE pSps = codecPrivateData;
    PBYTE pAdaptedBits = NULL;
    UINT32 size = codecPrivateDataSize;
    BYTE start3ByteCode[] = {0x00, 0x00, 0x01};
    BYTE start4ByteCode[] = {0x00, 0x00, 0x00, 0x01};
    BYTE start5ByteCode[] = {0x00, 0x00, 0x00, 0x00, 0x01};
    BYTE naluType;
    UINT16 numNalus, naluIterator, naluLen;
    BOOL spsNaluFound = FALSE;
    UINT32 adaptedSize;

    CHK(codecPrivateData != NULL && pWidth != NULL && pHeight != NULL, STATUS_NULL_ARG);
    CHK(codecPrivateDataSize >= MIN_H264_H265_CPD_SIZE, STATUS_MKV_INVALID_H264_H265_CPD);

    // First of all, we need to determine what format the CPD is in - Annex-B, HEVC or raw
    // NOTE: Some "bad" encoders encode an extra 0 at the end of the NALu resulting in
    // an extra zero interfering with the Annex-B start code so we check for 4 zeroes and 1
    if ((0 == MEMCMP(codecPrivateData, start5ByteCode, SIZEOF(start5ByteCode))) ||
        (0 == MEMCMP(codecPrivateData, start4ByteCode, SIZEOF(start4ByteCode))) ||
        (0 == MEMCMP(codecPrivateData, start3ByteCode, SIZEOF(start3ByteCode)))) {
        // 3 or 4-byte Annex-B start code.
        // Assume this is VPS/SPS/PPS format. Extract the second NALu
        // Convert the raw bits
        CHK_STATUS(adaptFrameNalsFromAnnexBToAvcc(pSps, size, FALSE, NULL, &adaptedSize));

        // Allocate enough storage to store the data temporarily
        pAdaptedBits = (PBYTE) MEMALLOC(adaptedSize);
        CHK(pAdaptedBits != NULL, STATUS_NOT_ENOUGH_MEMORY);

        // Get the converted bits
        CHK_STATUS(adaptFrameNalsFromAnnexBToAvcc(pSps, size, FALSE, pAdaptedBits, &adaptedSize));

        // Retrieve the SPS
        CHK_STATUS(getH265SpsNalusFromAvccNalus(pAdaptedBits, adaptedSize, &pSps, &size));
        CHK(pSps != NULL, STATUS_MKV_INVALID_ANNEXB_CPD_NALUS);
    } else {
        // Check for the HEVC format
        if (checkHevcFormatHeader(codecPrivateData, codecPrivateDataSize)) {
            pSps = codecPrivateData + HEVC_CPD_HEADER_SIZE;
            size = codecPrivateDataSize - HEVC_CPD_HEADER_SIZE;
        }

        // Process as raw
        CHK(size <= codecPrivateDataSize, STATUS_MKV_INVALID_HEVC_FORMAT);

        // Iterate over the raw array and extract the SPS
        while (size > HEVC_NALU_ARRAY_ENTRY_SIZE) {
            naluType = (BYTE)(pSps[0] & 0x3f);
            numNalus = (UINT16) GET_UNALIGNED_BIG_ENDIAN((PINT16) &pSps[1]);
            pSps += HEVC_NALU_ARRAY_ENTRY_SIZE;
            size -= HEVC_NALU_ARRAY_ENTRY_SIZE;

            if (HEVC_SPS_NALU_TYPE == naluType) {
                CHK(numNalus == 1, STATUS_MKV_INVALID_HEVC_NALU_COUNT);
                spsNaluFound = TRUE;
                break;
            }

            for (naluIterator = 0; naluIterator < numNalus; naluIterator++) {
                CHK(size > SIZEOF(UINT16), STATUS_MKV_INVALID_HEVC_FORMAT);
                naluLen = (UINT16) GET_UNALIGNED_BIG_ENDIAN((PINT16) pSps);
                size -= SIZEOF(UINT16);
                pSps += SIZEOF(UINT16);

                CHK(size >= naluLen, STATUS_MKV_INVALID_HEVC_FORMAT);
                size -= naluLen;
                pSps += naluLen;
            }
        }

        // We should have the SPS nalu and it's the only entry
        CHK(spsNaluFound, STATUS_MKV_HEVC_SPS_NALU_MISSING);

        CHK(size > SIZEOF(UINT16), STATUS_MKV_INVALID_HEVC_FORMAT);
        naluLen = (UINT16) GET_UNALIGNED_BIG_ENDIAN((PINT16) pSps);
        size -= SIZEOF(UINT16);
        pSps += SIZEOF(UINT16);

        CHK(size >= naluLen, STATUS_MKV_INVALID_HEVC_SPS_NALU_SIZE);

        // Set the size to the nalu length only
        size = naluLen;
    }

    CHK_STATUS(parseH265SpsGetResolution(pSps, size, pWidth, pHeight));

CleanUp:

    SAFE_MEMFREE(pAdaptedBits);

    return retStatus;
}

/**
 * Returns whether the CPD has HEVC header format
 */
BOOL checkHevcFormatHeader(PBYTE codecPrivateData, UINT32 codecPrivateDataSize)
{
    BOOL hevc = TRUE;
    if (codecPrivateData == NULL || codecPrivateDataSize <= HEVC_CPD_HEADER_SIZE || codecPrivateData[0] != 1 ||
        (codecPrivateData[13] & 0xf0) != 0xf0 || (codecPrivateData[15] & 0xfc) != 0xfc || (codecPrivateData[16] & 0xfc) != 0xfc ||
        (codecPrivateData[17] & 0xf8) != 0xf8 || (codecPrivateData[18] & 0xf8) != 0xf8) {
        hevc = FALSE;
    }

    return hevc;
}

/**
 * Extracts the video width and height from the BitmapInfoHeader
 */
STATUS getVideoWidthAndHeightFromBih(PBYTE codecPrivateData, UINT32 codecPrivateDataSize, PUINT16 pWidth, PUINT16 pHeight)
{
    STATUS retStatus = STATUS_SUCCESS;

    CHK(codecPrivateData != NULL && pWidth != NULL && pHeight != NULL, STATUS_NULL_ARG);
    CHK(codecPrivateDataSize >= BITMAP_INFO_HEADER_SIZE, STATUS_MKV_INVALID_BIH_CPD);

    // NOTE: Bitmap info header structure defines the data in a LITTLE-ENDIAN format. :(
    if (isBigEndian()) {
        *pWidth = (UINT16) SWAP_INT32(*((PINT32) codecPrivateData + 1));
        *pHeight = (UINT16) SWAP_INT32(*((PINT32) codecPrivateData + 2));
    } else {
        *pWidth = (UINT16) * ((PINT32) codecPrivateData + 1);
        *pHeight = (UINT16) * ((PINT32) codecPrivateData + 2);
    }

CleanUp:

    return retStatus;
}

STATUS parseH264SpsGetResolution(PBYTE pSps, UINT32 spsSize, PUINT16 pWidth, PUINT16 pHeight)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 frameCropLeftOffset = 0, frameCropRightOffset = 0, frameCropTopOffset = 0, frameCropBottomOffset = 0, picWidthInMbsMinus1 = 0,
           frameMbsOnlyFlag = 0, picHeightInMapUnitsMinus1 = 0, profileIdc = 0, sizeOfScalingList = 0, lastScale = 8, nextScale = 8,
           picOrderCntType = 0, numRefFramesInPicOrderCntCycle = 0, deltaScale = 0, chromaFormatIdc = 1, separateColourPlaneFlag = 0;
    UINT32 i, j, read;
    INT32 readInt, width, height, pixelWidth, pixelHeight, subWidthC, subHeightC, arrayWidth, cropUnitX, cropUnitY;

    BitReader bitReader;

    CHK(pSps != NULL && pWidth != NULL && pHeight != NULL, STATUS_NULL_ARG);
    CHK(spsSize != 0, STATUS_INVALID_ARG_LEN);

    // Create a bit reader on top of the SPS
    CHK_STATUS(bitReaderReset(&bitReader, pSps, spsSize * 8));

    // Read the SPS Nalu
    CHK_STATUS(bitReaderReadBits(&bitReader, 8, &read));
    CHK(((read & 0x80) == 0) && ((read & 0x60) != 0) && ((read & 0x1f) == SPS_NALU_TYPE), STATUS_MKV_INVALID_H264_H265_SPS_NALU);

    // Get the profile
    CHK_STATUS(bitReaderReadBits(&bitReader, 8, &profileIdc));

    // Skip the constraint set flags 0 - 5, the reserved 2 zero bits
    // 8 bits level_idc and after that read the exp golomb for seq_parameter_set_id
    CHK_STATUS(bitReaderReadBits(&bitReader, 6 + 2 + 8, &read));

    // Read exp golomb for seq_parameter_set_id
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));

    if (profileIdc == 100 || profileIdc == 110 || profileIdc == 122 || profileIdc == 244 || profileIdc == 44 || profileIdc == 83 ||
        profileIdc == 86 || profileIdc == 118 || profileIdc == 128 || profileIdc == 138) {
        // Read chroma_format_idc
        CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &chromaFormatIdc));

        if (chromaFormatIdc == 3) {
            // Read residual_colour_transform_flag
            CHK_STATUS(bitReaderReadBit(&bitReader, &separateColourPlaneFlag));
        }

        // Read bit_depth_luma_minus8
        CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));

        // Read bit_depth_chroma_minus8
        CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));

        // Read qpprime_y_zero_transform_bypass_flag
        CHK_STATUS(bitReaderReadBit(&bitReader, &read));

        // Read seq_scaling_matrix_present_flag
        CHK_STATUS(bitReaderReadBit(&bitReader, &read));

        if (read != 0) {
            for (i = 0; i < 8; i++) {
                // Read seq_scaling_list_present_flag
                CHK_STATUS(bitReaderReadBit(&bitReader, &read));

                if (read != 0) {
                    sizeOfScalingList = (i < 6) ? 16 : 64;
                    for (j = 0; j < sizeOfScalingList; j++) {
                        if (nextScale != 0) {
                            // Read delta scale
                            CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &deltaScale));
                            nextScale = (lastScale + deltaScale + 256) % 256;
                        }

                        lastScale = (nextScale == 0) ? lastScale : nextScale;
                    }
                }
            }
        }
    }

    // Read log2_max_frame_num_minus4
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));

    // Read pic_order_cnt_type
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &picOrderCntType));

    if (picOrderCntType == 0) {
        // Read log2_max_pic_order_cnt_lsb_minus4
        CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));
    } else if (picOrderCntType == 1) {
        // Read delta_pic_order_always_zero_flag
        CHK_STATUS(bitReaderReadBit(&bitReader, &read));

        // Read offset_for_non_ref_pic
        CHK_STATUS(bitReaderReadExpGolombSe(&bitReader, &readInt));

        // Read offset_for_top_to_bottom_field
        CHK_STATUS(bitReaderReadExpGolombSe(&bitReader, &readInt));

        // Read bit_depth_luma_minus8
        CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &numRefFramesInPicOrderCntCycle));

        for (i = 0; i < numRefFramesInPicOrderCntCycle; i++) {
            // Read offset_for_ref_frame
            CHK_STATUS(bitReaderReadExpGolombSe(&bitReader, &readInt));
        }
    }

    // Read max_num_ref_frames
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));

    // Read gaps_in_frame_num_value_allowed_flag
    CHK_STATUS(bitReaderReadBit(&bitReader, &read));

    // Read pic_width_in_mbs_minus1
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &picWidthInMbsMinus1));

    // Read pic_height_in_map_units_minus1
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &picHeightInMapUnitsMinus1));

    // Read frame_mbs_only_flag
    CHK_STATUS(bitReaderReadBit(&bitReader, &frameMbsOnlyFlag));

    if (frameMbsOnlyFlag == 0) {
        // Read mb_adaptive_frame_field_flag
        CHK_STATUS(bitReaderReadBit(&bitReader, &read));
    }

    // Read direct_8x8_inference_flag
    CHK_STATUS(bitReaderReadBit(&bitReader, &read));

    // Read frame_cropping_flag
    CHK_STATUS(bitReaderReadBit(&bitReader, &read));

    if (read != 0) {
        // Read frame_crop_left_offset
        CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &frameCropLeftOffset));

        // Read frame_crop_right_offset
        CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &frameCropRightOffset));

        // Read frame_crop_top_offset
        CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &frameCropTopOffset));

        // Read frame_crop_bottom_offset
        CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &frameCropBottomOffset));
    }

    // Read vui_parameters_present_flag
    CHK_STATUS(bitReaderReadBit(&bitReader, &read));

    // Proper width and height extraction is defined in part in
    // 7.3.2.1.1 for SPS syntax: https://www.itu.int/rec/T-REC-H.264-201304-S/en
    arrayWidth = frameMbsOnlyFlag != 0 ? 1 : 2;
    pixelWidth = (picWidthInMbsMinus1 + 1) * 16;
    pixelHeight = (picHeightInMapUnitsMinus1 + 1) * 16 * arrayWidth;

    switch (chromaFormatIdc) {
        case 1:
            subWidthC = 2;
            subHeightC = 2;
            break;

        case 2:
            subWidthC = 2;
            subHeightC = 1;
            break;

        default:
            subWidthC = 1;
            subHeightC = 1;
    }

    cropUnitX = subWidthC;
    cropUnitY = subHeightC * arrayWidth;

    width = pixelWidth - frameCropLeftOffset * cropUnitX - frameCropRightOffset * cropUnitX;
    height = pixelHeight - frameCropTopOffset * cropUnitY - frameCropBottomOffset * cropUnitY;

    CHK(width >= 0 && width <= MAX_UINT16, STATUS_MKV_INVALID_H264_H265_SPS_WIDTH);
    CHK(height >= 0 && height <= MAX_UINT16, STATUS_MKV_INVALID_H264_H265_SPS_HEIGHT);

    *pWidth = (UINT16) width;
    *pHeight = (UINT16) height;

CleanUp:

    return retStatus;
}

STATUS parseH265SpsGetResolution(PBYTE pSps, UINT32 spsSize, PUINT16 pWidth, PUINT16 pHeight)
{
    STATUS retStatus = STATUS_SUCCESS;
    H265SpsInfo spsInfo;

    CHK(pSps != NULL && pWidth != NULL && pHeight != NULL, STATUS_NULL_ARG);
    CHK(spsSize != 0, STATUS_INVALID_ARG_LEN);

    // Parse the sps first
    CHK_STATUS(parseH265Sps(pSps, spsSize, &spsInfo));

    // Extract the resolution
    CHK_STATUS(extractResolutionFromH265SpsInfo(&spsInfo, pWidth, pHeight));

CleanUp:

    return retStatus;
}

STATUS parseH265Sps(PBYTE pSps, UINT32 spsSize, PH265SpsInfo pSpsInfo)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i, read, sps_sub_layer_ordering_info_present_flag, scaling_list_enabled_flag, sps_scaling_list_data_present_flag, pcm_enabled_flag,
        num_short_term_ref_pic_sets, log2_max_pic_order_cnt_lsb_minus4;

    BitReader bitReader;
    UINT32 adaptedSize = spsSize;
    PBYTE pAdaptedBits = NULL;
    BOOL gotSpsData = FALSE;

    CHK(pSps != NULL && pSpsInfo != NULL, STATUS_NULL_ARG);
    CHK(spsSize != 0, STATUS_INVALID_ARG_LEN);

    MEMSET(pSpsInfo, 0x00, SIZEOF(H265SpsInfo));

    // Adapt the sps to get rid of the EPB - NOTE: the existing size should be greater than the adapted so
    // we don't need to run the function to extract the adapted size first
    // Allocate enough storage to store the data temporarily
    pAdaptedBits = (PBYTE) MEMALLOC(spsSize);
    CHK(pAdaptedBits != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Get the converted bits
    CHK_STATUS(adaptFrameNalsFromAnnexBToAvcc(pSps, spsSize, TRUE, pAdaptedBits, &adaptedSize));

    // Create a bit reader on top of the SPS
    CHK_STATUS(bitReaderReset(&bitReader, pAdaptedBits, adaptedSize * 8));

    // Read the sps_video_parameter_set_id
    CHK_STATUS(bitReaderReadBits(&bitReader, 4, &read));

    // Read the sps_max_sub_layers_minus1 - NOTE: As it's 3 bits only it cant be larger than 8
    CHK_STATUS(bitReaderReadBits(&bitReader, 3, &read));
    pSpsInfo->max_sub_layers_minus1 = (UINT8) read;

    // Read the sps_temporal_id_nesting_flag
    CHK_STATUS(bitReaderReadBits(&bitReader, 1, &read));

    // profile tier level
    CHK_STATUS(parseProfileTierLevel(&bitReader, pSpsInfo));

    // Read the sps_seq_parameter_set_id
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));

    // Read the chroma_format_idc
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));
    pSpsInfo->chroma_format_idc = (UINT8) read;

    if (pSpsInfo->chroma_format_idc == 3) {
        // Read the separate_colour_plane_flag
        CHK_STATUS(bitReaderReadBits(&bitReader, 1, &read));
    }

    // Read the pic_width_in_luma_samples
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));
    pSpsInfo->pic_width_in_luma_samples = (UINT16) read;

    // Read the pic_height_in_luma_samples
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));
    pSpsInfo->pic_height_in_luma_samples = (UINT16) read;

    // Read the conformance_window_flag
    CHK_STATUS(bitReaderReadBits(&bitReader, 1, &read));
    pSpsInfo->conformance_window_flag = (UINT8) read;

    if (pSpsInfo->conformance_window_flag != 0) {
        // Read the conf_win_left_offset
        CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &pSpsInfo->conf_win_left_offset));

        // Read the conf_win_right_offset
        CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &pSpsInfo->conf_win_right_offset));

        // Read the conf_win_top_offset
        CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &pSpsInfo->conf_win_top_offset));

        // Read the conf_win_bottom_offset
        CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &pSpsInfo->conf_win_bottom_offset));
    }

    // Read the bit_depth_luma_minus8
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));
    pSpsInfo->bit_depth_luma_minus8 = (UINT8) read;

    // Read the bit_depth_chroma_minus8
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));
    pSpsInfo->bit_depth_chroma_minus8 = (UINT8) read;

    // At this stage we have the sps data we need. The parser will break on some data
    // so at this stage we can return success and return the data we extracted.
    // This seems to come from an older version of the HEVC.
    gotSpsData = TRUE;

    // Read the log2_max_pic_order_cnt_lsb_minus4
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &log2_max_pic_order_cnt_lsb_minus4));

    // Read the sps_sub_layer_ordering_info_present_flag
    CHK_STATUS(bitReaderReadBits(&bitReader, 1, &sps_sub_layer_ordering_info_present_flag));

    i = (sps_sub_layer_ordering_info_present_flag != 0) ? 0 : pSpsInfo->max_sub_layers_minus1;
    for (; i <= pSpsInfo->max_sub_layers_minus1; i++) {
        // Read the sps_max_dec_pic_buffering_minus1[i]
        CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));

        // Read the sps_max_num_reorder_pics[i]
        CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));

        // Read the sps_max_latency_increase_plus1[i]
        CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));
    }

    // Read the log2_min_luma_coding_block_size_minus3
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));

    // Read the log2_diff_max_min_luma_coding_block_size
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));

    // Read the log2_min_transform_block_size_minus2
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));

    // Read the log2_diff_max_min_transform_block_size
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));

    // Read the max_transform_hierarchy_depth_inter
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));

    // Read the max_transform_hierarchy_depth_intra
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));

    // Read the scaling_list_enabled_flag
    CHK_STATUS(bitReaderReadBits(&bitReader, 1, &scaling_list_enabled_flag));

    if (scaling_list_enabled_flag != 0) {
        // Read the sps_scaling_list_data_present_flag
        CHK_STATUS(bitReaderReadBits(&bitReader, 1, &sps_scaling_list_data_present_flag));
        if (sps_scaling_list_data_present_flag != 0) {
            // Process scaling lists
            CHK_STATUS(parseScalingListData(&bitReader));
        }
    }

    // Read the amp_enabled_flag
    CHK_STATUS(bitReaderReadBits(&bitReader, 1, &read));

    // Read the sample_adaptive_offset_enabled_flag
    CHK_STATUS(bitReaderReadBits(&bitReader, 1, &read));

    // Read the pcm_enabled_flag
    CHK_STATUS(bitReaderReadBits(&bitReader, 1, &pcm_enabled_flag));

    if (pcm_enabled_flag != 0) {
        // Read the pcm_sample_bit_depth_luma_minus1
        CHK_STATUS(bitReaderReadBits(&bitReader, 4, &read));

        // Read the pcm_sample_bit_depth_chroma_minus1
        CHK_STATUS(bitReaderReadBits(&bitReader, 4, &read));

        // Read the log2_min_pcm_luma_coding_block_size_minus3
        CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));

        // Read the log2_diff_max_min_pcm_luma_coding_block_size
        CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));

        // Read the pcm_loop_filter_disabled_flag
        CHK_STATUS(bitReaderReadBits(&bitReader, 1, &read));
    }

    // Read the num_short_term_ref_pic_sets
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &num_short_term_ref_pic_sets));
    for (i = 0; i < num_short_term_ref_pic_sets; i++) {
        // short_term_ref_pic_set()
        // TODO: Add proper processing for the rest but for now we already have the information we need
    }

CleanUp:

    if (pAdaptedBits != NULL) {
        MEMFREE(pAdaptedBits);
    }

    if (gotSpsData) {
        // If we got the data we need then we can reset the status
        retStatus = STATUS_SUCCESS;
    }

    return retStatus;
}

STATUS parseProfileTierLevel(PBitReader pBitReader, PH265SpsInfo pSpsInfo)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i, read;
    BOOL subLayerProfilePresentFlags[MAX_H265_SPS_SUB_LAYERS];
    BOOL subLayerLevelPresentFlags[MAX_H265_SPS_SUB_LAYERS];

    CHK(pBitReader != NULL && pSpsInfo != NULL, STATUS_NULL_ARG);

    // Read the general_profile_space
    CHK_STATUS(bitReaderReadBits(pBitReader, 2, &read));
    pSpsInfo->general_profile_space = (UINT8) read;

    // Read the general_tier_flag
    CHK_STATUS(bitReaderReadBits(pBitReader, 1, &read));
    pSpsInfo->general_tier_flag = (UINT8) read;

    // Read the general_profile_idc
    CHK_STATUS(bitReaderReadBits(pBitReader, 5, &read));
    pSpsInfo->general_profile_idc = (UINT8) read;

    // Read the general_profile_compatibility_flags[i] - 32 bits
    CHK_STATUS(bitReaderReadBits(pBitReader, 8, &read));
    pSpsInfo->general_profile_compatibility_flags[0] = (UINT8) read;
    CHK_STATUS(bitReaderReadBits(pBitReader, 8, &read));
    pSpsInfo->general_profile_compatibility_flags[1] = (UINT8) read;
    CHK_STATUS(bitReaderReadBits(pBitReader, 8, &read));
    pSpsInfo->general_profile_compatibility_flags[2] = (UINT8) read;
    CHK_STATUS(bitReaderReadBits(pBitReader, 8, &read));
    pSpsInfo->general_profile_compatibility_flags[3] = (UINT8) read;

    // Read the general_progressive_source_flag
    // Read the general_interlaced_source_flag
    // Read the general_non_packed_constraint_flag
    // Read the general_frame_only_constraint_flag
    // Read the max_12bit_constraint_flag
    // Read the max_10bit_constraint_flag
    // Read the max_8bit_constraint_flag
    // Read the max_422chroma_constraint_flag
    // Read the max_420chroma_constraint_flag
    // Read the max_monochrome_constraint_flag
    // Read the intra_constraint_flag
    // Read the one_picture_only_constraint_flag
    // Read the lower_bit_rate_constraint_flag
    // Read the max_14bit_constraint_flag
    // Skip the reserved zero bits 34
    CHK_STATUS(bitReaderReadBits(pBitReader, 8, &read));
    pSpsInfo->general_constraint_indicator_flags[0] = (UINT8) read;
    CHK_STATUS(bitReaderReadBits(pBitReader, 8, &read));
    pSpsInfo->general_constraint_indicator_flags[1] = (UINT8) read;
    CHK_STATUS(bitReaderReadBits(pBitReader, 8, &read));
    pSpsInfo->general_constraint_indicator_flags[2] = (UINT8) read;
    CHK_STATUS(bitReaderReadBits(pBitReader, 8, &read));
    pSpsInfo->general_constraint_indicator_flags[3] = (UINT8) read;
    CHK_STATUS(bitReaderReadBits(pBitReader, 8, &read));
    pSpsInfo->general_constraint_indicator_flags[4] = (UINT8) read;
    CHK_STATUS(bitReaderReadBits(pBitReader, 8, &read));
    pSpsInfo->general_constraint_indicator_flags[5] = (UINT8) read;

    // Read the general_level_idc
    CHK_STATUS(bitReaderReadBits(pBitReader, 8, &read));
    pSpsInfo->general_level_idc = (UINT8) read;

    for (i = 0; i < pSpsInfo->max_sub_layers_minus1; i++) {
        // Read the sub_layer_profile_present_flag[i]
        CHK_STATUS(bitReaderReadBits(pBitReader, 1, &read));
        subLayerProfilePresentFlags[i] = (read != 0);

        // Read the sub_layer_level_present_flag[i]
        CHK_STATUS(bitReaderReadBits(pBitReader, 1, &read));
        subLayerLevelPresentFlags[i] = (read != 0);
    }

    if (pSpsInfo->max_sub_layers_minus1 > 0) {
        for (i = pSpsInfo->max_sub_layers_minus1; i < 8; i++) {
            // Read the reserved_zero_2bits[i]
            CHK_STATUS(bitReaderReadBits(pBitReader, 2, &read));
            // CHK(0 == read, STATUS_MKV_INVALID_HEVC_SPS_RESERVED);
        }
    }

    for (i = 0; i < pSpsInfo->max_sub_layers_minus1; i++) {
        if (subLayerProfilePresentFlags[i]) {
            // Read the sub_layer_profile_space[i]
            CHK_STATUS(bitReaderReadBits(pBitReader, 2, &read));

            // Read the sub_layer_tier_flag[i]
            CHK_STATUS(bitReaderReadBits(pBitReader, 1, &read));

            // Read the sub_layer_profile_idc[i]
            CHK_STATUS(bitReaderReadBits(pBitReader, 5, &read));

            // Read the sub_layer_profile_compatibility_flag[i][32]
            CHK_STATUS(bitReaderReadBits(pBitReader, 32, &read));

            // Read the sub_layer_progressive_source_flag[i]
            CHK_STATUS(bitReaderReadBits(pBitReader, 1, &read));

            // Read the sub_layer_interlaced_source_flag[i]
            CHK_STATUS(bitReaderReadBits(pBitReader, 1, &read));

            // Read the sub_layer_non_packed_constraint_flag[i]
            CHK_STATUS(bitReaderReadBits(pBitReader, 1, &read));

            // Read the sub_layer_frame_only_constraint_flag[i]
            CHK_STATUS(bitReaderReadBits(pBitReader, 1, &read));

            // Read the sub_layer_reserved_zero_44bits[i]
            CHK_STATUS(bitReaderReadBits(pBitReader, 32, &read));
            CHK(0 == read, STATUS_MKV_INVALID_HEVC_SPS_RESERVED);
            CHK_STATUS(bitReaderReadBits(pBitReader, 12, &read));
            CHK(0 == read, STATUS_MKV_INVALID_HEVC_SPS_RESERVED);
        }

        if (subLayerLevelPresentFlags[i]) {
            // Read the sub_layer_level_idc[i]
            CHK_STATUS(bitReaderReadBits(pBitReader, 8, &read));
        }
    }

CleanUp:

    return retStatus;
}

STATUS parseScalingListData(PBitReader pBitReader)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i, read, sizeId, matrixId, coefNum;
    INT32 iread;

    CHK(pBitReader != NULL, STATUS_NULL_ARG);

    for (sizeId = 0; sizeId < 4; sizeId++) {
        for (matrixId = 0; matrixId < (UINT32)(((sizeId == 3) ? 2 : 6)); matrixId++) {
            // Read scaling_list_pred_mode_flag[sizeId][matrixId]
            CHK_STATUS(bitReaderReadBits(pBitReader, 1, &read));

            if (read == 0) {
                // Read the scaling_list_pred_matrix_id_delta[sizeId][matrixId]
                CHK_STATUS(bitReaderReadExpGolomb(pBitReader, &read));
            } else {
                coefNum = (UINT32) MIN(64, (1 << (4 + (sizeId << 1))));

                if (sizeId > 1) {
                    // Read the scaling_list_dc_coef_minus8[sizeId − 2][matrixId]
                    CHK_STATUS(bitReaderReadExpGolombSe(pBitReader, &iread));
                }

                for (i = 0; i < coefNum; i++) {
                    // Read the scaling_list_delta_coef[sizeId][matrixId][i]
                    CHK_STATUS(bitReaderReadExpGolombSe(pBitReader, &iread));
                }
            }
        }
    }

CleanUp:

    return retStatus;
}

STATUS extractResolutionFromH265SpsInfo(PH265SpsInfo pSpsInfo, PUINT16 pWidth, PUINT16 pHeight)
{
    STATUS retStatus = STATUS_SUCCESS;
    INT32 crop_x = 0, crop_y = 0, sub_width_c = 0, sub_height_c = 0;

    CHK(pSpsInfo != NULL && pWidth != NULL && pHeight != NULL, STATUS_NULL_ARG);

    if (pSpsInfo->conformance_window_flag != 0) {
        switch (pSpsInfo->chroma_format_idc) {
            case 0:
                // Monochrome
                sub_width_c = 1;
                sub_height_c = 1;
                break;

            case 1:
                // 4:2:0
                sub_width_c = 2;
                sub_height_c = 2;
                break;

            case 2:
                // 4:2:2
                sub_width_c = 2;
                sub_height_c = 1;
                break;

            case 3:
                // 4:4:4
                sub_width_c = 1;
                sub_height_c = 1;
                break;

            default:
                CHK(FALSE, STATUS_MKV_INVALID_HEVC_SPS_CHROMA_FORMAT_IDC);
        }

        // Formula D-28, D-29
        crop_x = sub_width_c * (pSpsInfo->conf_win_right_offset + pSpsInfo->conf_win_left_offset);
        crop_y = sub_height_c * (pSpsInfo->conf_win_bottom_offset + pSpsInfo->conf_win_top_offset);
    }

    *pWidth = (UINT16)(pSpsInfo->pic_width_in_luma_samples - crop_x);
    *pHeight = (UINT16)(pSpsInfo->pic_height_in_luma_samples - crop_y);

CleanUp:

    return retStatus;
}

STATUS getH264SpsPpsNalusFromAvccNalus(PBYTE pAvccNalus, UINT32 nalusSize, PBYTE* ppSps, PUINT32 pSpsSize, PBYTE* ppPps, PUINT32 pPpsSize)
{
    STATUS retStatus = STATUS_SUCCESS;
    PBYTE pSps = NULL, pPps = NULL;
    UINT32 spsSize = 0, ppsSize = 0, runLen;
    PUINT32 pCurPtr = (PUINT32) pAvccNalus;
    BYTE naluHeader;
    BOOL iterate = TRUE;

    CHK(pAvccNalus != NULL && ppSps != NULL && pSpsSize != NULL && ppPps != NULL && pPpsSize != NULL, STATUS_NULL_ARG);

    while (iterate && (PBYTE)(pCurPtr + 1) < pAvccNalus + nalusSize) {
        runLen = GET_UNALIGNED_BIG_ENDIAN(pCurPtr);
        naluHeader = *(PBYTE)(pCurPtr + 1);

        // Check for the SPS header
        if (pSps == NULL && (naluHeader & 0x80) == 0 && (naluHeader & 0x60) != 0 && (naluHeader & 0x1f) == SPS_NALU_TYPE) {
            pSps = (PBYTE)(pCurPtr + 1);
            spsSize = runLen;
        }

        // Check for the PPS header
        if (pPps == NULL && (naluHeader & 0x80) == 0 && (naluHeader & 0x60) != 0 && (naluHeader & 0x1f) == PPS_NALU_TYPE) {
            pPps = (PBYTE)(pCurPtr + 1);
            ppsSize = runLen;
        }

        iterate = (pSps == NULL || pPps == NULL);

        // Advance the current ptr taking into account the 4 byte length and the size of the run
        pCurPtr = (PUINT32)((PBYTE)(pCurPtr + 1) + runLen);
    }

    *ppSps = pSps;
    *pSpsSize = spsSize;
    *ppPps = pPps;
    *pPpsSize = ppsSize;

CleanUp:

    return retStatus;
}

STATUS getH265SpsNalusFromAvccNalus(PBYTE pAvccNalus, UINT32 nalusSize, PBYTE* ppSps, PUINT32 pSpsSize)
{
    STATUS retStatus = STATUS_SUCCESS;
    PBYTE pSps = NULL;
    UINT32 spsSize = 0, runLen;
    PUINT32 pCurPtr = (PUINT32) pAvccNalus;
    BYTE naluHeader;
    BOOL iterate = TRUE;

    CHK(pAvccNalus != NULL && ppSps != NULL && pSpsSize != NULL, STATUS_NULL_ARG);

    while (iterate && (PBYTE)(pCurPtr + 1) < pAvccNalus + nalusSize) {
        runLen = GET_UNALIGNED_BIG_ENDIAN(pCurPtr);
        naluHeader = *(PBYTE)(pCurPtr + 1);

        // Check for the SPS header
        if (pSps == NULL && (naluHeader >> 1) == HEVC_SPS_NALU_TYPE) {
            pSps = (PBYTE)(pCurPtr + 1);
            spsSize = runLen;
        }

        iterate = (pSps == NULL);

        // Advance the current ptr taking into account the 4 byte length and the size of the run
        pCurPtr = (PUINT32)((PBYTE)(pCurPtr + 1) + runLen);
    }

    *ppSps = pSps;
    *pSpsSize = spsSize;

CleanUp:

    return retStatus;
}
