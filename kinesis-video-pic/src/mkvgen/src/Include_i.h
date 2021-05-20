/*******************************************
Main internal include file
*******************************************/
#ifndef __MKV_GEN_INCLUDE_I__
#define __MKV_GEN_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////
// Project include files
////////////////////////////////////////////////////
#include "com/amazonaws/kinesis/video/mkvgen/Include.h"

////////////////////////////////////////////////////
// Packager version
//
// IMPORTANT!!! Update the version when modifying
// the packaging bits.
//
////////////////////////////////////////////////////
#define MKV_GENERATOR_CURRENT_VERSION_STRING      "1.1.0"
#define MKV_GENERATOR_CURRENT_VERSION_STRING_SIZE (UINT32)((SIZEOF(MKV_GENERATOR_CURRENT_VERSION_STRING) / SIZEOF(CHAR)) - 1)

/**
 * MKV generator application name to be used in MKV header
 */
#define MKV_GENERATOR_APPLICATION_NAME_STRING      "Kinesis Video SDK"
#define MKV_GENERATOR_APPLICATION_NAME_STRING_SIZE (UINT32)((SIZEOF(MKV_GENERATOR_APPLICATION_NAME_STRING) / SIZEOF(CHAR)) - 1)

////////////////////////////////////////////////////
// General defines and data structures
////////////////////////////////////////////////////

/**
 * Static definitions
 */
extern BYTE gMkvHeaderBits[];
extern UINT32 gMkvHeaderBitsSize;
#define MKV_HEADER_BITS      gMkvHeaderBits
#define MKV_HEADER_BITS_SIZE gMkvHeaderBitsSize

extern BYTE gMkvSegmentHeaderBits[];
extern UINT32 gMkvSegmentHeaderBitsSize;
#define MKV_SEGMENT_HEADER_BITS      gMkvSegmentHeaderBits
#define MKV_SEGMENT_HEADER_BITS_SIZE gMkvSegmentHeaderBitsSize

extern BYTE gMkvSegmentInfoBits[];
extern UINT32 gMkvSegmentInfoBitsSize;
#define MKV_SEGMENT_INFO_BITS      gMkvSegmentInfoBits
#define MKV_SEGMENT_INFO_BITS_SIZE gMkvSegmentInfoBitsSize

extern BYTE gMkvTitleBits[];
extern UINT32 gMkvTitleBitsSize;
#define MKV_TITLE_BITS      gMkvTitleBits
#define MKV_TITLE_BITS_SIZE gMkvTitleBitsSize

extern BYTE gMkvMuxingAppBits[];
extern UINT32 gMkvMuxingAppBitsSize;
#define MKV_MUXING_APP_BITS      gMkvMuxingAppBits
#define MKV_MUXING_APP_BITS_SIZE gMkvMuxingAppBitsSize

extern BYTE gMkvWritingAppBits[];
extern UINT32 gMkvWritingAppBitsSize;
#define MKV_WRITING_APP_BITS      gMkvWritingAppBits
#define MKV_WRITING_APP_BITS_SIZE gMkvWritingAppBitsSize

// Offset into gMkvSegmentInfoBits for fixing-up the Segment Info size
#define MKV_SEGMENT_INFO_SIZE_OFFSET 4

// Segment info header size - element name + 2 byte size
#define MKV_SEGMENT_INFO_HEADER_SIZE (MKV_SEGMENT_INFO_SIZE_OFFSET + SIZEOF(UINT16))

// Offset into gMkvSegmentInfoBits for fixing-up the SegmentUID
#define MKV_SEGMENT_UID_OFFSET 9

// Offset into gMkvSegmentInfoBits for fixing-up the timecode scale
#define MKV_SEGMENT_TIMECODE_SCALE_OFFSET 29

// gMkvTitleBits, gMkvMuxingAppBits and gMkvWritingAppBits element size offset for fixing up
#define MKV_APP_ELEMENT_SIZE_OFFSET 2

extern DOUBLE gMkvAACSamplingFrequencies[];
extern UINT32 gMkvAACSamplingFrequenciesCount;
#define MKV_AAC_SAMPLING_FREQUNECY_IDX_MAX gMkvAACSamplingFrequenciesCount
// documented here: https://wiki.multimedia.cx/index.php/MPEG-4_Audio
#define MKV_AAC_CHANNEL_CONFIG_MAX 8

extern BYTE gMkvTrackInfoBits[];
extern UINT32 gMkvTrackInfoBitsSize;
extern BYTE gMkvTracksElem[];
extern UINT32 gMkvTracksElemSize;
extern BYTE gMkvTrackVideoBits[];
extern UINT32 gMkvTrackVideoBitsSize;
extern BYTE gMkvCodecPrivateDataElem[];
extern UINT32 gMkvCodecPrivateDataElemSize;
extern BYTE gMkvTrackAudioBits[];
extern UINT32 gMkvTrackAudioBitsSize;
extern BYTE gMkvTrackAudioBitDepthBits[];
extern UINT32 gMkvTrackAudioBitDepthBitsSize;
extern BYTE gMkvCodecIdBits[];
extern UINT32 gMkvCodecIdBitsSize;
extern BYTE gMkvTrackNameBits[];
extern UINT32 gMkvTrackNameBitsSize;
#define MKV_TRACK_INFO_BITS                 gMkvTrackInfoBits
#define MKV_TRACK_INFO_BITS_SIZE            gMkvTrackInfoBitsSize
#define MKV_TRACKS_ELEM_BITS                gMkvTracksElem
#define MKV_TRACKS_ELEM_BITS_SIZE           gMkvTracksElemSize
#define MKV_TRACK_VIDEO_BITS                gMkvTrackVideoBits
#define MKV_TRACK_VIDEO_BITS_SIZE           gMkvTrackVideoBitsSize
#define MKV_CODEC_PRIVATE_DATA_ELEM         gMkvCodecPrivateDataElem
#define MKV_CODEC_PRIVATE_DATA_ELEM_SIZE    gMkvCodecPrivateDataElemSize
#define MKV_TRACK_AUDIO_BITS                gMkvTrackAudioBits
#define MKV_TRACK_AUDIO_BITS_SIZE           gMkvTrackAudioBitsSize
#define MKV_TRACK_AUDIO_BIT_DEPTH_BITS      gMkvTrackAudioBitDepthBits
#define MKV_TRACK_AUDIO_BIT_DEPTH_BITS_SIZE gMkvTrackAudioBitDepthBitsSize
#define MKV_CODEC_ID_BITS                   gMkvCodecIdBits
#define MKV_CODEC_ID_BITS_SIZE              gMkvCodecIdBitsSize
#define MKV_TRACK_NAME_BITS                 gMkvTrackNameBits
#define MKV_TRACK_NAME_BITS_SIZE            gMkvTrackNameBitsSize

// gMkvTrackInfoBits element size offset for fixing up
#define MKV_TRACK_HEADER_SIZE_OFFSET 4

// gMkvTrackInfoBits track entry size offset for fixing up
#define MKV_TRACK_ENTRY_SIZE_OFFSET 1

// gMkvTrackInfoBits track ID offset for fixing up
#define MKV_TRACK_ID_OFFSET 11

// gMkvTrackInfoBits track type offset for fixing up
#define MKV_TRACK_TYPE_OFFSET 21

// Track info video width offset for fixing up
#define MKV_TRACK_VIDEO_WIDTH_OFFSET 7

// Track info video height offset for fixing up
#define MKV_TRACK_VIDEO_HEIGHT_OFFSET 11

// Number of bytes to skip to get from Tracks to first TrackEntry
#define MKV_TRACK_ENTRY_OFFSET 8

// Number of bytes to skip to get from begin of TrackEntry to TrackNumber
#define MKV_TRACK_NUMBER_OFFSET 7

// Number of bytes to skip to get from begin of Audio to SamplingFrequency
#define MKV_TRACK_AUDIO_SAMPLING_RATE_OFFSET 7

// 1 byte for audio element, 4 byte for ebml size
#define MKV_TRACK_AUDIO_EBML_HEADER_SIZE 5

// Number of bytes to skip to get from begin of TrackEntry to Channels
#define MKV_TRACK_AUDIO_CHANNELS_OFFSET 17

// Number of bytes to skip to get from begin of TrackEntry to bit depth
#define MKV_TRACK_AUDIO_BIT_DEPTH_OFFSET 21

// Mkv TrackEntry element plus its data size
#define MKV_TRACK_ENTRY_HEADER_BITS_SIZE 5

// Minimum A_MS/ACM codec private size
#define MIN_AMS_ACM_CPD_SIZE 16

extern BYTE gMkvClusterInfoBits[];
extern UINT32 gMkvClusterInfoBitsSize;
#define MKV_CLUSTER_INFO_BITS      gMkvClusterInfoBits
#define MKV_CLUSTER_INFO_BITS_SIZE gMkvClusterInfoBitsSize

// gMkvClusterInfoBits cluster timecode offset for fixing up
#define MKV_CLUSTER_TIMECODE_OFFSET 7

extern BYTE gMkvSimpleBlockBits[];
extern UINT32 gMkvSimpleBlockBitsSize;
#define MKV_SIMPLE_BLOCK_BITS      gMkvSimpleBlockBits
#define MKV_SIMPLE_BLOCK_BITS_SIZE gMkvSimpleBlockBitsSize

// gMkvSimpleBlockBits payload header size - Track number, timecode (2 bytes) and flags
#define MKV_SIMPLE_BLOCK_PAYLOAD_HEADER_SIZE 4

// gMkvSimpleBlockBits simple block element size offset for fixing up
#define MKV_SIMPLE_BLOCK_SIZE_OFFSET 1

// gMkvSimpleBlockBits block timecode offset for fixing up
#define MKV_SIMPLE_BLOCK_TIMECODE_OFFSET 10

// gMkvSimpleBlockBits simple block flags offset for fixing up
#define MKV_SIMPLE_BLOCK_FLAGS_OFFSET 12

// Offset to the track number field in mkv simple block
#define MKV_SIMPLE_BLOCK_TRACK_NUMBER_OFFSET 9

extern BYTE gMkvTagsBits[];
extern UINT32 gMkvTagsBitsSize;
#define MKV_TAGS_BITS      gMkvTagsBits
#define MKV_TAGS_BITS_SIZE gMkvTagsBitsSize

extern BYTE gMkvTagNameBits[];
extern UINT32 gMkvTagNameBitsSize;
#define MKV_TAG_NAME_BITS      gMkvTagNameBits
#define MKV_TAG_NAME_BITS_SIZE gMkvTagNameBitsSize

extern BYTE gMkvTagStringBits[];
extern UINT32 gMkvTagStringBitsSize;
#define MKV_TAG_STRING_BITS      gMkvTagStringBits
#define MKV_TAG_STRING_BITS_SIZE gMkvTagStringBitsSize

// gMkvTagsBits tags element size offset for fixing up
#define MKV_TAGS_ELEMENT_SIZE_OFFSET 4

// gMkvTagsBits tag element offset for fixing up
#define MKV_TAG_ELEMENT_OFFSET 12

// gMkvTagsBits tag element size offset for fixing up
#define MKV_TAG_ELEMENT_SIZE_OFFSET 14

// gMkvTagsBits simple tag element offset for fixing up
#define MKV_SIMPLE_TAG_ELEMENT_OFFSET 22

// gMkvTagsBits simple tag element size offset for fixing up
#define MKV_SIMPLE_TAG_ELEMENT_SIZE_OFFSET 24

// gMkvTagNameBits simple tag name element size offset for fixing up
#define MKV_TAG_NAME_ELEMENT_SIZE_OFFSET 2

// gMkvTagStringBits simple tag string element size offset for fixing up
#define MKV_TAG_STRING_ELEMENT_SIZE_OFFSET 2

/**
 * MKV simple block flags
 */
#define MKV_SIMPLE_BLOCK_FLAGS_NONE       0x00
#define MKV_SIMPLE_BLOCK_KEY_FRAME_FLAG   0x80
#define MKV_SIMPLE_BLOCK_INVISIBLE_FLAG   0x08
#define MKV_SIMPLE_BLOCK_DISCARDABLE_FLAG 0x01

/**
 * MKV wave format code
 */
#define MKV_WAV_FORMAT_ALAW  0x0006
#define MKV_WAV_FORMAT_MULAW 0x0007

/**
 * MKV block overhead in bytes
 */
#define MKV_SIMPLE_BLOCK_OVERHEAD (MKV_SIMPLE_BLOCK_BITS_SIZE)

/**
 * MKV cluster overhead in bytes
 */
#define MKV_CLUSTER_OVERHEAD (MKV_CLUSTER_INFO_BITS_SIZE + MKV_SIMPLE_BLOCK_OVERHEAD)

/**
 * MKV EBML and Segment header size in bytes
 */
#define MKV_EBML_SEGMENT_SIZE (MKV_HEADER_BITS_SIZE + MKV_SEGMENT_HEADER_BITS_SIZE)

/**
 * To and from MKV timestamp conversion factoring in the timecode
 */
#define TIMESTAMP_TO_MKV_TIMECODE(ts, tcs) ((ts) *DEFAULT_TIME_UNIT_IN_NANOS / (tcs))
#define MKV_TIMECODE_TO_TIMESTAMP(tc, tcs) ((tc) * ((tcs) / DEFAULT_TIME_UNIT_IN_NANOS))

/**
 * Default value to set the segment uuid bytes to
 */
#define MKV_SEGMENT_UUID_DEFAULT_VALUE 0x55

/**
 * Version string delimiter separating the title, version and client id string
 */
#define MKV_VERSION_STRING_DELIMITER ' '

/**
 * The rest of the internal include files
 */
#include "NalAdapter.h"
#include "SpsParser.h"

/**
 * MKV stream states enum
 */
typedef enum {
    MKV_GENERATOR_STATE_START,
    MKV_GENERATOR_STATE_SEGMENT_HEADER,
    MKV_GENERATOR_STATE_CLUSTER_INFO,
    MKV_GENERATOR_STATE_SIMPLE_BLOCK,
    MKV_GENERATOR_STATE_TAGS,
} MKV_GENERATOR_STATE,
    *PMKV_GENERATOR_STATE;

/**
 * MKV track type used internally
 */
typedef enum {
    MKV_CONTENT_TYPE_NONE = (UINT64) 0,
    MKV_CONTENT_TYPE_UNKNOWN = (1 << 0),
    MKV_CONTENT_TYPE_H264 = (1 << 1),
    MKV_CONTENT_TYPE_H265 = (1 << 2),
    MKV_CONTENT_TYPE_X_MKV_VIDEO = (1 << 3),
    MKV_CONTENT_TYPE_X_MKV_AUDIO = (1 << 4),
    MKV_CONTENT_TYPE_AAC = (1 << 5),
    MKV_CONTENT_TYPE_ALAW = (1 << 6),
    MKV_CONTENT_TYPE_MULAW = (1 << 7),
    MKV_CONTENT_TYPE_H264_H265 = (MKV_CONTENT_TYPE_H264 | MKV_CONTENT_TYPE_H265),
} MKV_CONTENT_TYPE,
    *PMKV_CONTENT_TYPE;

/**
 * Content type delimiter character
 */
#define MKV_CONTENT_TYPE_DELIMITER ','

/**
 * MkvGenerator internal structure
 */
typedef struct {
    // The original public structure
    MkvGenerator mkvGenerator;

    // Whether to do clustering decision based on key frames only
    BOOL keyFrameClustering;

    // Whether to use in-stream timestamps
    BOOL streamTimestamps;

    // Whether to use absolute timestamps in clusters
    BOOL absoluteTimeClusters;

    // Timecode scale
    UINT64 timecodeScale;

    // Clusters desired duration
    UINT64 clusterDuration;

    // The content type of the stream
    MKV_CONTENT_TYPE contentType;

    // Time function entry
    GetCurrentTimeFunc getTimeFn;

    // Custom data to be passed to the callback
    UINT64 customData;

    // Generator internal state
    MKV_GENERATOR_STATE generatorState;

    // The latest cluster start timestamp
    UINT64 lastClusterPts;

    // The latest cluster start timestamp
    UINT64 lastClusterDts;

    // The timestamp of the beginning of the stream
    UINT64 streamStartTimestamp;

    // Whether we have stored the timestamp for clusters in case of relative cluster timestamps
    BOOL streamStartTimestampStored;

    // NALUs adaptation
    MKV_NALS_ADAPTATION nalsAdaptation;

    // Whether to adapt CPD NALs from Annex-B to Avcc format.
    BOOL adaptCpdNals;

    // Video height and width - Only for video
    UINT16 videoWidth;
    UINT16 videoHeight;

    // Segment UUID
    BYTE segmentUuid[MKV_SEGMENT_UUID_LEN];

    // Array of TrackInfo containing track metadata
    PTrackInfo trackInfoList;

    // Number of TrackInfo object in trackInfoList
    UINT32 trackInfoCount;

    // Version string to package with the MKV header
    // IMPORTANT!!! the combined string should be less than 127 chars
    CHAR version[MAX_MKV_CLIENT_ID_STRING_LEN + SIZEOF(MKV_GENERATOR_CURRENT_VERSION_STRING) + 1];

    // Size of the version string
    UINT32 versionSize;

} StreamMkvGenerator, *PStreamMkvGenerator;

////////////////////////////////////////////////////
// Internal functionality
////////////////////////////////////////////////////
/**
 * Gets the size overhead packaging to MKV
 *
 * @PStreamMkvGenerator - the current generator object
 * @FRAME_FLAGS - frame flags to use in calculation
 * @UINT64 - frame presentation timestamp
 *
 * @return - current state of the stream
 **/
MKV_STREAM_STATE mkvgenGetStreamState(PStreamMkvGenerator, FRAME_FLAGS, UINT64);

/**
 * Gets the size overhead packaging to MKV
 *
 * @PStreamMkvGenerator - the current generator object
 * @MKV_STREAM_STATE - State of the MKV parser
 *
 * @return - Size of the overhead in bytes
 **/
UINT32 mkvgenGetFrameOverhead(PStreamMkvGenerator, MKV_STREAM_STATE);

/**
 * Fast validation of the frame object
 *
 * @PStreamMkvGenerator - the current generator object
 * @PFrame - Frame object
 * @PTrackInfo - IN - Track info the frame belongs to
 * @PUINT64 - OUT - Extracted presentation timestamp
 * @PUINT64 - OUT - Extracted decoding timestamp
 * @PUINT64 - OUT - Extracted frame duration
 * @PMKV_STREAM_STATE - OUT - Current stream state
 *
 * @return - STATUS code of the execution
 **/
STATUS mkvgenValidateFrame(PStreamMkvGenerator, PFrame, PTrackInfo, PUINT64, PUINT64, PUINT64, PMKV_STREAM_STATE);

/**
 * Returns the MKV content type from the provided content type string by tokenizing and matching the string
 *
 * @PCHAR - content type string to convert
 *
 * @return - MKV content type corresponding to the content type string
 */
MKV_CONTENT_TYPE mkvgenGetContentTypeFromContentTypeString(PCHAR);

/**
 * Returns the MKV content type for a given tokenized string
 *
 * @PCHAR - content type string token to convert
 * @UINT32 - the length of the token
 *
 * @return - MKV content type corresponding to the content type string
 */
MKV_CONTENT_TYPE mkvgenGetContentTypeFromContentTypeTokenString(PCHAR, UINT32);

/**
 * EBML encodes a number and stores in the buffer
 *
 * @UINT64 - the number to encode
 * @PBYTE - the buffer to store the number in
 * @UINT32 - the size of the buffer
 * @PUINT32 - the returned encoded length in bytes
 */
STATUS mkvgenEbmlEncodeNumber(UINT64, PBYTE, UINT32, PUINT32);

/**
 * Stores a number in the buffer in a big-endian way
 *
 * @UINT64 - the number to encode
 * @PBYTE - the buffer to store the number in
 * @UINT32 - the size of the buffer
 * @PUINT32 - the returned encoded length in bytes
 */
STATUS mkvgenBigEndianNumber(UINT64, PBYTE, UINT32, PUINT32);

/**
 * EBML encodes a header and stores in the buffer
 *
 * @PBYTE - the buffer to store the encoded info in
 * @UINT32 - the size of the buffer
 * @PUINT32 - the returned encoded length of the number in bytes
 */
STATUS mkvgenEbmlEncodeHeader(PBYTE, UINT32, PUINT32);

/**
 * EBML encodes a segment header and stores in the buffer
 *
 * @PBYTE - the buffer to store the encoded info in
 * @UINT32 - the size of the buffer
 * @PUINT32 - the returned encoded length of the number in bytes
 */
STATUS mkvgenEbmlEncodeSegmentHeader(PBYTE, UINT32, PUINT32);

/**
 * EBML encodes a segment info and stores in the buffer
 *
 * @PStreamMkvGenerator - the mkv generator object
 * @PBYTE - the buffer to store the encoded info in
 * @UINT32 - the size of the buffer
 * @PUINT32 - the returned encoded length of the number in bytes
 */
STATUS mkvgenEbmlEncodeSegmentInfo(PStreamMkvGenerator, PBYTE, UINT32, PUINT32);

/**
 * EBML encodes a track info and stores in the buffer
 *
 * @PBYTE - the buffer to store the encoded info in
 * @UINT32 - the size of the buffer
 * @PStreamMkvGenerator - The MKV generator
 * @PUINT32 - the returned encoded length of the number in bytes
 */
STATUS mkvgenEbmlEncodeTrackInfo(PBYTE, UINT32, PStreamMkvGenerator, PUINT32);

/**
 * EBML encodes a cluster header and stores in the buffer
 *
 * @PBYTE - the buffer to store the encoded info in
 * @UINT32 - the size of the buffer
 * @UINT64 - cluster timestamp
 * @PUINT32 - the returned encoded length of the number in bytes
 */
STATUS mkvgenEbmlEncodeClusterInfo(PBYTE, UINT32, UINT64, PUINT32);

/**
 * EBML encodes a simple block and stores in the buffer
 *
 * @PBYTE - the buffer to store the encoded info in
 * @UINT32 - the size of the buffer
 * @INT16 - frame timestamp
 * @PFrame - the frame to encode
 * @MKV_NALS_ADAPTATION - Type of NAL adaptation to perform
 * @UINT32 - the adapted frame size
 * @PStreamMkvGenerator - The MKV generator
 * @PUINT32 - the returned encoded length of the number in bytes
 */
STATUS mkvgenEbmlEncodeSimpleBlock(PBYTE, UINT32, INT16, PFrame, MKV_NALS_ADAPTATION, UINT32, PStreamMkvGenerator, PUINT32);

/**
 * Gets the size of a single mkv TrackEntry
 * @PTrackInfo - the TrackInfo object
 *
 * @return - Size of the TrackEntry in bytes
 */
UINT32 mkvgenGetTrackEntrySize(PTrackInfo);

/**
 * Gets the size of mkv Track header
 * @PTrackInfo - array of TrackInfo
 * @UINT32 - number of TrackInfo in the array
 *
 * @return - Size of mkv Track header in bytes
 */
UINT32 mkvgenGetMkvTrackHeaderSize(PTrackInfo, UINT32);

/**
 * Gets MKV Segment and Track header
 * @PStreamMkvGenerator - Stream generator object
 *
 * @return - Size of mkv Segment and Track header in bytes
 */
UINT32 mkvgenGetMkvSegmentTrackHeaderSize(PStreamMkvGenerator);

/**
 * Gets the size of mkv header
 * @PStreamMkvGenerator - Stream generator object
 *
 * @return - Size of mkv Track header in bytes
 */
UINT32 mkvgenGetMkvHeaderSize(PStreamMkvGenerator);

/**
 * Gets the size of mkv audio header
 * @PTrackInfo - TrackInfo object
 *
 * @return - Size of mkv audio header in bytes
 */
UINT32 mkvgenGetMkvAudioBitsSize(PTrackInfo pTrackInfo);

/**
 * Gets MKV header overhead in bytes
 * @PStreamMkvGenerator - Stream generator object
 *
 * @return - Size of mkv header overhead in bytes
 */
UINT32 mkvgenGetMkvHeaderOverhead(PStreamMkvGenerator);

/**
 * Gets MKV Segment and Track info overhead in bytes
 * @PStreamMkvGenerator - Stream generator object
 *
 * @return - Size of MKV Segment and Track info overhead in bytes
 */
UINT32 mkvgenGetMkvSegmentTrackInfoOverhead(PStreamMkvGenerator);

/**
 * Returns the byte count of a number
 *
 * @UINT64 - the number to process
 *
 * @return - the number of bytes required.
 */
UINT32 mkvgenGetByteCount(UINT64);

/**
 * Adapts the getTime function
 *
 * @UINT64 - the custom data passed in
 *
 * @return - the current time.
 */
UINT64 getTimeAdapter(UINT64);

/**
 * Parse out sampling frequency and channel config from aac cpd
 *
 * @PBYTE - The buffer storing aac codec private data
 * @UINT32 - Size of the codec private data
 * @PDOUBLE - The returned sampling frequency
 * @PUINT16 - The returned channel config
 */
STATUS getSamplingFreqAndChannelFromAacCpd(PBYTE, UINT32, PDOUBLE, PUINT16);

/**
 * Parse out sampling frequency and channel config from A_MS/ACM cpd
 *
 * @PBYTE - The buffer storing codec private data
 * @UINT32 - Size of the codec private data
 * @PDOUBLE - The returned sampling frequency
 * @PUINT16 - The returned channel config
 * @PUINT16 - The returned bit depth
 */
STATUS getAudioConfigFromAmsAcmCpd(PBYTE, UINT32, PDOUBLE, PUINT16, PUINT16);

/**
 * Adapt Codec Private Data for the given track
 *
 * @PStreamMkvGenerator - IN - The MKV generator
 * @MKV_TRACK_INFO_TYPE - IN - The track info type
 * @PCHAR - IN - Codec ID string
 * @UINT32 - IN - Input CPD size
 * @PBYTE - IN - Input CPD bytes
 * @PUINT32 - OUT - Output adapted CPD size
 * @PBYTE* - OUT - Output adapted CPD bytes
 * @PTrackCustomData - OUT - Track custom data to fill in
 *
 * @return Status of the operation
 */
STATUS mkvgenAdaptCodecPrivateData(PStreamMkvGenerator, MKV_TRACK_INFO_TYPE, PCHAR, UINT32, PBYTE, PUINT32, PBYTE*, PTrackCustomData);

/**
 * Attempts to extract the H264/H265 Annex-B format CPD from the frame data and set to the track info
 *
 * @PStreamMkvGenerator - IN - The MKV generator
 * @PFrame - IN - Frame to get the CPD from
 * @PTrackInfo - IN - Track info object to set the CPD for
 *
 * @return Status of the operation
 */
STATUS mkvgenExtractCpdFromAnnexBFrame(PStreamMkvGenerator, PFrame, PTrackInfo);

#ifdef __cplusplus
}
#endif
#endif /* __MKV_GEN_INCLUDE_I__ */
