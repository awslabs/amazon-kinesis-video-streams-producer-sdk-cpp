/*******************************************
Stream include file
*******************************************/
#ifndef __SPS_PARSER_H_
#define __SPS_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

#pragma once
////////////////////////////////////////////////////
// General defines and data structures
////////////////////////////////////////////////////

// Check whether to generate video config element based on the width not being 0
#define GENERATE_VIDEO_CONFIG(x)                                                                                                                     \
    (((PTrackInfo)(x))->trackType == MKV_TRACK_INFO_TYPE_VIDEO && ((PTrackInfo)(x))->trackCustomData.trackVideoConfig.videoWidth != 0)
#define GENERATE_AUDIO_CONFIG(x)                                                                                                                     \
    (((PTrackInfo)(x))->trackType == MKV_TRACK_INFO_TYPE_AUDIO && ((PTrackInfo)(x))->trackCustomData.trackAudioConfig.samplingFrequency != 0)

// SPS NALU type value
#define SPS_NALU_TYPE 0x07

// PPS NALU type value
#define PPS_NALU_TYPE 0x08

// IDR NALU type value
#define IDR_NALU_TYPE 0x05

// H265 IDR NALU type value
#define IDR_W_RADL_NALU_TYPE 0x13
#define IDR_N_LP_NALU_TYPE   0x14

// AUD NALu value
#define AUD_NALU_TYPE 0x09

/**
 * Bitmap info header size
 */
#define BITMAP_INFO_HEADER_SIZE 40

/**
 * Max sub-layers in H265 SPS
 */
#define MAX_H265_SPS_SUB_LAYERS 8

/**
 * H265 SPS info structure containing only items needed for resolution calculation
 */
typedef struct {
    UINT8 conformance_window_flag;
    UINT8 chroma_format_idc;
    UINT32 conf_win_right_offset;
    UINT32 conf_win_left_offset;
    UINT32 conf_win_bottom_offset;
    UINT32 conf_win_top_offset;
    UINT16 pic_width_in_luma_samples;
    UINT16 pic_height_in_luma_samples;
    UINT8 max_sub_layers_minus1;
    UINT8 general_profile_space;
    UINT8 general_tier_flag;
    UINT8 general_profile_idc;
    UINT8 general_profile_compatibility_flags[4];
    UINT8 general_constraint_indicator_flags[6];
    UINT8 general_level_idc;
    UINT8 bit_depth_luma_minus8;
    UINT8 bit_depth_chroma_minus8;
} H265SpsInfo, *PH265SpsInfo;

////////////////////////////////////////////////////
// Internal functionality
////////////////////////////////////////////////////
/**
 * Extracts the video configuration from the H264 SPS
 *
 * @PBYTE - Codec private data
 * @UINT32 - Codec private data size
 * @PUINT16 - OUT - Extracted width
 * @PUINT16 - OUT - Extracted height
 *
 * @return - STATUS code of the execution
 */
STATUS getVideoWidthAndHeightFromH264Sps(PBYTE, UINT32, PUINT16, PUINT16);

/**
 * Extracts the video configuration from the H265 SPS
 *
 * @PBYTE - Codec private data
 * @UINT32 - Codec private data size
 * @PUINT16 - OUT - Extracted width
 * @PUINT16 - OUT - Extracted height
 *
 * @return - STATUS code of the execution
 */
STATUS getVideoWidthAndHeightFromH265Sps(PBYTE, UINT32, PUINT16, PUINT16);

/**
 * Extracts the video configuration from the BitmapInfoHeader.
 *
 * https://msdn.microsoft.com/en-us/library/windows/desktop/dd183376(v=vs.85).aspx
 *
 * @PBYTE - Codec private data
 * @UINT32 - Codec private data size
 * @PUINT16 - OUT - Extracted width
 * @PUINT16 - OUT - Extracted height
 *
 * @return - STATUS code of the execution
 */
STATUS getVideoWidthAndHeightFromBih(PBYTE, UINT32, PUINT16, PUINT16);

/**
 * Parses H264 SPS and retrieves the resolution
 *
 * @PBYTE - SPS bits
 * @UINT32 - SPS size
 * @PUINT16 - OUT - Extracted width
 * @PUINT16 - OUT - Extracted height
 *
 * @return - STATUS code of the execution
 */
STATUS parseH264SpsGetResolution(PBYTE, UINT32, PUINT16, PUINT16);

/**
 * Parses H265 SPS and retrieves the sps info object
 *
 * @PBYTE - SPS bits
 * @UINT32 - SPS size
 * @PH265SpsInfo - OUT - Extracted SPS information
 *
 * @return - STATUS code of the execution
 */
STATUS parseH265Sps(PBYTE, UINT32, PH265SpsInfo);

/**
 * Parses H265 SPS and retrieves the resolution
 *
 * @PBYTE - SPS bits
 * @UINT32 - SPS size
 * @PUINT16 - OUT - Extracted width
 * @PUINT16 - OUT - Extracted height
 *
 * @return - STATUS code of the execution
 */
STATUS parseH265SpsGetResolution(PBYTE, UINT32, PUINT16, PUINT16);

/**
 * Parses the profile tier level from H265 SPS
 *
 * @PBitReader - BitReader object for bit reading
 * @PH265SpsInfo - IN_OUT - H265 sps info object
 *
 * @return - STATUS code of the execution
 */
STATUS parseProfileTierLevel(PBitReader, PH265SpsInfo);

/**
 * Parses scaling list data from H265 SPS
 *
 * @PBitReader - BitReader object for bit reading
 *
 * @return - STATUS code of the execution
 */
STATUS parseScalingListData(PBitReader);

/**
 * Extracts the resolution from H265 Sps info object
 *
 * @PH265SpsInfo - SPS info
 * @PUINT16 - OUT - Extracted width
 * @PUINT16 - OUT - Extracted height
 *
 * @return - STATUS code of the execution
 */
STATUS extractResolutionFromH265SpsInfo(PH265SpsInfo, PUINT16, PUINT16);

/**
 * Scans the AvCC NALu buffer for the first occurrence of SPS and PPS
 *
 * @PBYTE - IN - AvCC format NALu buffer
 * @UINT32 - IN - Size of the buffer
 * @PBYTE* - OUT - Pointer to the start of the SPS NALu run
 * @PUINT32 - OUT - Size of the SPS NALu
 * @PBYTE* - OUT - Pointer to the start of the PPS NALu run
 * @PUINT32 - OUT - Size of the PPS NALu
 *
 * @return - STATUS code of the execution
 */
STATUS getH264SpsPpsNalusFromAvccNalus(PBYTE, UINT32, PBYTE*, PUINT32, PBYTE*, PUINT32);

/**
 * Scans the AvCC NALu buffer for the first occurrence of H265 SPS
 *
 * @PBYTE - IN - AvCC format NALu buffer
 * @UINT32 - IN - Size of the buffer
 * @PBYTE* - OUT - Pointer to the start of the SPS NALu run
 * @PUINT32 - OUT - Size of the SPS NALu
 *
 * @return - STATUS code of the execution
 */
STATUS getH265SpsNalusFromAvccNalus(PBYTE, UINT32, PBYTE*, PUINT32);

#ifdef __cplusplus
}
#endif

#endif // __SPS_PARSER_H_
