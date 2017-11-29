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
 * 8 bit Version, profile, compat, level, 8 bit NALU len minus one, 8 bit number of SPS NALUs, 16 bit SPS size, SPS data, 8 bit PPS NALUs, 16 bit PPS size, PPS data
 */
#define AVCC_CPD_OVERHEAD       11

/**
 * Minimal size of the Annex-B encoded CPD size should be at least two short start codes and min sps pps data
 */
#define MIN_ANNEXB_CPD_SIZE     3 + 3 + 4 + 1

/**
 * AVCC Nalu size minus one byte - 6 significant bits set to 1 + 4 bytes per NALU - another 11 bits set
 */
#define AVCC_NALU_LEN_MINUS_ONE     0xFF

/**
 * AVCC Number of SPS blocks byte with 3 leading reserved bits set to 1 and 1 SPS blocks to follow
 */
#define AVCC_NUMBER_OF_SPS_ONE      0xE1

/**
 * AVCC version code
 */
#define AVCC_VERSION_CODE           0x01

/**
 * The H264/H265 should be at least this long
 */
#define MIN_H264_H265_CPD_SIZE      8

/**
 * SPS bits offset in AVCC encoded CPD
 */
#define AVCC_SPS_OFFSET             8

/**
 * NALu adaptation
 */
typedef enum {
    MKV_NALS_ADAPT_NONE,
    MKV_NALS_ADAPT_ANNEXB,
    MKV_NALS_ADAPT_AVCC,
} MKV_NALS_ADAPTATION, *PMKV_NALS_ADAPTATION;

////////////////////////////////////////////////////
// Internal functionality
////////////////////////////////////////////////////
/**
 * Adapts the frame data Annex-B NALUs to AVCC
 *
 * @PBYTE - Frame data buffer
 * @UINT32 - Frame data buffer size
 * @PBYTE - OUT - OPTIONAL - Adapted frame data buffer
 * @PUINT32 - IN/OUT - Adapted frame data buffer size
 *
 * @return - STATUS code of the execution
 */
STATUS adaptFrameNalsFromAnnexBToAvcc(PBYTE, UINT32, PBYTE, PUINT32);

/**
 * Adapts the CPD Annex-B NALUs to AVCC
 *
 * @PBYTE - CPD buffer
 * @UINT32 - CPD buffer size
 * @PBYTE - OUT - OPTIONAL - Adapted CPD buffer
 * @PUINT32 - IN/OUT - Adapted CPD buffer size
 *
 * @return - STATUS code of the execution
 */
STATUS adaptCpdNalsFromAnnexBToAvcc(PBYTE, UINT32, PBYTE, PUINT32);

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
#ifdef __cplusplus
}
#endif

#endif // __NAL_ADAPTER_H_
