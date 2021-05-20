/*******************************************
Stream include file
*******************************************/
#ifndef __NAL_ADAPTER_H_
#define __NAL_ADAPTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#pragma once
////////////////////////////////////////////////////
// General defines and data structures
////////////////////////////////////////////////////
/**
 * Avcc format CPD overhead size
 *
 * For more info on the format check out https://www.iso.org/standard/50726.html
 * 8 bit Version, profile, compat, level, 8 bit NALU len minus one, 8 bit number of SPS NALUs, 16 bit SPS size, SPS data, 8 bit PPS NALUs, 16 bit PPS
 * size, PPS data
 */
#define AVCC_CPD_OVERHEAD 11

/**
 * Hevc format CPD overhead size
 *
 * For more info on the format check out https://www.iso.org/standard/69660.html
 *
 * aligned(8) class HEVCDecoderConfigurationRecord {
 *  unsigned int(8) configurationVersion = 1;
 *  unsigned int(2) general_profile_space;
 *  unsigned int(1) general_tier_flag;
 *  unsigned int(5) general_profile_idc;
 *  unsigned int(32) general_profile_compatibility_flags;
 *  unsigned int(48) general_constraint_indicator_flags;
 *  unsigned int(8) general_level_idc;
 *  bit(4) reserved = ‘1111’b;
 *  unsigned int(12) min_spatial_segmentation_idc;
 *  bit(6) reserved = ‘111111’b;
 *  unsigned int(2) parallelismType;
 *  bit(6) reserved = ‘111111’b;
 *  unsigned int(2) chroma_format_idc;
 *  bit(5) reserved = ‘11111’b;
 *  unsigned int(3) bit_depth_luma_minus8;
 *  bit(5) reserved = ‘11111’b;
 *  unsigned int(3) bit_depth_chroma_minus8;
 *  bit(16) avgFrameRate;
 *  bit(2) constantFrameRate;
 *  bit(3) numTemporalLayers;
 *  bit(1) temporalIdNested;
 *  unsigned int(2) lengthSizeMinusOne;
 *  unsigned int(8) numOfArrays;
 *  for (j=0; j < numOfArrays; j++) {
 *     bit(1) array_completeness;
 *     unsigned int(1) reserved = 0;
 *     unsigned int(6) NAL_unit_type;
 *     unsigned int(16) numNalus;
 *     for (i=0; i< numNalus; i++) {
 *        unsigned int(16) nalUnitLength;
 *        bit(8*nalUnitLength) nalUnit;
 *     }
 *  }
 *}
 */
#define HEVC_CPD_HEADER_OVERHEAD 23
#define HEVC_CPD_ENTRY_OVERHEAD  5

/**
 * Minimal size of the H264 Annex-B encoded CPD size should be at least two short start codes and min sps pps data
 */
#define MIN_H264_ANNEXB_CPD_SIZE 3 + 3 + 4 + 1

/**
 * Minimal size of the H265 Annex-B encoded CPD size should be at least three short start codes and min vps sps pps data
 */
#define MIN_H_265_ANNEXB_CPD_SIZE 3 + 3 + 3 + 1 + 4 + 1

/**
 * AVCC Nalu size minus one byte - 6 significant bits set to 1 + 4 bytes per NALU - another 11 bits set
 */
#define AVCC_NALU_LEN_MINUS_ONE 0xFF

/**
 * AVCC Number of SPS blocks byte with 3 leading reserved bits set to 1 and 1 SPS blocks to follow
 */
#define AVCC_NUMBER_OF_SPS_ONE 0xE1

/**
 * AVCC version code
 */
#define AVCC_VERSION_CODE 0x01

/**
 * HEVC configuration version code
 */
#define HEVC_CONFIG_VERSION_CODE 0x01

/**
 * The H264/H265 should be at least this long
 */
#define MIN_H264_H265_CPD_SIZE 8

/**
 * Min Hevc encoded CPD size
 */
#define HEVC_CPD_HEADER_SIZE 23

/**
 * Min HEVC array entry size
 */
#define HEVC_NALU_ARRAY_ENTRY_SIZE 3

/**
 * SPS bits offset in AVCC encoded CPD
 */
#define AVCC_SPS_OFFSET 8

/**
 * HEVC VPS NALu type
 */
#define HEVC_VPS_NALU_TYPE 0x20

/**
 * HEVC SPS NALu type
 */
#define HEVC_SPS_NALU_TYPE 0x21

/**
 * HEVC PPS NALu type
 */
#define HEVC_PPS_NALU_TYPE 0x22

#ifdef FIXUP_ANNEX_B_TRAILING_NALU_ZERO
#define HANDLING_TRAILING_NALU_ZERO TRUE
#define MAX_ANNEX_B_ZERO_COUNT      4
#else
#define HANDLING_TRAILING_NALU_ZERO FALSE
#define MAX_ANNEX_B_ZERO_COUNT      3
#endif

/**
 * NALu adaptation
 */
typedef enum {
    MKV_NALS_ADAPT_NONE,
    MKV_NALS_ADAPT_ANNEXB,
    MKV_NALS_ADAPT_AVCC,
} MKV_NALS_ADAPTATION,
    *PMKV_NALS_ADAPTATION;

////////////////////////////////////////////////////
// Internal functionality
////////////////////////////////////////////////////
/**
 * Adapts the frame data Annex-B NALUs to AVCC
 *
 * @PBYTE - Frame data buffer
 * @UINT32 - Frame data buffer size
 * @BOOL - Remove EPB (Emulation Prevention Bytes)
 * @PBYTE - OUT - OPTIONAL - Adapted frame data buffer
 * @PUINT32 - IN/OUT - Adapted frame data buffer size
 *
 * @return - STATUS code of the execution
 */
STATUS adaptFrameNalsFromAnnexBToAvcc(PBYTE, UINT32, BOOL, PBYTE, PUINT32);

/**
 * Adapts the CPD Annex-B NALUs to AVCC for H264
 *
 * @PBYTE - CPD buffer
 * @UINT32 - CPD buffer size
 * @PBYTE - OUT - OPTIONAL - Adapted CPD buffer
 * @PUINT32 - IN/OUT - Adapted CPD buffer size
 *
 * @return - STATUS code of the execution
 */
STATUS adaptH264CpdNalsFromAnnexBToAvcc(PBYTE, UINT32, PBYTE, PUINT32);

/**
 * Adapts the CPD Annex-B NALUs to HEVC for H265
 *
 * @PBYTE - CPD buffer
 * @UINT32 - CPD buffer size
 * @PBYTE - OUT - OPTIONAL - Adapted CPD buffer
 * @PUINT32 - IN/OUT - Adapted CPD buffer size
 *
 * @return - STATUS code of the execution
 */
STATUS adaptH265CpdNalsFromAnnexBToHvcc(PBYTE, UINT32, PBYTE, PUINT32);

/**
 * Adapts the frame data AVCC NALUs to Annex-B
 *
 * NOTE: The adaptation happens in-place aka - the input buffer will be modified
 *
 * @PBYTE - IN/OUT - Frame data buffer
 * @UINT32 - Frame data buffer size
 *
 * @return - STATUS code of the execution
 */
STATUS adaptFrameNalsFromAvccToAnnexB(PBYTE, UINT32);

/**
 * Gets the adapted frame size
 *
 * NOTE: Only converting from Annex-B to AVCC will potentially bloat the size.
 *
 * @PFrame - IN - Frame to get the packaged size for
 * @MKV_NALS_ADAPTATION - the nals adaptation mode
 * @PUINT32 - Packaged size of the frame
 *
 * @return - STATUS code of the execution
 */
STATUS getAdaptedFrameSize(PFrame, MKV_NALS_ADAPTATION, PUINT32);

/**
 * @PBYTE - CPD buffer
 * @UINT32 - CPD buffer size
 *
 * @return - Whether the format is HEVC header
 */
BOOL checkHevcFormatHeader(PBYTE, UINT32);

#ifdef __cplusplus
}
#endif

#endif // __NAL_ADAPTER_H_
