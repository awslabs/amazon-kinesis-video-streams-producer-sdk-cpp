/**
 * Kinesis Video MKV generator statics
 */
#define LOG_CLASS "MkvGeneratorStatics"
#include "Include_i.h"

/**
 * Static definitions
 */
BYTE gMkvHeaderBits[] = {
    0x1A, 0x45, 0xDF, 0xA3,                                           // EBML element id
    0xA3,                                                             // Size of the header
    0x42, 0x86, 0x81, 0x01,                                           // EBML version
    0x42, 0xF7, 0x81, 0x01,                                           // EBML read version
    0x42, 0xF2, 0x81, 0x04,                                           // EBML max id len
    0x42, 0xF3, 0x81, 0x08,                                           // EBML max size len
    0x42, 0x82, 0x88, 0x6D, 0x61, 0x74, 0x72, 0x6F, 0x73, 0x6B, 0x61, // Doc type - "matroska"
    0x42, 0x87, 0x81, 0x02,                                           // Doc type version - 2
    0x42, 0x85, 0x81, 0x02,                                           // Doc type read version - 2
};
UINT32 gMkvHeaderBitsSize = SIZEOF(gMkvHeaderBits);

BYTE gMkvSegmentHeaderBits[] = {
    0x18, 0x53, 0x80, 0x67, // Segment header id
    0xFF,                   // Size of the header - unknown
};
UINT32 gMkvSegmentHeaderBitsSize = SIZEOF(gMkvSegmentHeaderBits);

BYTE gMkvSegmentInfoBits[] = {
    0x15, 0x49, 0xA9, 0x66, // Segment info id
    0x40, 0x00,             // Size of the header - 2 bytes wide to accommodate max names - will be fixed up
    0x73, 0xA4, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // SegmentUID
    0x2A, 0xD7, 0xB1, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // TimecodeScale - Placeholder to fix-up the timecode scale
};
UINT32 gMkvSegmentInfoBitsSize = SIZEOF(gMkvSegmentInfoBits);

BYTE gMkvTitleBits[] = {
    0x7B, 0xA9, 0x80, // Title of the application with size to fix up
};
UINT32 gMkvTitleBitsSize = SIZEOF(gMkvTitleBits);

BYTE gMkvMuxingAppBits[] = {
    0x4D, 0x80, 0x80, // Muxing application with size to fix up
};
UINT32 gMkvMuxingAppBitsSize = SIZEOF(gMkvMuxingAppBits);

BYTE gMkvWritingAppBits[] = {
    0x57, 0x41, 0x80, // Writing application with size to fix up
};
UINT32 gMkvWritingAppBitsSize = SIZEOF(gMkvWritingAppBits);

BYTE gMkvTracksElem[] = {
    0x16, 0x54, 0xAE, 0x6B, // Tracks
    0x10, 0x00, 0x00, 0x00, // Size of the header - 4 bytes wide to accommodate max codec private data
};
UINT32 gMkvTracksElemSize = SIZEOF(gMkvTracksElem);

BYTE gMkvTrackInfoBits[] = {
    0xAE,                                                             // Track entry
    0x10, 0x00, 0x00, 0x56,                                           // Size of the track - 4 bytes wide to accommodate max codec private data
    0xD7, 0x81, 0x01,                                                 // Track number
    0x73, 0xC5, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Track ID - placeholder for 8 bytes
    0x83, 0x81, 0x01,                                                 // Track type
};
UINT32 gMkvTrackInfoBitsSize = SIZEOF(gMkvTrackInfoBits);

BYTE gMkvCodecIdBits[] = {
    0x86, // Codec id
};
UINT32 gMkvCodecIdBitsSize = SIZEOF(gMkvCodecIdBits);

BYTE gMkvTrackNameBits[] = {
    0x53, 0x6E, // Track name
};
UINT32 gMkvTrackNameBitsSize = SIZEOF(gMkvTrackNameBits);

BYTE gMkvCodecPrivateDataElem[] = {
    0x63, 0xA2, // CodecPrivate
};
UINT32 gMkvCodecPrivateDataElemSize = SIZEOF(gMkvCodecPrivateDataElem);

BYTE gMkvTrackVideoBits[] = {
    0xE0,                   // Track Video Element
    0x10, 0x00, 0x00, 0x08, // Size of the Video element - 4 bytes wide to accommodate future extension
    0xB0,                   // Pixel width
    0x82,                   // 2-byte size
    0x00, 0x00,
    0xBA, // Pixel height
    0x82, // 2-byte size
    0x00, 0x00,
};
UINT32 gMkvTrackVideoBitsSize = SIZEOF(gMkvTrackVideoBits);

BYTE gMkvTrackAudioBits[] = {
    0xE1,                                           // Track Audio Element
    0x10, 0x00, 0x00, 0x0d,                         // Size of the Audio element - 4 bytes wide to accommodate future extension
    0xB5,                                           // Sampling Rate
    0x88,                                           // 8-byte size
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // IEEE 754 double representing the sampling frequency, in big endian
    0x9f,                                           // Number of Channels
    0x81,                                           // 1-byte size
    0x02,                                           // 2 channels
};
UINT32 gMkvTrackAudioBitsSize = SIZEOF(gMkvTrackAudioBits);

BYTE gMkvTrackAudioBitDepthBits[] = {
    0x62, 0x64, // Track Audio Bit Depth Element
    0x81,       // 1-byte size
    0x00,       // bit depth to be fixed up
};
UINT32 gMkvTrackAudioBitDepthBitsSize = SIZEOF(gMkvTrackAudioBitDepthBits);

BYTE gMkvClusterInfoBits[] = {
    0x1F, 0x43, 0xB6, 0x75,                                     // Cluster
    0xFF,                                                       // Size of the header - unknown
    0xE7, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Time code - a placeholder
    0xA7, 0x81, 0x00,                                           // Position - 0 for live
};
UINT32 gMkvClusterInfoBitsSize = SIZEOF(gMkvClusterInfoBits);

BYTE gMkvSimpleBlockBits[] = {
    0xA3,                                           // SimpleBlock
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Size of the block - needs to be fixed up
    0x81,                                           // Track number
    0x00, 0x00,                                     // Timecode - relative to cluster timecode - INT16 - needs to be fixed up
    0x00,                                           // Flags - needs to be fixed up
                                                    // Frame data follows
};
UINT32 gMkvSimpleBlockBitsSize = SIZEOF(gMkvSimpleBlockBits);

BYTE gMkvTagsBits[] = {
    0x12, 0x54, 0xC3, 0x67,                         // Tags
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Size of the tags - needs to be fixed up
    0x73, 0x73,                                     // Tag
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Size of the tag - needs to be fixed up
    0x67, 0xC8,                                     // Simple tag
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Size of the simple tag - needs to be fixed up
                                                    // Tag name and tag string to follow
};
UINT32 gMkvTagsBitsSize = SIZEOF(gMkvTagsBits);

BYTE gMkvTagNameBits[] = {
    0x45, 0xA3,                                     // Tag name
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Size of the tag name - needs to be fixed up
                                                    // Tag name
};
UINT32 gMkvTagNameBitsSize = SIZEOF(gMkvTagNameBits);

BYTE gMkvTagStringBits[] = {
    0x44, 0x87,                                     // Tag string
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Size of the tag string - needs to be fixed up
                                                    // Tag string
};
UINT32 gMkvTagStringBitsSize = SIZEOF(gMkvTagStringBits);

// Sampling Frequency in Hz
DOUBLE gMkvAACSamplingFrequencies[] = {
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350,
};
UINT32 gMkvAACSamplingFrequenciesCount = SIZEOF(gMkvAACSamplingFrequencies) / SIZEOF(DOUBLE);
