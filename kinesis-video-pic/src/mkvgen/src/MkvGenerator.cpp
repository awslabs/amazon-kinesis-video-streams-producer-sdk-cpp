/**
 * Kinesis Video MKV Generator
 */
#define LOG_CLASS "MkvGenerator"

#include "Include_i.h"

//////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////

/**
 * Creates an mkv generator object
 */
STATUS createMkvGenerator(PCHAR contentType, UINT32 behaviorFlags, UINT64 timecodeScale, UINT64 clusterDuration,
        PCHAR codecId, PCHAR trackName, PBYTE codecPrivateData, UINT32 codecPrivateDataSize,
        GetCurrentTimeFunc getTimeFn, UINT64 customData, PMkvGenerator* ppMkvGenerator)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStreamMkvGenerator pMkvGenerator = NULL;
    UINT32 allocationSize, adaptedCodecPrivateDataSize;
    BOOL adaptAnnexB = FALSE, adaptAvcc = FALSE, adaptCpdAnnexB = FALSE;

    // Check the input params
    CHK(ppMkvGenerator != NULL, STATUS_NULL_ARG);

    // Clustering boundary checks if we are not doing key-frame clustering
    if ((behaviorFlags & MKV_GEN_KEY_FRAME_PROCESSING) == MKV_GEN_FLAG_NONE) {
        CHK(clusterDuration <= MAX_CLUSTER_DURATION && clusterDuration >= MIN_CLUSTER_DURATION, STATUS_MKV_INVALID_CLUSTER_DURATION);
    }

    if ((behaviorFlags & MKV_GEN_ADAPT_ANNEXB_NALS) == MKV_GEN_ADAPT_ANNEXB_NALS) {
        adaptAnnexB = TRUE;
    }

    if ((behaviorFlags & MKV_GEN_ADAPT_AVCC_NALS) == MKV_GEN_ADAPT_AVCC_NALS) {
        adaptAvcc = TRUE;
    }

    if ((behaviorFlags & MKV_GEN_ADAPT_ANNEXB_CPD_NALS) == MKV_GEN_ADAPT_ANNEXB_CPD_NALS) {
        adaptCpdAnnexB = TRUE;
    }

    CHK(!(adaptAnnexB && adaptAvcc), STATUS_MKV_BOTH_ANNEXB_AND_AVCC_SPECIFIED);
    CHK(timecodeScale <= MAX_TIMECODE_SCALE && timecodeScale >= MIN_TIMECODE_SCALE, STATUS_MKV_INVALID_TIMECODE_SCALE);
    CHK(STRNLEN(contentType, MAX_CONTENT_TYPE_LEN) < MAX_CONTENT_TYPE_LEN, STATUS_MKV_INVALID_CONTENT_TYPE_LENGTH);
    CHK(STRNLEN(codecId, MKV_MAX_CODEC_ID_LEN) < MKV_MAX_CODEC_ID_LEN, STATUS_MKV_INVALID_CODEC_ID_LENGTH);
    CHK(STRNLEN(trackName, MKV_MAX_TRACK_NAME_LEN) < MKV_MAX_TRACK_NAME_LEN, STATUS_MKV_INVALID_TRACK_NAME_LENGTH);
    CHK(codecPrivateDataSize <= MKV_MAX_CODEC_PRIVATE_LEN, STATUS_MKV_INVALID_CODEC_PRIVATE_LENGTH);
    CHK(codecPrivateDataSize == 0 || codecPrivateData != NULL, STATUS_MKV_CODEC_PRIVATE_NULL);

    // Initialize the endianness for the library
    initializeEndianness();

    // Initialize the random generator
    SRAND((UINT32) GETTIME());

    // Calculate the adapted CPD size
    adaptedCodecPrivateDataSize = codecPrivateDataSize;
    if (codecPrivateDataSize != 0 && adaptCpdAnnexB) {
        CHK_STATUS(adaptCpdNalsFromAnnexBToAvcc(codecPrivateData, codecPrivateDataSize, NULL, &adaptedCodecPrivateDataSize));
    }

    // Allocate the main struct
    // NOTE: The calloc will Zero the fields
    allocationSize = SIZEOF(StreamMkvGenerator) + adaptedCodecPrivateDataSize;
    pMkvGenerator = (PStreamMkvGenerator) MEMCALLOC(1, allocationSize);
    CHK(pMkvGenerator != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Set the values
    pMkvGenerator->mkvGenerator.version = 0;
    pMkvGenerator->timecodeScale = timecodeScale * DEFAULT_TIME_UNIT_IN_NANOS; // store in nanoseconds
    pMkvGenerator->clusterDuration = clusterDuration * DEFAULT_TIME_UNIT_IN_NANOS / pMkvGenerator->timecodeScale; // No chance of an overflow as we check against max earlier
    pMkvGenerator->trackType = mkvgenGetTrackTypeFromContentType(contentType);
    pMkvGenerator->streamStarted = FALSE;
    pMkvGenerator->keyFrameClustering = (behaviorFlags & MKV_GEN_KEY_FRAME_PROCESSING) != MKV_GEN_FLAG_NONE;
    pMkvGenerator->streamTimestamps = (behaviorFlags & MKV_GEN_IN_STREAM_TIME) != MKV_GEN_FLAG_NONE;
    pMkvGenerator->absoluteTimeClusters = (behaviorFlags & MKV_GEN_ABSOLUTE_CLUSTER_TIME) != MKV_GEN_FLAG_NONE;
    pMkvGenerator->adaptCpdNals = adaptCpdAnnexB;
    pMkvGenerator->lastClusterTimestamp = 0;
    pMkvGenerator->streamStartTimestamp = 0;
    STRNCPY(pMkvGenerator->codecId, codecId, MKV_MAX_CODEC_ID_LEN);
    pMkvGenerator->codecId[MKV_MAX_CODEC_ID_LEN] = '\0';
    STRNCPY(pMkvGenerator->trackName, trackName, MKV_MAX_TRACK_NAME_LEN);
    pMkvGenerator->trackName[MKV_MAX_TRACK_NAME_LEN] = '\0';

    if (adaptAnnexB) {
        pMkvGenerator->nalsAdaptation = MKV_NALS_ADAPT_ANNEXB;
    } else if (adaptAvcc) {
        pMkvGenerator->nalsAdaptation = MKV_NALS_ADAPT_AVCC;
    } else {
        pMkvGenerator->nalsAdaptation = MKV_NALS_ADAPT_NONE;
    }

    // the getTime function is optional
    pMkvGenerator->getTimeFn = (getTimeFn != NULL) ? getTimeFn : getTimeAdapter;

    // Store the custom data as well
    pMkvGenerator->customData = customData;

    // Set the codec private data
    pMkvGenerator->codecPrivateDataSize = adaptedCodecPrivateDataSize;
    pMkvGenerator->codecPrivateData = (PBYTE)(pMkvGenerator + 1);
    if (adaptedCodecPrivateDataSize != 0) {
        if (pMkvGenerator->adaptCpdNals) {
            CHK_STATUS(adaptCpdNalsFromAnnexBToAvcc(codecPrivateData, codecPrivateDataSize, pMkvGenerator->codecPrivateData, &pMkvGenerator->codecPrivateDataSize));
        } else {
            MEMCPY(pMkvGenerator->codecPrivateData, codecPrivateData, adaptedCodecPrivateDataSize);
        }
    }

    // Check whether we need to generate a video config element for
    // H264 or H265 content type if the CPD is present
    if ((0 == STRCMP(contentType, MKV_H264_CONTENT_TYPE) || 0 == STRCMP(contentType, MKV_H265_CONTENT_TYPE))
        && codecPrivateData != NULL
        && codecPrivateDataSize != 0) {

        retStatus = getVideoWidthAndHeightFromSps(pMkvGenerator->codecPrivateData,
                                                  pMkvGenerator->codecPrivateDataSize,
                                                  &pMkvGenerator->videoWidth,
                                                  &pMkvGenerator->videoHeight);
        if (STATUS_FAILED(retStatus)) {
            // This might not be yet fatal so warn and reset the status
            DLOGW("Failed extracting video configuration from SPS with %08x.", retStatus);

            retStatus = STATUS_SUCCESS;
        }
    }

    // For M-JPG we have content type as video/x-matroska and the codec
    // type set as V_MS/VFW/FOURCC
    if ((0 == STRCMP(contentType, MKV_X_MKV_CONTENT_TYPE) && 0 == STRCMP(codecId, MKV_FOURCC_CODEC_ID))
        && codecPrivateData != NULL
        && codecPrivateDataSize != 0) {

        retStatus = getVideoWidthAndHeightFromBih(pMkvGenerator->codecPrivateData,
                                                  pMkvGenerator->codecPrivateDataSize,
                                                  &pMkvGenerator->videoWidth,
                                                  &pMkvGenerator->videoHeight);
        if (STATUS_FAILED(retStatus)) {
            // This might not be yet fatal so warn and reset the status
            DLOGW("Failed extracting video configuration from SPS with %08x.", retStatus);

            retStatus = STATUS_SUCCESS;
        }
    }

    // Assign the created object
    *ppMkvGenerator = (PMkvGenerator) pMkvGenerator;

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        freeMkvGenerator((PMkvGenerator) pMkvGenerator);
    }

    LEAVES();
    return retStatus;
}

/**
 * Frees the mkv generator object
 */
STATUS freeMkvGenerator(PMkvGenerator pMkvGenerator)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    // Call is idempotent
    CHK(pMkvGenerator != NULL, retStatus);

    // Release the object
    MEMFREE(pMkvGenerator);

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Resets the generator to it's initial state.
 */
STATUS mkvgenResetGenerator(PMkvGenerator pMkvGenerator)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStreamMkvGenerator pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;

    // Check the input params
    CHK(pMkvGenerator != NULL, STATUS_NULL_ARG);

    // By setting streamStarted we will kick off the new header, etc...
    pStreamMkvGenerator->streamStarted = FALSE;

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Package frame in MKV format
 */
STATUS mkvgenPackageFrame(PMkvGenerator pMkvGenerator, PFrame pFrame, PBYTE pBuffer, PUINT32 pSize, PEncodedFrameInfo pEncodedFrameInfo)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStreamMkvGenerator pStreamMkvGenerator;
    MKV_STREAM_STATE streamState = MKV_STATE_START_STREAM;
    UINT32 bufferSize, encodedLen, packagedSize, adaptedFrameSize, overheadSize;
    // Evaluated presentation and decode timestamps
    UINT64 pts = 0, dts = 0;
    BOOL clusterStart = FALSE;
    PBYTE pCurrentPnt = pBuffer;

    // Check the input params
    CHK(pSize != NULL && pMkvGenerator != NULL, STATUS_NULL_ARG);

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;

    // Validate and extract the timestamp
    CHK_STATUS(mkvgenValidateFrame(pStreamMkvGenerator, pFrame, &pts, &dts, &streamState));

    // Calculate the necessary size

    // Get the overhead when packaging MKV
    overheadSize = mkvgenGetFrameOverhead(pStreamMkvGenerator, streamState);

    // Get the adapted size of the frame and add to the overall size
    CHK_STATUS(getAdaptedFrameSize(pFrame, pStreamMkvGenerator->nalsAdaptation, &adaptedFrameSize));
    packagedSize = overheadSize + adaptedFrameSize;

    // Check if we are asked for size only and early return if so
    CHK(pBuffer != NULL, STATUS_SUCCESS);

    // Preliminary check for the buffer size
    CHK(*pSize >= packagedSize, STATUS_NOT_ENOUGH_MEMORY);

    // Start with the full buffer
    bufferSize = *pSize;

    // Generate the actual data
    switch(streamState) {
        case MKV_STATE_START_STREAM:
            // Encode in sequence and subtract the size
            CHK_STATUS(mkvgenEbmlEncodeHeader(pCurrentPnt, bufferSize, &encodedLen));
            bufferSize -= encodedLen;
            pCurrentPnt += encodedLen;

            CHK_STATUS(mkvgenEbmlEncodeSegmentHeader(pCurrentPnt, bufferSize, &encodedLen));
            bufferSize -= encodedLen;
            pCurrentPnt += encodedLen;

            CHK_STATUS(mkvgenEbmlEncodeSegmentInfo(pCurrentPnt, bufferSize, pStreamMkvGenerator->timecodeScale, &encodedLen));
            bufferSize -= encodedLen;
            pCurrentPnt += encodedLen;

            CHK_STATUS(mkvgenEbmlEncodeTrackInfo(pCurrentPnt,
                                                 bufferSize,
                                                 pStreamMkvGenerator,
                                                 &encodedLen));
            bufferSize -= encodedLen;
            pCurrentPnt += encodedLen;

            // Fall-through
        case MKV_STATE_START_CLUSTER:
            // Store the stream timestamp if we started the stream
            if (!pStreamMkvGenerator->streamStarted) {
                pStreamMkvGenerator->streamStarted = TRUE;
                pStreamMkvGenerator->streamStartTimestamp = pts;
            }

            // Adjust the timestamp to the beginning of the stream if no absolute clustering
            CHK_STATUS(mkvgenEbmlEncodeClusterInfo(pCurrentPnt,
                                                   bufferSize,
                                                   pStreamMkvGenerator->absoluteTimeClusters ? pts : pts - pStreamMkvGenerator->streamStartTimestamp,
                                                   &encodedLen));
            bufferSize -= encodedLen;
            pCurrentPnt += encodedLen;

            // Store the timestamp of the last cluster
            pStreamMkvGenerator->lastClusterTimestamp = pts;

            // indicate a cluster start
            clusterStart = TRUE;

            // Fall-through
        case MKV_STATE_START_BLOCK:
            // Calculate the timestamp of the Frame relative to the cluster start
            if (clusterStart) {
                pts = dts = 0;
            } else {
                // Make the timestamp relative
                pts -= pStreamMkvGenerator->lastClusterTimestamp;
                dts -= pStreamMkvGenerator->lastClusterTimestamp;
            }

            // The timecode for the frame has only 2 bytes which represent a signed int.
            CHK(pts <= MAX_INT16, STATUS_MKV_LARGE_FRAME_TIMECODE);

            // Adjust the timestamp to the start of the cluster
            CHK_STATUS(mkvgenEbmlEncodeSimpleBlock(pCurrentPnt,
                                                   bufferSize,
                                                   (INT16) pts,
                                                   pFrame,
                                                   adaptedFrameSize,
                                                   pStreamMkvGenerator->nalsAdaptation,
                                                   &encodedLen));
            bufferSize -= encodedLen;
            pCurrentPnt += encodedLen;
            break;
    }

    // Validate the size
    CHK(packagedSize == (UINT32)(pCurrentPnt - pBuffer), STATUS_INTERNAL_ERROR);

CleanUp:

    if (STATUS_SUCCEEDED(retStatus)) {
        // Set the size and the state before return
        *pSize = packagedSize;

        if (pEncodedFrameInfo != NULL) {
            pEncodedFrameInfo->streamStartTs = MKV_TIMECODE_TO_TIMESTAMP(pStreamMkvGenerator->streamStartTimestamp, pStreamMkvGenerator->timecodeScale);
            pEncodedFrameInfo->clusterTs = MKV_TIMECODE_TO_TIMESTAMP(pStreamMkvGenerator->lastClusterTimestamp, pStreamMkvGenerator->timecodeScale);
            pEncodedFrameInfo->framePts = MKV_TIMECODE_TO_TIMESTAMP(pts, pStreamMkvGenerator->timecodeScale);
            pEncodedFrameInfo->frameDts = MKV_TIMECODE_TO_TIMESTAMP(dts, pStreamMkvGenerator->timecodeScale);
            pEncodedFrameInfo->dataOffset = overheadSize;
            pEncodedFrameInfo->streamState = streamState;
        }
    }

    LEAVES();
    return retStatus;
}

/**
 * Packages MKV header
 */
STATUS mkvgenGenerateHeader(PMkvGenerator pMkvGenerator, PBYTE pBuffer, PUINT32 pSize, PUINT64 pStreamStartTs)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStreamMkvGenerator pStreamMkvGenerator;
    UINT32 bufferSize, encodedLen, packagedSize;
    PBYTE pCurrentPnt = pBuffer;

    // Check the input params
    CHK(pSize != NULL && pMkvGenerator != NULL, STATUS_NULL_ARG);

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;

    // Calculate the necessary size

    // Get the overhead when packaging MKV
    packagedSize = MKV_HEADER_SIZE + mkvgenGetHeaderOverhead(pStreamMkvGenerator);

    // Check if we are asked for size only and early return if so
    CHK(pBuffer != NULL, STATUS_SUCCESS);

    // Preliminary check for the buffer size
    CHK(*pSize >= packagedSize, STATUS_NOT_ENOUGH_MEMORY);

    // Start with the full buffer
    bufferSize = *pSize;

    // Generate the actual data
    CHK_STATUS(mkvgenEbmlEncodeHeader(pCurrentPnt, bufferSize, &encodedLen));
    bufferSize -= encodedLen;
    pCurrentPnt += encodedLen;

    CHK_STATUS(mkvgenEbmlEncodeSegmentHeader(pCurrentPnt, bufferSize, &encodedLen));
    bufferSize -= encodedLen;
    pCurrentPnt += encodedLen;

    CHK_STATUS(mkvgenEbmlEncodeSegmentInfo(pCurrentPnt, bufferSize, pStreamMkvGenerator->timecodeScale, &encodedLen));
    bufferSize -= encodedLen;
    pCurrentPnt += encodedLen;

    CHK_STATUS(mkvgenEbmlEncodeTrackInfo(pCurrentPnt,
                                         bufferSize,
                                         pStreamMkvGenerator,
                                         &encodedLen));
    bufferSize -= encodedLen;
    pCurrentPnt += encodedLen;

    // Validate the size
    CHK(packagedSize == (UINT32)(pCurrentPnt - pBuffer), STATUS_INTERNAL_ERROR);

CleanUp:

    if (STATUS_SUCCEEDED(retStatus)) {
        // Set the size and the state before return
        *pSize = packagedSize;

        if (pStreamStartTs != NULL) {
            *pStreamStartTs = MKV_TIMECODE_TO_TIMESTAMP(pStreamMkvGenerator->streamStartTimestamp, pStreamMkvGenerator->timecodeScale);
        }
    }

    LEAVES();
    return retStatus;
}

/**
 * Converts an MKV timecode to a timestamp
 */
STATUS mkvgenTimecodeToTimestamp(PMkvGenerator pMkvGenerator, UINT64 timecode, PUINT64 pTimestamp)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStreamMkvGenerator pStreamMkvGenerator;

    // Check the input params
    CHK(pTimestamp != NULL && pMkvGenerator != NULL, STATUS_NULL_ARG);

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;

    *pTimestamp = MKV_TIMECODE_TO_TIMESTAMP(timecode, pStreamMkvGenerator->timecodeScale);

CleanUp:

    LEAVES();
    return retStatus;
}

//////////////////////////////////////////////////////////
// Private functions
//////////////////////////////////////////////////////////

/**
 * Simple and fast validation of a frame and extraction of the current frame timestamp
 */
STATUS mkvgenValidateFrame(PStreamMkvGenerator pStreamMkvGenerator, PFrame pFrame, PUINT64 pPts, PUINT64 pDts, PMKV_STREAM_STATE pStreamState)
{
    UINT64 dts = 0, pts = 0;
    MKV_STREAM_STATE streamState;
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pStreamMkvGenerator != NULL
        && pFrame != NULL
        && pPts != NULL
        && pDts != NULL
        && pStreamState != NULL, STATUS_NULL_ARG);
    CHK(pFrame->duration > 0 && pFrame->size > 0 && pFrame->frameData != NULL, STATUS_MKV_INVALID_FRAME_DATA);

    // Calculate the timestamp - based on in stream or current time
    if (pStreamMkvGenerator->streamTimestamps) {
        dts = pFrame->decodingTs;
        pts = pFrame->presentationTs;
    } else {
        pts = dts = pStreamMkvGenerator->getTimeFn(pStreamMkvGenerator->customData);
    }

    // Ensure we have sane values
    CHK(dts <= MAX_TIMESTAMP_VALUE && pts <= MAX_TIMESTAMP_VALUE, STATUS_MKV_MAX_FRAME_TIMECODE);

    // Adjust to timescale - slight chance of an overflow if we didn't check for sane values
    pts = TIMESTAMP_TO_MKV_TIMECODE(pts, pStreamMkvGenerator->timecodeScale);
    dts = TIMESTAMP_TO_MKV_TIMECODE(dts, pStreamMkvGenerator->timecodeScale);

    CHK(pts >= pStreamMkvGenerator->lastClusterTimestamp && dts >= pStreamMkvGenerator->lastClusterTimestamp,
        STATUS_MKV_INVALID_FRAME_TIMESTAMP);

    // Evaluate the current state
    streamState = mkvgenGetStreamState(pStreamMkvGenerator, pFrame->flags, pts);

    // Ensure we have identical pts and dts on stream start or cluster start
    if (streamState != MKV_STATE_START_BLOCK) {
        CHK(pts == dts, STATUS_MKV_PTS_DTS_ARE_NOT_SAME);
    }

    // Set the return values
    *pPts = pts;
    *pDts = dts;
    *pStreamState = streamState;

CleanUp:
    return retStatus;
}

/**
 * Gets the state of the stream
 */
MKV_STREAM_STATE mkvgenGetStreamState(PStreamMkvGenerator pStreamMkvGenerator, FRAME_FLAGS flags, UINT64 timeCode)
{
    // Input params have been already validated
    if (!pStreamMkvGenerator->streamStarted) {
        // We haven't yet started the stream
        return MKV_STATE_START_STREAM;
    }

    // If we have a key frame we might or might not yet require a cluster
    if (CHECK_FRAME_FLAG_KEY_FRAME(flags)) {
        // if we are processing on key frame boundary then return just the cluster
        // or if enough time has passed since the last cluster start
        if (pStreamMkvGenerator->keyFrameClustering ||
                timeCode - pStreamMkvGenerator->lastClusterTimestamp >= pStreamMkvGenerator->clusterDuration) {
            return MKV_STATE_START_CLUSTER;
        }
    }

    // We should continue the same Block/Cluster
    return MKV_STATE_START_BLOCK;
}

/**
 * Gets a packaged size of a frame
 */
UINT32 mkvgenGetFrameOverhead(PStreamMkvGenerator pStreamMkvGenerator, MKV_STREAM_STATE streamState)
{
    UINT32 overhead = 0;

    switch(streamState) {
        case MKV_STATE_START_STREAM:
            overhead = MKV_HEADER_OVERHEAD + mkvgenGetHeaderOverhead(pStreamMkvGenerator);
            break;
        case MKV_STATE_START_CLUSTER:
            overhead = MKV_CLUSTER_OVERHEAD;
            break;
        case MKV_STATE_START_BLOCK:
            overhead = MKV_SIMPLE_BLOCK_OVERHEAD;
            break;
    }

    return overhead;
}

UINT32 mkvgenGetHeaderOverhead(PStreamMkvGenerator pStreamMkvGenerator)
{
    UINT32 encodedCpdSize = 0, encodedVideoConfig = 0;

    if (pStreamMkvGenerator->codecPrivateDataSize != 0) {
        mkvgenEbmlEncodeNumber(pStreamMkvGenerator->codecPrivateDataSize, NULL, 0, &encodedCpdSize);

        // Account for the element itself
        encodedCpdSize += MKV_CODEC_PRIVATE_DATA_ELEM_SIZE;

        // Account for the actual bit size too
        encodedCpdSize += pStreamMkvGenerator->codecPrivateDataSize;
    }

    if (GENERATE_VIDEO_CONFIG(pStreamMkvGenerator)) {
        encodedVideoConfig = MKV_TRACK_VIDEO_BITS_SIZE;
    }

    return encodedCpdSize + encodedVideoConfig;
}

/**
 * Returns the track type from the content type
 */
BYTE mkvgenGetTrackTypeFromContentType(PCHAR contentType)
{
    if (contentType != NULL && contentType[0] != '\0') {
        // Check if it starts with audio or video prefix (not counting the null terminator)
        if (0 == STRNCMP(contentType, MKV_CONTENT_TYPE_PREFIX_AUDIO, SIZEOF(MKV_CONTENT_TYPE_PREFIX_AUDIO) / SIZEOF(CHAR) - 1)) {
            return MKV_TRACK_TYPE_AUDIO;
        }

        if (0 == STRNCMP(contentType, MKV_CONTENT_TYPE_PREFIX_VIDEO, SIZEOF(MKV_CONTENT_TYPE_PREFIX_VIDEO) / SIZEOF(CHAR) - 1)) {
            return MKV_TRACK_TYPE_VIDEO;
        }
    }

    return MKV_DEFAULT_TRACK_TYPE;
}

/**
 * EBML encodes a number
 */
STATUS mkvgenEbmlEncodeNumber(UINT64 number, PBYTE pBuffer, UINT32 bufferSize, PUINT32 pEncodedLen) {
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 encoded = 0;
    UINT32 byteLen = 0;
    UINT32 i;

    CHK(pEncodedLen != NULL, STATUS_NULL_ARG);

    // The basic checks have been already performed
    // Check for unknown size first
    if ((INT64) number == -1LL) {
        encoded = 0xff;
        byteLen = 1;
    } else if (number >= 0x100000000000000ULL) {
        CHK(FALSE, STATUS_MKV_NUMBER_TOO_BIG);
    } else if (number < 0x80 - 1) {
        encoded = 0x80 | number;
        byteLen = 1;
    } else if (number < 0x4000 - 1) {
        encoded = 0x4000 | number;
        byteLen = 2;
    } else if (number < 0x200000 - 1) {
        encoded = 0x200000 | number;
        byteLen = 3;
    } else if (number < 0x10000000 - 1) {
        encoded = 0x10000000 | number;
        byteLen = 4;
    } else if (number < 0x800000000ULL - 1) {
        encoded = 0x800000000ULL | number;
        byteLen = 5;
    } else if (number < 0x40000000000ULL - 1) {
        encoded = 0x40000000000ULL | number;
        byteLen = 6;
    } else if (number < 0x2000000000000ULL - 1) {
        encoded = 0x2000000000000ULL | number;
        byteLen = 7;
    } else if (number < 0x100000000000000ULL - 1) {
        encoded = 0x100000000000000ULL | number;
        byteLen = 8;
    }

    // Store the byte len first if asked
    *pEncodedLen = byteLen;

    // Check if we have a buffer and if not - early exit
    CHK(pBuffer != NULL, retStatus);

    // Ensure we have enough buffer left
    CHK(bufferSize >= byteLen, STATUS_NOT_ENOUGH_MEMORY);
    for (i = byteLen; i > 0; i--) {
        *(pBuffer + i - 1) = (BYTE) encoded;
        encoded >>= 8;
    }

CleanUp:

    return retStatus;
}

/**
 * Stores a big-endian number
 */
STATUS mkvgenBigEndianNumber(UINT64 number, PBYTE pBuffer, UINT32 bufferSize, PUINT32 pEncodedLen)
{
    STATUS retStatus = STATUS_SUCCESS;
    BYTE bits;
    UINT32 i, byteLen;
    BOOL started = FALSE;
    BYTE storage[8];

    CHK(pEncodedLen != NULL, STATUS_NULL_ARG);

    // Store in the big-endian byte order
    if (number == 0) {
        // Quick case of 0
        storage[0] = 0;
        byteLen = 1;
    } else {
        for (i = 8, byteLen = 0; i > 0; i--) {
            bits = (BYTE) (number >> ((i - 1) * 8));
            if (started || bits != 0) {
                started = TRUE;
                storage[byteLen++] = bits;
            }
        }
    }

    // Store the byte len first if asked
    *pEncodedLen = byteLen;

    // Check if we have a buffer and if not - early exit
    CHK(pBuffer != NULL, retStatus);

    // Ensure we have enough buffer left
    CHK(bufferSize >= byteLen, STATUS_NOT_ENOUGH_MEMORY);

    // Copy the temp storage
    MEMCPY(pBuffer, storage, byteLen);

CleanUp:

    return retStatus;
}

/**
 * EBML encodes a header
 */
STATUS mkvgenEbmlEncodeHeader(PBYTE pBuffer, UINT32 bufferSize, PUINT32 pEncodedLen)
{
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pEncodedLen != NULL, STATUS_NULL_ARG);

    // Set the size first
    *pEncodedLen = MKV_HEADER_BITS_SIZE;

    // Quick return if we just need to calculate the size
    CHK(pBuffer != NULL, retStatus);

    // Check the buffer size
    CHK(bufferSize >= MKV_HEADER_BITS_SIZE, STATUS_NOT_ENOUGH_MEMORY);
    MEMCPY(pBuffer, MKV_HEADER_BITS, MKV_HEADER_BITS_SIZE);

CleanUp:

    return retStatus;
}

/**
 * EBML encodes a segment header
 */
STATUS mkvgenEbmlEncodeSegmentHeader(PBYTE pBuffer, UINT32 bufferSize, PUINT32 pEncodedLen)
{
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pEncodedLen != NULL, STATUS_NULL_ARG);

    // Set the size first
    *pEncodedLen = MKV_SEGMENT_HEADER_BITS_SIZE;

    // Quick return if we just need to calculate the size
    CHK(pBuffer != NULL, retStatus);

    // Check the buffer size
    CHK(bufferSize >= MKV_SEGMENT_HEADER_BITS_SIZE, STATUS_NOT_ENOUGH_MEMORY);
    MEMCPY(pBuffer, MKV_SEGMENT_HEADER_BITS, MKV_SEGMENT_HEADER_BITS_SIZE);

CleanUp:

    return retStatus;
}

/**
 * EBML encodes a segment info
 */
STATUS mkvgenEbmlEncodeSegmentInfo(PBYTE pBuffer, UINT32 bufferSize, UINT64 timecodeScale, PUINT32 pEncodedLen)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i;

    CHK(pEncodedLen != NULL, STATUS_NULL_ARG);

    // Set the size first
    *pEncodedLen = MKV_SEGMENT_INFO_BITS_SIZE;

    // Quick return if we just need to calculate the size
    CHK(pBuffer != NULL, retStatus);

    // Check the buffer size
    CHK(bufferSize >= MKV_SEGMENT_INFO_BITS_SIZE, STATUS_NOT_ENOUGH_MEMORY);
    MEMCPY(pBuffer, MKV_SEGMENT_INFO_BITS, MKV_SEGMENT_INFO_BITS_SIZE);

    // Fix up the segment UID
    for (i = 0; i < 16; i++) {
        *(pBuffer + MKV_SEGMENT_UID_OFFSET + i) = MKV_GEN_RANDOM_BYTE();
    }

    // Fix up the default timecode scale
    putInt64((PINT64)(pBuffer + MKV_SEGMENT_TIMECODE_SCALE_OFFSET), timecodeScale);

CleanUp:

    return retStatus;
}

/**
 * EBML encodes a track info
 */
STATUS mkvgenEbmlEncodeTrackInfo(PBYTE pBuffer, UINT32 bufferSize, PStreamMkvGenerator pMkvGenerator, PUINT32 pEncodedLen)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 i, encodedCpdLen = 0, cpdSize = 0, encodedLen = 0, encodedVideoConfig = 0, cpdOffset, videoOffset;

    CHK(pEncodedLen != NULL && pMkvGenerator != NULL, STATUS_NULL_ARG);

    if (pMkvGenerator->codecPrivateDataSize != 0) {
        mkvgenEbmlEncodeNumber(pMkvGenerator->codecPrivateDataSize, NULL, 0, &encodedCpdLen);

        // Account for the element itself
        encodedCpdLen += MKV_CODEC_PRIVATE_DATA_ELEM_SIZE;

        // Account for the actual bit size too
        encodedCpdLen += pMkvGenerator->codecPrivateDataSize;
    }

    if (GENERATE_VIDEO_CONFIG(pMkvGenerator)) {
        encodedVideoConfig = MKV_TRACK_VIDEO_BITS_SIZE;
    }

    // Set the size first
    *pEncodedLen = MKV_TRACK_INFO_BITS_SIZE + encodedCpdLen + encodedVideoConfig;

    // Quick return if we just need to calculate the size
    CHK(pBuffer != NULL, retStatus);

    // Check the buffer size
    CHK(bufferSize >= MKV_TRACK_INFO_BITS_SIZE + encodedCpdLen + encodedVideoConfig, STATUS_NOT_ENOUGH_MEMORY);
    MEMCPY(pBuffer, MKV_TRACK_INFO_BITS, MKV_TRACK_INFO_BITS_SIZE);

    // Fix-up the track type, name and codec id
    *(pBuffer + MKV_TRACK_TYPE_OFFSET) = pMkvGenerator->trackType;
    MEMCPY(pBuffer + MKV_CODEC_ID_OFFSET, pMkvGenerator->codecId, MKV_MAX_CODEC_ID_LEN);
    MEMCPY(pBuffer + MKV_TRACK_NAME_OFFSET, pMkvGenerator->trackName, MKV_MAX_TRACK_NAME_LEN);

    // Fix up the track UID
    for (i = 0; i < MKV_TRACK_ID_BYTE_SIZE; i++) {
        *(pBuffer + MKV_TRACK_ID_OFFSET + i) = MKV_GEN_RANDOM_BYTE();
    }

    // Append the video config if any
    if (GENERATE_VIDEO_CONFIG(pMkvGenerator)) {
        // Offset of the video right after the main structure
        videoOffset = MKV_TRACK_INFO_BITS_SIZE;
        // Copy the element first
        MEMCPY(pBuffer + videoOffset, MKV_TRACK_VIDEO_BITS, MKV_TRACK_VIDEO_BITS_SIZE);

        // Fix-up the width and height
        putInt16((PINT16)(pBuffer + videoOffset + MKV_TRACK_VIDEO_WIDTH_OFFSET), pMkvGenerator->videoWidth);
        putInt16((PINT16)(pBuffer + videoOffset + MKV_TRACK_VIDEO_HEIGHT_OFFSET), pMkvGenerator->videoHeight);

        // Important! Need to fix-up the overall track header element size and the track entry element size
        // Encode and fix-up the size - encode 4 bytes
        encodedLen = 0x10000000 | (UINT32) (videoOffset + MKV_TRACK_VIDEO_BITS_SIZE - MKV_TRACK_HEADER_SIZE_OFFSET - 4);
        putInt32((PINT32)(pBuffer + MKV_TRACK_HEADER_SIZE_OFFSET), encodedLen);

        encodedLen = 0x10000000 | (UINT32) (videoOffset + MKV_TRACK_VIDEO_BITS_SIZE - MKV_TRACK_ENTRY_SIZE_OFFSET - 4);
        putInt32((PINT32)(pBuffer + MKV_TRACK_ENTRY_SIZE_OFFSET), encodedLen);
    }

    // Append the codec private data if any
    if (pMkvGenerator->codecPrivateDataSize != 0 && pMkvGenerator->codecPrivateData != NULL) {
        // Offset of the CPD right after the main structure and the video config if any
        cpdOffset = MKV_CODEC_PRIVATE_DATA_OFFSET + encodedVideoConfig;
        // Copy the element first
        MEMCPY(pBuffer + cpdOffset, MKV_CODEC_PRIVATE_DATA_ELEM, MKV_CODEC_PRIVATE_DATA_ELEM_SIZE);
        cpdSize = MKV_CODEC_PRIVATE_DATA_ELEM_SIZE;

        // Encode the CPD size
        CHK_STATUS(mkvgenEbmlEncodeNumber(pMkvGenerator->codecPrivateDataSize,
                                          pBuffer + cpdOffset + cpdSize,
                                          bufferSize - cpdOffset - cpdSize,
                                          &encodedCpdLen));
        cpdSize += encodedCpdLen;

        CHK(cpdOffset + cpdSize + pMkvGenerator->codecPrivateDataSize <= bufferSize, STATUS_NOT_ENOUGH_MEMORY);

        // Copy the actual CPD bits
        MEMCPY(pBuffer + cpdOffset + cpdSize, pMkvGenerator->codecPrivateData, pMkvGenerator->codecPrivateDataSize);
        cpdSize += pMkvGenerator->codecPrivateDataSize;

        // Important! Need to fix-up the overall track header element size and the track entry element size
        // Encode and fix-up the size - encode 4 bytes
        encodedLen = 0x10000000 | (UINT32) (cpdSize + cpdOffset - MKV_TRACK_HEADER_SIZE_OFFSET - 4);
        putInt32((PINT32)(pBuffer + MKV_TRACK_HEADER_SIZE_OFFSET), encodedLen);

        encodedLen = 0x10000000 | (UINT32) (cpdSize + cpdOffset - MKV_TRACK_ENTRY_SIZE_OFFSET - 4);
        putInt32((PINT32)(pBuffer + MKV_TRACK_ENTRY_SIZE_OFFSET), encodedLen);
    }

CleanUp:

    return retStatus;
}

/**
 * EBML encodes a cluster
 */
STATUS mkvgenEbmlEncodeClusterInfo(PBYTE pBuffer, UINT32 bufferSize, UINT64 timestamp, PUINT32 pEncodedLen)
{
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pEncodedLen != NULL, STATUS_NULL_ARG);

    // Set the size first
    *pEncodedLen = MKV_CLUSTER_INFO_BITS_SIZE;

    // Quick return if we just need to calculate the size
    CHK(pBuffer != NULL, retStatus);

    // Check the buffer size
    CHK(bufferSize >= MKV_CLUSTER_INFO_BITS_SIZE, STATUS_NOT_ENOUGH_MEMORY);
    MEMCPY(pBuffer, MKV_CLUSTER_INFO_BITS, MKV_CLUSTER_INFO_BITS_SIZE);

    // Fix-up the cluster timecode
    putInt64((PINT64)(pBuffer + MKV_CLUSTER_TIMECODE_OFFSET), timestamp);

CleanUp:

    return retStatus;
}

/**
 * EBML encodes a simple block
 */
STATUS mkvgenEbmlEncodeSimpleBlock(PBYTE pBuffer, UINT32 bufferSize, INT16 timestamp, PFrame pFrame, UINT32 adaptedFrameSize, MKV_NALS_ADAPTATION nalsAdaptation, PUINT32 pEncodedLen)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 encodedLength;
    BYTE flags;
    UINT32 size;

    CHK(pEncodedLen != NULL && pFrame != NULL, STATUS_NULL_ARG);

    // Set the size first
    size = MKV_SIMPLE_BLOCK_BITS_SIZE + adaptedFrameSize;
    *pEncodedLen = size;

    // Quick return if we just need to calculate the size
    CHK(pBuffer != NULL, retStatus);

    // Check the buffer size
    CHK(bufferSize >= size, STATUS_NOT_ENOUGH_MEMORY);
    // Copy the header and the frame data
    MEMCPY(pBuffer, MKV_SIMPLE_BLOCK_BITS, MKV_SIMPLE_BLOCK_BITS_SIZE);

    switch (nalsAdaptation) {
        case MKV_NALS_ADAPT_NONE:
            // Just copy the bits
            MEMCPY(pBuffer + MKV_SIMPLE_BLOCK_BITS_SIZE, pFrame->frameData, adaptedFrameSize);
            break;

        case MKV_NALS_ADAPT_AVCC:
            // Copy the bits first
            MEMCPY(pBuffer + MKV_SIMPLE_BLOCK_BITS_SIZE, pFrame->frameData, adaptedFrameSize);

            // Adapt from Avcc to Annex-B nals
            CHK_STATUS(adaptFrameNalsFromAvccToAnnexB(pBuffer + MKV_SIMPLE_BLOCK_BITS_SIZE,
                                                      adaptedFrameSize));
            break;

        case MKV_NALS_ADAPT_ANNEXB:
            // Adapt from Annex-B to Avcc nals. NOTE: The conversion is not 'in-place'
            CHK_STATUS(adaptFrameNalsFromAnnexBToAvcc(pFrame->frameData,
                                                      pFrame->size,
                                                      pBuffer + MKV_SIMPLE_BLOCK_BITS_SIZE,
                                                      &adaptedFrameSize));
    }

    // Encode and fix-up the size - encode 8 bytes
    encodedLength = 0x100000000000000ULL | (UINT64) (adaptedFrameSize + MKV_SIMPLE_BLOCK_PAYLOAD_HEADER_SIZE);
    putInt64((PINT64)(pBuffer + MKV_SIMPLE_BLOCK_SIZE_OFFSET), encodedLength);

    // Fix up the timecode
    putInt16((PINT16)(pBuffer + MKV_SIMPLE_BLOCK_TIMECODE_OFFSET), timestamp);

    // Fix up the flags
    flags = MKV_SIMPLE_BLOCK_FLAGS_NONE;
    if (CHECK_FRAME_FLAG_KEY_FRAME(pFrame->flags)) {
        flags |= MKV_SIMPLE_BLOCK_KEY_FRAME_FLAG;
    }

    if (CHECK_FRAME_FLAG_DISCARDABLE_FRAME(pFrame->flags)) {
        flags |= MKV_SIMPLE_BLOCK_DISCARDABLE_FLAG;
    }

    if (CHECK_FRAME_FLAG_INVISIBLE_FRAME(pFrame->flags)) {
        flags |= MKV_SIMPLE_BLOCK_INVISIBLE_FLAG;
    }

    *(pBuffer + MKV_SIMPLE_BLOCK_FLAGS_OFFSET) = flags;

CleanUp:

    return retStatus;
}

STATUS getAdaptedFrameSize(PFrame pFrame, MKV_NALS_ADAPTATION nalsAdaptation, PUINT32 pAdaptedFrameSize)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 adaptedFrameSize = 0;

    CHK(pFrame != NULL && pAdaptedFrameSize != NULL, STATUS_NULL_ARG);

    // Calculate the adapted frame size
    switch (nalsAdaptation) {
        case MKV_NALS_ADAPT_NONE:
            // Explicit call-through as no bloating occurs
        case MKV_NALS_ADAPT_AVCC:
            // No bloat converting from AVCC to Annex-B format NALs
            adaptedFrameSize = pFrame->size;
            break;
        case MKV_NALS_ADAPT_ANNEXB:
            // Get the size after conversion
            CHK_STATUS(adaptFrameNalsFromAnnexBToAvcc(pFrame->frameData,
                                                      pFrame->size,
                                                      NULL,
                                                      &adaptedFrameSize));
            break;
    }

    // Set the return value
    *pAdaptedFrameSize = adaptedFrameSize;

CleanUp:

    return retStatus;
}

/**
 * Returns the number of bytes required to encode
 */
UINT32 mkvgenGetByteCount(UINT64 number)
{
    UINT32 count = 0;
    while (number != 0)
    {
        number >>= 8;
        count++;
    }

    // We should have at least one byte to encode 0 - special case
    return MAX(count, 1);
}

/**
 * Gettime adaptation callback
 */
UINT64 getTimeAdapter(UINT64 customData)
{
    UNUSED_PARAM(customData);
    return GETTIME();
}