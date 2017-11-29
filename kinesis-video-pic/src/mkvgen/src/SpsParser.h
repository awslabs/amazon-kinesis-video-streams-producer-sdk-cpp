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
#define GENERATE_VIDEO_CONFIG(x)        (((PStreamMkvGenerator) (x))->videoWidth != 0)

// SPS NALU values
#define SPS_NALU_67     0x67
#define SPS_NALU_27     0x27

#define BITMAP_INFO_HEADER_SIZE     40

typedef struct {
    // SPS buffer
    PBYTE sps;

    // Size of the buffer
    UINT32 spsSize;

    // Current bit
    UINT32 currentBit;
} BitStreamReader, *PBitStreamReader;

////////////////////////////////////////////////////
// Internal functionality
////////////////////////////////////////////////////
/**
 * Extracts the video configuration from the SPS
 *
 * @PBYTE - Codec private data
 * @UINT32 - Codec private data size
 * @PUINT16 - OUT - Extracted width
 * @PUINT16 - OUT - Extracted height
 *
 * @return - STATUS code of the execution
 */
STATUS getVideoWidthAndHeightFromSps(PBYTE, UINT32, PUINT16, PUINT16);

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
 * Parses H264/H265 SPS and retrieves the resolution
 *
 * @PBYTE - SPS bits
 * @UINT32 - SPS size
 * @PUINT16 - OUT - Extracted width
 * @PUINT16 - OUT - Extracted height
 *
 * @return - STATUS code of the execution
 */
STATUS parseSpsGetResolution(PBYTE, UINT32, PUINT16, PUINT16);

#ifdef __cplusplus
}
#endif

#endif // __SPS_PARSER_H_
