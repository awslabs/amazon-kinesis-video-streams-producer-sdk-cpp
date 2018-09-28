/**
 * Main public include file
 */
#ifndef __MKV_GEN_INCLUDE__
#define __MKV_GEN_INCLUDE__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

#include <com/amazonaws/kinesis/video/common/CommonDefs.h>
#include <com/amazonaws/kinesis/video/common/PlatformUtils.h>

// IMPORTANT! Some of the headers are not tightly packed!
////////////////////////////////////////////////////
// Public headers
////////////////////////////////////////////////////
#include <com/amazonaws/kinesis/video/utils/Include.h>

// For tight packing
#pragma pack(push, include, 1) // for byte alignment

////////////////////////////////////////////////////
// Status return codes
////////////////////////////////////////////////////
#define STATUS_MKVGEN_BASE                                                          0x32000000
#define STATUS_MKV_INVALID_FRAME_DATA                                               STATUS_MKVGEN_BASE + 0x00000001
#define STATUS_MKV_INVALID_FRAME_TIMESTAMP                                          STATUS_MKVGEN_BASE + 0x00000002
#define STATUS_MKV_INVALID_CLUSTER_DURATION                                         STATUS_MKVGEN_BASE + 0x00000003
#define STATUS_MKV_INVALID_CONTENT_TYPE_LENGTH                                      STATUS_MKVGEN_BASE + 0x00000004
#define STATUS_MKV_NUMBER_TOO_BIG                                                   STATUS_MKVGEN_BASE + 0x00000005
#define STATUS_MKV_INVALID_CODEC_ID_LENGTH                                          STATUS_MKVGEN_BASE + 0x00000006
#define STATUS_MKV_INVALID_TRACK_NAME_LENGTH                                        STATUS_MKVGEN_BASE + 0x00000007
#define STATUS_MKV_INVALID_CODEC_PRIVATE_LENGTH                                     STATUS_MKVGEN_BASE + 0x00000008
#define STATUS_MKV_CODEC_PRIVATE_NULL                                               STATUS_MKVGEN_BASE + 0x00000009
#define STATUS_MKV_INVALID_TIMECODE_SCALE                                           STATUS_MKVGEN_BASE + 0x0000000a
#define STATUS_MKV_MAX_FRAME_TIMECODE                                               STATUS_MKVGEN_BASE + 0x0000000b
#define STATUS_MKV_LARGE_FRAME_TIMECODE                                             STATUS_MKVGEN_BASE + 0x0000000c
#define STATUS_MKV_INVALID_ANNEXB_NALU_IN_FRAME_DATA                                STATUS_MKVGEN_BASE + 0x0000000d
#define STATUS_MKV_INVALID_AVCC_NALU_IN_FRAME_DATA                                  STATUS_MKVGEN_BASE + 0x0000000e
#define STATUS_MKV_BOTH_ANNEXB_AND_AVCC_SPECIFIED                                   STATUS_MKVGEN_BASE + 0x0000000f
#define STATUS_MKV_INVALID_ANNEXB_NALU_IN_CPD                                       STATUS_MKVGEN_BASE + 0x00000010
#define STATUS_MKV_PTS_DTS_ARE_NOT_SAME                                             STATUS_MKVGEN_BASE + 0x00000011
#define STATUS_MKV_INVALID_H264_H265_CPD                                            STATUS_MKVGEN_BASE + 0x00000012
#define STATUS_MKV_INVALID_H264_H265_SPS_WIDTH                                      STATUS_MKVGEN_BASE + 0x00000013
#define STATUS_MKV_INVALID_H264_H265_SPS_HEIGHT                                     STATUS_MKVGEN_BASE + 0x00000014
#define STATUS_MKV_INVALID_H264_H265_SPS_NALU                                       STATUS_MKVGEN_BASE + 0x00000015
#define STATUS_MKV_INVALID_BIH_CPD                                                  STATUS_MKVGEN_BASE + 0x00000016
#define STATUS_MKV_INVALID_HEVC_NALU_COUNT                                          STATUS_MKVGEN_BASE + 0x00000017
#define STATUS_MKV_INVALID_HEVC_FORMAT                                              STATUS_MKVGEN_BASE + 0x00000018
#define STATUS_MKV_HEVC_SPS_NALU_MISSING                                            STATUS_MKVGEN_BASE + 0x00000019
#define STATUS_MKV_INVALID_HEVC_SPS_NALU_SIZE                                       STATUS_MKVGEN_BASE + 0x0000001a
#define STATUS_MKV_INVALID_HEVC_SPS_CHROMA_FORMAT_IDC                               STATUS_MKVGEN_BASE + 0x0000001b
#define STATUS_MKV_INVALID_HEVC_SPS_RESERVED                                        STATUS_MKVGEN_BASE + 0x0000001c
#define STATUS_MKV_MIN_ANNEX_B_CPD_SIZE                                             STATUS_MKVGEN_BASE + 0x0000001d
#define STATUS_MKV_ANNEXB_CPD_MISSING_NALUS                                         STATUS_MKVGEN_BASE + 0x0000001e
#define STATUS_MKV_INVALID_ANNEXB_CPD_NALUS                                         STATUS_MKVGEN_BASE + 0x0000001f
#define STATUS_MKV_INVALID_TAG_NAME_LENGTH                                          STATUS_MKVGEN_BASE + 0x00000020
#define STATUS_MKV_INVALID_TAG_VALUE_LENGTH                                         STATUS_MKVGEN_BASE + 0x00000021
#define STATUS_MKV_INVALID_GENERATOR_STATE_TAGS                                     STATUS_MKVGEN_BASE + 0x00000022

////////////////////////////////////////////////////
// Main structure declarations
////////////////////////////////////////////////////

/**
 * Max length of the content type in chars
 */
#define MAX_CONTENT_TYPE_LEN            128

/**
 * Max length of the codec ID
 */
#define MKV_MAX_CODEC_ID_LEN            32

/**
 * Max length of the track name
 */
#define MKV_MAX_TRACK_NAME_LEN          32

/**
 * Max codec private data length
 */
#define MKV_MAX_CODEC_PRIVATE_LEN       1 * 1024 * 1024

/**
 * Max tag name length
 */
#define MKV_MAX_TAG_NAME_LEN                128

/**
 * Max tag string value length
 */
#define MKV_MAX_TAG_VALUE_LEN               256

/**
 * Minimal and Maximal cluster durations sanity values
 */
#define MAX_CLUSTER_DURATION                (30 * HUNDREDS_OF_NANOS_IN_A_SECOND)
#define MIN_CLUSTER_DURATION                (1 * HUNDREDS_OF_NANOS_IN_A_SECOND)

/**
 * Minimal and Maximal timecode scale values - for sanity reasons
 */
#define MAX_TIMECODE_SCALE                  (1 * HUNDREDS_OF_NANOS_IN_A_SECOND)
#define MIN_TIMECODE_SCALE                  1

/**
 * Max possible timestamp - signed value. This is needed so we won't overflow
 * while multiplying by 100ns (default Kinesis Video time scale) and then divide it
 * by the value of the timecode scale before subtracting the cluster timecode
 * as the frame timecodes are relative to the beginning of the cluster.
 */
#define MAX_TIMESTAMP_VALUE                 (MAX_INT64 / DEFAULT_TIME_UNIT_IN_NANOS)

/**
 * Frame types enum
 */
typedef enum {
    /**
     * No flags are set
     */
    FRAME_FLAG_NONE                     = 0,

    /**
     * The frame is a key frame - I or IDR
     */
    FRAME_FLAG_KEY_FRAME                = (1 << 0),

    /**
     * The frame is discardable - no other frames depend on it
     */
    FRAME_FLAG_DISCARDABLE_FRAME        = (1 << 1),

    /**
     * The frame is invisible for rendering
     */
    FRAME_FLAG_INVISIBLE_FRAME          = (1 << 2),
} FRAME_FLAGS;

/**
 * MKV stream states enum
 */
typedef enum {
    MKV_STATE_START_STREAM,
    MKV_STATE_START_CLUSTER,
    MKV_STATE_START_BLOCK,
} MKV_STREAM_STATE, *PMKV_STREAM_STATE;

/**
 * Macros checking for the frame flags
 */
#define CHECK_FRAME_FLAG_KEY_FRAME(f)                          (((f) & FRAME_FLAG_KEY_FRAME) != FRAME_FLAG_NONE)
#define CHECK_FRAME_FLAG_DISCARDABLE_FRAME(f)                  (((f) & FRAME_FLAG_DISCARDABLE_FRAME) != FRAME_FLAG_NONE)
#define CHECK_FRAME_FLAG_INVISIBLE_FRAME(f)                    (((f) & FRAME_FLAG_INVISIBLE_FRAME) != FRAME_FLAG_NONE)

/**
 * Frame types enum
 */
typedef enum {
    /**
     * No flags are set
     */
    MKV_GEN_FLAG_NONE                   = 0,

    /**
     * Always create clusters on the key frame boundary
     */
    MKV_GEN_KEY_FRAME_PROCESSING        = (1 << 0),

    /**
     * Whether to use in-stream defined timestamps or call get time
     */
    MKV_GEN_IN_STREAM_TIME              = (1 << 1),

    /**
     * Whether to generate absolute cluster timestamps
     */
    MKV_GEN_ABSOLUTE_CLUSTER_TIME       = (1 << 2),

    /**
     * Whether to adapt Annex-B NALUs to Avcc NALUs
     */
    MKV_GEN_ADAPT_ANNEXB_NALS           = (1 << 3),

    /**
     * Whether to adapt Avcc NALUs to Annex-B NALUs
     */
    MKV_GEN_ADAPT_AVCC_NALS             = (1 << 4),

    /**
     * Whether to adapt Annex-B NALUs for the codec private data to Avcc format NALUs
     */
    MKV_GEN_ADAPT_ANNEXB_CPD_NALS       = (1 << 5),
} MKV_BEHAVIOR_FLAGS;

/**
 * The representation of the Frame
 */
typedef struct {
    // Id of the frame
    UINT32 index;

    // Flags associated with the frame (ex. IFrame for frames)
    FRAME_FLAGS flags;

    // The decoding timestamp of the frame in 100ns precision
    UINT64 decodingTs;

    // The presentation timestamp of the frame in 100ns precision
    UINT64 presentationTs;

    // The duration of the frame in 100ns precision
    UINT64 duration;

    // Size of the frame data in bytes
    UINT32 size;

    // The frame bits
    PBYTE frameData;
} Frame, *PFrame;

/**
 * The representation of the packaged frame information
 */
typedef struct {
    // Stream start timestamp adjusted with the timecode scale and the generator properties.
    UINT64 streamStartTs;

    // Cluster timestamp adjusted with the timecode scale and the generator properties.
    UINT64 clusterTs;

    // Frame presentation timestamp adjusted with the timecode scale.
    UINT64 framePts;

    // Frame decoding timestamp adjusted with the timecode scale.
    UINT64 frameDts;

    // The offset where the original/adapted frame data begins
    UINT16 dataOffset;

    // The state of the MKV stream generator.
    MKV_STREAM_STATE streamState;

} EncodedFrameInfo, *PEncodedFrameInfo;

/**
 * MKV Packager
 */
typedef struct __MkvGenerator MkvGenerator;
struct __MkvGenerator {
    UINT32 version;
    // NOTE: The internal structure follows
};

typedef __MkvGenerator* PMkvGenerator;

////////////////////////////////////////////////////
// Callbacks definitions
////////////////////////////////////////////////////
/**
 * Gets the current time in 100ns from some timestamp.
 *
 * @param 1 UINT64 - Custom handle passed by the caller.
 *
 * @return Current time value in 100ns
 */
typedef UINT64 (*GetCurrentTimeFunc)(UINT64);

////////////////////////////////////////////////////
// Public functions
////////////////////////////////////////////////////

/**
 * Create the MkvGenerator object
 *
 * @PCHAR - The content type of the stream
 * @UINT32 - The behavior flags
 * @UINT64 - Default timecode scale which will be applied to the generated MKV in 100ns
 * @UINT64 - Duration of the cluster in 100ns
 * @PCHAR - the codec ID of the track
 * @PCHAR - the track name
 * @PBYTE - codec private data
 * UINT32 - size of the codec private data
 * @GetCurrentTimeFunc - the time function callback
 * UINT64 - custom data to be passed to the callback
 * @PMkvGenerator* - returns the newly created object
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS createMkvGenerator(PCHAR, UINT32, UINT64, UINT64, PCHAR, PCHAR, PBYTE, UINT32, GetCurrentTimeFunc, UINT64, PMkvGenerator*);

/**
 * Frees and de-allocates the memory of the MkvGenerator and it's sub-objects
 *
 * NOTE: This function is idempotent - can be called at various stages of construction.
 *
 * @PMkvGenerator - the object to free
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS freeMkvGenerator(PMkvGenerator);

/**
 * Resets the generator to the initial state
 *
 * @PMkvGenerator - the object to reset
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS mkvgenResetGenerator(PMkvGenerator);

/**
 * Generates the MKV header
 *
 * @PMkvGenerator - The generator object
 * @PBYTE - Buffer to hold the packaged bits
 * @PUINT32 - IN/OUT - Size of the produced packaged bits
 * @PUINT64 - OUT - Stream start timestamp
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS mkvgenGenerateHeader(PMkvGenerator, PBYTE, PUINT32, PUINT64);

/**
 * Generates the MKV Tags/Tag/SimpleTag/<TagName, TagString> element
 *
 * @PMkvGenerator - The generator object
 * @PBYTE - Buffer to hold the packaged bits
 * @PCHAR - Name of the tag
 * @PCHAR - Value of the tag as a string
 * @PUINT32 - IN/OUT - Size of the produced packaged bits
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS mkvgenGenerateTag(PMkvGenerator, PBYTE, PCHAR, PCHAR, PUINT32);

/**
 * Packages a frame into an MKV fragment
 *
 * @PMkvGenerator - The generator object
 * @PFrame - Frame to package
 * @PBYTE - Buffer to hold the packaged bits
 * @PUINT32 - IN/OUT - Size of the produced packaged bits
 * @PEncodedFrameInfo - OUT OPT - Information about the encoded frame - optional.
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS mkvgenPackageFrame(PMkvGenerator, PFrame, PBYTE, PUINT32, PEncodedFrameInfo);

/**
 * Converts an MKV timecode to a timestamp
 *
 * @PMkvGenerator - The generator object
 * UINT64 - MKV timecode to convert
 * @PUINT64 - OUT - The converted timestamp
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS mkvgenTimecodeToTimestamp(PMkvGenerator, UINT64, PUINT64);

/**
 * Gets the MKV overhead in bytes for a specified type of frame
 *
 * @PMkvGenerator - The generator object
 * MKV_STREAM_STATE - Type of frame to get the overhead for
 * @PUINT32 - OUT - The overhead in bytes
 *
 * @return - STATUS code of the execution
 */
PUBLIC_API STATUS mkvgenGetMkvOverheadSize(PMkvGenerator, MKV_STREAM_STATE, PUINT32);

#pragma pack(pop, include)

#ifdef  __cplusplus
}
#endif
#endif  /* __MKV_GEN_INCLUDE__ */
