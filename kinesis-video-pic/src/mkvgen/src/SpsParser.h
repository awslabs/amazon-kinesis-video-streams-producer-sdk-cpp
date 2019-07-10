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
#define GENERATE_VIDEO_CONFIG(x)        (((PTrackInfo) (x))->trackType == MKV_TRACK_INFO_TYPE_VIDEO && ((PTrackInfo) (x))->trackCustomData.trackVideoConfig.videoWidth != 0)
#define GENERATE_AUDIO_CONFIG(x)        (((PTrackInfo) (x))->trackType == MKV_TRACK_INFO_TYPE_AUDIO && ((PTrackInfo) (x))->trackCustomData.trackAudioConfig.samplingFrequency != 0)

// SPS NALU values
#define SPS_NALU_67     0x67
#define SPS_NALU_27     0x27

// AUD NALu value
#define AUD_NALU_TYPE   0x09

/**
 * Bitmap info header size
 */
#define BITMAP_INFO_HEADER_SIZE     40

/**
 * Max sub-layers in H265 SPS
 */
#define MAX_H265_SPS_SUB_LAYERS     8

/**
 * H265 SPS info structure containing only items needed for resolution calculation
 */
typedef struct {
    UINT32 conformance_window_flag;
    UINT32 chroma_format_idc;
    UINT32 conf_win_right_offset;
    UINT32 conf_win_left_offset;
    UINT32 conf_win_bottom_offset;
    UINT32 conf_win_top_offset;
    UINT32 pic_width_in_luma_samples;
    UINT32 pic_height_in_luma_samples;
    UINT32 max_sub_layers_minus1;
    UINT32 general_profile_space;
    UINT32 general_tier_flag;
    UINT32 general_profile_idc;
    UINT32 general_profile_compatibility_flags[4];
    UINT32 general_constraint_indicator_flags[6];
    UINT32 general_level_idc;
    UINT32 bit_depth_luma_minus8;
    UINT32 bit_depth_chroma_minus8;
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

#ifdef __cplusplus
}
#endif

#endif // __SPS_PARSER_H_
