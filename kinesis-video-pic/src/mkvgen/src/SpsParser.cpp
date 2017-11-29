/**
 * Helper functionality for SPS parsing
 */

#define LOG_CLASS "SpsParser"

#include "Include_i.h"
/**
 * Extracts the video width and height from the SPS
 */
STATUS getVideoWidthAndHeightFromSps(PBYTE codecPrivateData, UINT32 codecPrivateDataSize, PUINT16 pWidth,
                                     PUINT16 pHeight)
{
    STATUS retStatus = STATUS_SUCCESS;
    PBYTE pSps;
    UINT32 size;
    BYTE start3ByteCode[] = {0x00, 0x00, 0x01};
    BYTE start4ByteCode[] = {0x00, 0x00, 0x00, 0x01};

    CHK(codecPrivateData != NULL && pWidth != NULL && pHeight != NULL, STATUS_NULL_ARG);
    CHK(codecPrivateDataSize >= MIN_H264_H265_CPD_SIZE, STATUS_MKV_INVALID_H264_H265_CPD);

    // First of all, we need to determine what format the CPD is in - Annex-B, Avcc or raw
    if (0 == MEMCMP(codecPrivateData, start4ByteCode, SIZEOF(start4ByteCode))) {
        // 4-byte Annex-B start code
        pSps = codecPrivateData + SIZEOF(start4ByteCode);
        size = codecPrivateDataSize - SIZEOF(start4ByteCode);
    } else if (0 == MEMCMP(codecPrivateData, start3ByteCode, SIZEOF(start3ByteCode))) {
        // 3-byte Annex-B start code
        pSps = codecPrivateData + SIZEOF(start3ByteCode);
        size = codecPrivateDataSize - SIZEOF(start3ByteCode);
    } else if (codecPrivateData[0] == AVCC_VERSION_CODE
               && codecPrivateData[4] == AVCC_NALU_LEN_MINUS_ONE
               && codecPrivateData[5] == AVCC_NUMBER_OF_SPS_ONE) {
        // Avcc encoded sps
        size = getInt16(*(PINT16) &codecPrivateData[6]);
        pSps = codecPrivateData + AVCC_SPS_OFFSET;
    } else {
        // Must be raw SPS
        size = codecPrivateDataSize;
        pSps = codecPrivateData;
    }

    CHK(size <= codecPrivateDataSize, STATUS_MKV_INVALID_H264_H265_CPD);

    CHK_STATUS(parseSpsGetResolution(pSps, size, pWidth, pHeight));

CleanUp:

    return retStatus;
}

/**
 * Extracts the video width and height from the BitmapInfoHeader
 */
STATUS getVideoWidthAndHeightFromBih(PBYTE codecPrivateData, UINT32 codecPrivateDataSize, PUINT16 pWidth,
                                     PUINT16 pHeight)
{
    STATUS retStatus = STATUS_SUCCESS;

    CHK(codecPrivateData != NULL && pWidth != NULL && pHeight != NULL, STATUS_NULL_ARG);
    CHK(codecPrivateDataSize >= BITMAP_INFO_HEADER_SIZE, STATUS_MKV_INVALID_BIH_CPD);

    // NOTE: Bitmap info header structure defines the data in a LITTLE-ENDIAN format. :(
    if (isBigEndian()) {
        *pWidth = SWAP_INT32(*((PINT32)codecPrivateData + 1));
        *pHeight = SWAP_INT32(*((PINT32)codecPrivateData + 2));
    } else {
        *pWidth = *((PINT32)codecPrivateData + 1);
        *pHeight = *((PINT32)codecPrivateData + 2);
    }

CleanUp:

    return retStatus;
}

STATUS parseSpsGetResolution(PBYTE pSps, UINT32 spsSize, PUINT16 pWidth, PUINT16 pHeight)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 frameCropLeftOffset = 0,
            frameCropRightOffset = 0,
            frameCropTopOffset = 0,
            frameCropBottomOffset = 0,
            picWidthInMbsMinus1 = 0,
            frameMbsOnlyFlag = 0,
            picHeightInMapUnitsMinus1 = 0,
            profileIdc = 0,
            sizeOfScalingList = 0,
            lastScale = 8,
            nextScale = 8,
            picOrderCntType = 0,
            numRefFramesInPicOrderCntCycle = 0,
            deltaScale = 0;
    UINT32 i, j, read;
    INT32 readInt, width, height;

    BitReader bitReader;

    CHK(pSps != NULL && pWidth != NULL && pHeight != NULL, STATUS_NULL_ARG);
    CHK(spsSize != 0, STATUS_INVALID_ARG_LEN);

    // Create a bit reader on top of the SPS
    CHK_STATUS(bitReaderReset(&bitReader, pSps, spsSize * 8));

    // Read the SPS Nalu
    CHK_STATUS(bitReaderReadBits(&bitReader, 8, &read));
    CHK(read == SPS_NALU_67 || read == SPS_NALU_27, STATUS_MKV_INVALID_H264_H265_SPS_NALU);

    // Get the profile
    CHK_STATUS(bitReaderReadBits(&bitReader, 8, &profileIdc));

    // Skip the constraint set flags 0 - 5, the reserved 2 zero bits
    // 8 bits level_idc and after that read the exp golomb for seq_parameter_set_id
    CHK_STATUS(bitReaderReadBits(&bitReader, 6 + 2 + 8, &read));

    // Read exp golomb for seq_parameter_set_id
    CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));

    if (profileIdc == 100 || profileIdc == 110
        || profileIdc == 122 || profileIdc == 244
        || profileIdc == 44 || profileIdc == 83
        || profileIdc == 86 || profileIdc == 118) {
        // Read chroma_format_idc
        CHK_STATUS(bitReaderReadExpGolomb(&bitReader, &read));

        if (read == 3) {
            // Read residual_colour_transform_flag
            CHK_STATUS(bitReaderReadBit(&bitReader, &read));
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

    width = ((picWidthInMbsMinus1 + 1) * 16) - frameCropLeftOffset * 2 - frameCropRightOffset * 2;
    height = ((2 - frameMbsOnlyFlag) * (picHeightInMapUnitsMinus1 + 1) * 16) - (frameCropTopOffset * 2) - (frameCropBottomOffset * 2);

    CHK(width >= 0 && width <= MAX_UINT16, STATUS_MKV_INVALID_H264_H265_SPS_WIDTH);
    CHK(height >= 0 && height <= MAX_UINT16, STATUS_MKV_INVALID_H264_H265_SPS_HEIGHT);

    *pWidth = width;
    *pHeight = height;

CleanUp:

    return retStatus;
}
