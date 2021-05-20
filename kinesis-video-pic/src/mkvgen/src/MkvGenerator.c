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
STATUS createMkvGenerator(PCHAR contentType, UINT32 behaviorFlags, UINT64 timecodeScale, UINT64 clusterDuration, PBYTE segmentUuid,
                          PTrackInfo trackInfoList, UINT32 trackInfoCount, PCHAR clientId, GetCurrentTimeFunc getTimeFn, UINT64 customData,
                          PMkvGenerator* ppMkvGenerator)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStreamMkvGenerator pMkvGenerator = NULL;
    UINT32 allocationSize, i;
    BOOL adaptAnnexB = FALSE, adaptAvcc = FALSE, adaptCpdAnnexB = FALSE;
    PTrackInfo pSrcTrackInfo, pDstTrackInfo;

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
    CHK(STRNLEN(contentType, MAX_CONTENT_TYPE_LEN + 1) <= MAX_CONTENT_TYPE_LEN, STATUS_MKV_INVALID_CONTENT_TYPE_LENGTH);

    // Validate the client id string if specified
    CHK(clientId == NULL || STRNLEN(clientId, MAX_MKV_CLIENT_ID_STRING_LEN + 1) <= MAX_MKV_CLIENT_ID_STRING_LEN, STATUS_MKV_INVALID_CLIENT_ID_LENGTH);

    for (i = 0; i < trackInfoCount; ++i) {
        PTrackInfo pTrackInfo = &trackInfoList[i];
        CHK(STRNLEN(pTrackInfo->codecId, MKV_MAX_CODEC_ID_LEN + 1) <= MKV_MAX_CODEC_ID_LEN, STATUS_MKV_INVALID_CODEC_ID_LENGTH);
        CHK(STRNLEN(pTrackInfo->trackName, MKV_MAX_TRACK_NAME_LEN + 1) <= MKV_MAX_TRACK_NAME_LEN, STATUS_MKV_INVALID_TRACK_NAME_LENGTH);
        CHK(pTrackInfo->codecPrivateDataSize <= MKV_MAX_CODEC_PRIVATE_LEN, STATUS_MKV_INVALID_CODEC_PRIVATE_LENGTH);
        CHK(pTrackInfo->codecPrivateDataSize == 0 || pTrackInfo->codecPrivateData != NULL, STATUS_MKV_CODEC_PRIVATE_NULL);
        CHK(pTrackInfo->trackId != 0, STATUS_MKV_INVALID_TRACK_UID);
    }

    // Initialize the endianness for the library
    initializeEndianness();

    for (i = 0; i < trackInfoCount; ++i) {
        if (trackInfoList[i].codecPrivateDataSize != 0) {
            DLOGS("TrackName: %s, CodecId: %s", trackInfoList[i].trackName, trackInfoList[i].codecId);
            dumpMemoryHex(trackInfoList[i].codecPrivateData, trackInfoList[i].codecPrivateDataSize);
        }
    }

    // Allocate the main struct
    // NOTE: The calloc will Zero the fields
    allocationSize = SIZEOF(StreamMkvGenerator) + SIZEOF(TrackInfo) * trackInfoCount;
    pMkvGenerator = (PStreamMkvGenerator) MEMCALLOC(1, allocationSize);
    CHK(pMkvGenerator != NULL, STATUS_NOT_ENOUGH_MEMORY);

    // Set the values
    pMkvGenerator->mkvGenerator.version = 0;
    pMkvGenerator->timecodeScale = timecodeScale * DEFAULT_TIME_UNIT_IN_NANOS; // store in nanoseconds
    pMkvGenerator->clusterDuration =
        clusterDuration * DEFAULT_TIME_UNIT_IN_NANOS / pMkvGenerator->timecodeScale; // No chance of an overflow as we check against max earlier
    pMkvGenerator->contentType = mkvgenGetContentTypeFromContentTypeString(contentType);
    pMkvGenerator->generatorState = MKV_GENERATOR_STATE_START;
    pMkvGenerator->keyFrameClustering = (behaviorFlags & MKV_GEN_KEY_FRAME_PROCESSING) != MKV_GEN_FLAG_NONE;
    pMkvGenerator->streamTimestamps = (behaviorFlags & MKV_GEN_IN_STREAM_TIME) != MKV_GEN_FLAG_NONE;
    pMkvGenerator->absoluteTimeClusters = (behaviorFlags & MKV_GEN_ABSOLUTE_CLUSTER_TIME) != MKV_GEN_FLAG_NONE;
    pMkvGenerator->adaptCpdNals = adaptCpdAnnexB;
    pMkvGenerator->lastClusterPts = 0;
    pMkvGenerator->lastClusterDts = 0;
    pMkvGenerator->streamStartTimestamp = 0;
    pMkvGenerator->streamStartTimestampStored = FALSE;
    pMkvGenerator->trackInfoCount = trackInfoCount;

    // Package the version
    if (clientId != NULL && clientId[0] != '\0') {
        pMkvGenerator->versionSize =
            SNPRINTF(pMkvGenerator->version, SIZEOF(pMkvGenerator->version), (PCHAR) "%c%s%c%s", MKV_VERSION_STRING_DELIMITER,
                     MKV_GENERATOR_CURRENT_VERSION_STRING, MKV_VERSION_STRING_DELIMITER, clientId);
    } else {
        pMkvGenerator->versionSize = SNPRINTF(pMkvGenerator->version, SIZEOF(pMkvGenerator->version), (PCHAR) "%c%s", MKV_VERSION_STRING_DELIMITER,
                                              MKV_GENERATOR_CURRENT_VERSION_STRING);
    }

    // Copy TrackInfoList to the end of MkvGenerator struct
    pMkvGenerator->trackInfoList = (PTrackInfo)(pMkvGenerator + 1);
    MEMCPY(pMkvGenerator->trackInfoList, trackInfoList, SIZEOF(TrackInfo) * trackInfoCount);

    if (adaptAnnexB) {
        pMkvGenerator->nalsAdaptation = MKV_NALS_ADAPT_ANNEXB;
    } else if (adaptAvcc) {
        pMkvGenerator->nalsAdaptation = MKV_NALS_ADAPT_AVCC;
    } else {
        pMkvGenerator->nalsAdaptation = MKV_NALS_ADAPT_NONE;
    }

    // the getTime function is optional
    pMkvGenerator->getTimeFn = (getTimeFn != NULL) ? getTimeFn : getTimeAdapter;

    // Segment UUID has been zeroed already by calloc
    if (segmentUuid != NULL) {
        CHK(!checkBufferValues(segmentUuid, 0x00, MKV_SEGMENT_UUID_LEN), STATUS_MKV_INVALID_SEGMENT_UUID);
        MEMCPY(pMkvGenerator->segmentUuid, segmentUuid, MKV_SEGMENT_UUID_LEN);
    } else {
        MEMSET(pMkvGenerator->segmentUuid, MKV_SEGMENT_UUID_DEFAULT_VALUE, MKV_SEGMENT_UUID_LEN);
    }

    // Store the custom data as well
    pMkvGenerator->customData = customData;

    // Set the codec private data
    pDstTrackInfo = pMkvGenerator->trackInfoList;
    pSrcTrackInfo = trackInfoList;
    for (i = 0; i < trackInfoCount; i++, pDstTrackInfo++, pSrcTrackInfo++) {
        CHK_STATUS(mkvgenAdaptCodecPrivateData(pMkvGenerator, pSrcTrackInfo->trackType, pSrcTrackInfo->codecId, pSrcTrackInfo->codecPrivateDataSize,
                                               pSrcTrackInfo->codecPrivateData, &pDstTrackInfo->codecPrivateDataSize,
                                               &pDstTrackInfo->codecPrivateData, &pDstTrackInfo->trackCustomData));
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
    PStreamMkvGenerator pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    UINT32 i;

    // Call is idempotent
    CHK(pMkvGenerator != NULL, retStatus);
    for (i = 0; i < pStreamMkvGenerator->trackInfoCount; ++i) {
        if (pStreamMkvGenerator->trackInfoList[i].codecPrivateData != NULL) {
            MEMFREE(pStreamMkvGenerator->trackInfoList[i].codecPrivateData);
            pStreamMkvGenerator->trackInfoList[i].codecPrivateData = NULL;
        }

        pStreamMkvGenerator->trackInfoList[i].codecPrivateDataSize = 0;
    }

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
    pStreamMkvGenerator->generatorState = MKV_GENERATOR_STATE_START;

    // Reset the last cluster timestamp
    pStreamMkvGenerator->lastClusterPts = 0;
    pStreamMkvGenerator->lastClusterDts = 0;

    // Set the start timestamp to not stored
    pStreamMkvGenerator->streamStartTimestampStored = FALSE;

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Package frame in MKV format
 */
STATUS mkvgenPackageFrame(PMkvGenerator pMkvGenerator, PFrame pFrame, PTrackInfo pTrackInfo, PBYTE pBuffer, PUINT32 pSize,
                          PEncodedFrameInfo pEncodedFrameInfo)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStreamMkvGenerator pStreamMkvGenerator = NULL;
    MKV_STREAM_STATE streamState = MKV_STATE_START_STREAM;
    UINT32 bufferSize = 0, encodedLen = 0, packagedSize = 0, adaptedFrameSize = 0, overheadSize = 0, dataOffset = 0;
    // Evaluated presentation and decode timestamps
    UINT64 pts = 0, dts = 0, duration = 0;
    PBYTE pCurrentPnt = pBuffer;
    MKV_NALS_ADAPTATION nalsAdaptation;

    // Check the input params
    CHK(pSize != NULL && pMkvGenerator != NULL && pTrackInfo != NULL, STATUS_NULL_ARG);

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;

    // Validate and extract the timestamp
    CHK_STATUS(mkvgenValidateFrame(pStreamMkvGenerator, pFrame, pTrackInfo, &pts, &dts, &duration, &streamState));

    // Check to see if we can extract the CPD first.
    // Currently, we will process only if all of these conditions hold:
    // * NAL adaptation is specified from Annex-B
    // * CPD is not present yet
    // * We have a video track
    // * We have a key frame
    // * The content type is either H264 or H265
    if (pStreamMkvGenerator->nalsAdaptation == MKV_NALS_ADAPT_ANNEXB && pTrackInfo->codecPrivateDataSize == 0 &&
        pTrackInfo->trackType == MKV_TRACK_INFO_TYPE_VIDEO && CHECK_FRAME_FLAG_KEY_FRAME(pFrame->flags) &&
        (pStreamMkvGenerator->contentType & MKV_CONTENT_TYPE_H264_H265) != MKV_CONTENT_TYPE_NONE) {
        if (STATUS_FAILED(mkvgenExtractCpdFromAnnexBFrame(pStreamMkvGenerator, pFrame, pTrackInfo))) {
            DLOGW("Warning: Failed auto-extracting the CPD from the key frame.");
        }
    }

    // Calculate the necessary size

    // Get the overhead when packaging MKV
    overheadSize = mkvgenGetFrameOverhead(pStreamMkvGenerator, streamState);

    // NAL adaptation should only be done for video frames
    nalsAdaptation = pTrackInfo->trackType == MKV_TRACK_INFO_TYPE_VIDEO ? pStreamMkvGenerator->nalsAdaptation : MKV_NALS_ADAPT_NONE;

    // Get the adapted size of the frame and add to the overall size
    CHK_STATUS(getAdaptedFrameSize(pFrame, nalsAdaptation, &adaptedFrameSize));
    packagedSize = overheadSize + adaptedFrameSize;

    // Check if we are asked for size only and early return if so
    CHK(pBuffer != NULL, STATUS_SUCCESS);

    // Preliminary check for the buffer size
    CHK(*pSize >= packagedSize, STATUS_NOT_ENOUGH_MEMORY);

    // Start with the full buffer
    bufferSize = *pSize;

    // Generate the actual data
    switch (streamState) {
        case MKV_STATE_START_STREAM:
            if (pStreamMkvGenerator->generatorState == MKV_GENERATOR_STATE_START) {
                // Encode in sequence and subtract the size
                CHK_STATUS(mkvgenEbmlEncodeHeader(pCurrentPnt, bufferSize, &encodedLen));
                bufferSize -= encodedLen;
                pCurrentPnt += encodedLen;

                CHK_STATUS(mkvgenEbmlEncodeSegmentHeader(pCurrentPnt, bufferSize, &encodedLen));
                bufferSize -= encodedLen;
                pCurrentPnt += encodedLen;

                pStreamMkvGenerator->generatorState = MKV_GENERATOR_STATE_SEGMENT_HEADER;
            }

            if (pStreamMkvGenerator->generatorState == MKV_GENERATOR_STATE_SEGMENT_HEADER) {
                CHK_STATUS(mkvgenEbmlEncodeSegmentInfo(pStreamMkvGenerator, pCurrentPnt, bufferSize, &encodedLen));
                bufferSize -= encodedLen;
                pCurrentPnt += encodedLen;

                CHK_STATUS(mkvgenEbmlEncodeTrackInfo(pCurrentPnt, bufferSize, pStreamMkvGenerator, &encodedLen));
                bufferSize -= encodedLen;
                pCurrentPnt += encodedLen;

                pStreamMkvGenerator->generatorState = MKV_GENERATOR_STATE_CLUSTER_INFO;
            }

            // We are only interested in the header size in the data offset return
            dataOffset = overheadSize - mkvgenGetFrameOverhead(pStreamMkvGenerator, MKV_STATE_START_CLUSTER);

            // Fall-through
        case MKV_STATE_START_CLUSTER:
            // If we just added tags then we need to add the segment and track info
            if (pStreamMkvGenerator->generatorState == MKV_GENERATOR_STATE_SEGMENT_HEADER) {
                CHK_STATUS(mkvgenEbmlEncodeSegmentInfo(pStreamMkvGenerator, pCurrentPnt, bufferSize, &encodedLen));
                bufferSize -= encodedLen;
                pCurrentPnt += encodedLen;

                CHK_STATUS(mkvgenEbmlEncodeTrackInfo(pCurrentPnt, bufferSize, pStreamMkvGenerator, &encodedLen));
                bufferSize -= encodedLen;
                pCurrentPnt += encodedLen;

                pStreamMkvGenerator->generatorState = MKV_GENERATOR_STATE_CLUSTER_INFO;
            }

            // Store the stream timestamp if we started the stream
            if (!pStreamMkvGenerator->streamStartTimestampStored) {
                pStreamMkvGenerator->streamStartTimestamp = pts;
                pStreamMkvGenerator->streamStartTimestampStored = TRUE;
            }

            // Adjust the timestamp to the beginning of the stream if no absolute clustering
            CHK_STATUS(mkvgenEbmlEncodeClusterInfo(pCurrentPnt, bufferSize,
                                                   pStreamMkvGenerator->absoluteTimeClusters ? pts : pts - pStreamMkvGenerator->streamStartTimestamp,
                                                   &encodedLen));
            bufferSize -= encodedLen;
            pCurrentPnt += encodedLen;

            // Store the timestamp of the last cluster
            pStreamMkvGenerator->lastClusterPts = pts;
            pStreamMkvGenerator->lastClusterDts = dts;

            // indicate a cluster start
            pStreamMkvGenerator->generatorState = MKV_GENERATOR_STATE_CLUSTER_INFO;

            // Fall-through
        case MKV_STATE_START_BLOCK:
            // Ensure we are not in a TAGs state
            CHK(pStreamMkvGenerator->generatorState == MKV_GENERATOR_STATE_CLUSTER_INFO ||
                    pStreamMkvGenerator->generatorState == MKV_GENERATOR_STATE_SIMPLE_BLOCK,
                STATUS_MKV_INVALID_GENERATOR_STATE_TAGS);

            // Calculate the timestamp of the Frame relative to the cluster start
            if (pStreamMkvGenerator->generatorState == MKV_GENERATOR_STATE_CLUSTER_INFO) {
                pts = dts = 0;
            } else {
                // Make the timestamp relative
                pts -= pStreamMkvGenerator->lastClusterPts;
                dts -= pStreamMkvGenerator->lastClusterDts;
            }

            // The timecode for the frame has only 2 bytes which represent a signed int.
            CHK(pts <= MAX_INT16, STATUS_MKV_LARGE_FRAME_TIMECODE);

            // Adjust the timestamp to the start of the cluster
            CHK_STATUS(mkvgenEbmlEncodeSimpleBlock(pCurrentPnt, bufferSize, (INT16) pts, pFrame, nalsAdaptation, adaptedFrameSize,
                                                   pStreamMkvGenerator, &encodedLen));
            bufferSize -= encodedLen;
            pCurrentPnt += encodedLen;

            // Indicate a simple block
            pStreamMkvGenerator->generatorState = MKV_GENERATOR_STATE_SIMPLE_BLOCK;
            break;
    }

    // Validate the size
    CHK(packagedSize == (UINT32)(pCurrentPnt - pBuffer), STATUS_INTERNAL_ERROR);

CleanUp:

    if (STATUS_SUCCEEDED(retStatus)) {
        // Set the size and the state before return
        *pSize = packagedSize;

        if (pEncodedFrameInfo != NULL && pStreamMkvGenerator != NULL) {
            pEncodedFrameInfo->streamStartTs =
                MKV_TIMECODE_TO_TIMESTAMP(pStreamMkvGenerator->streamStartTimestamp, pStreamMkvGenerator->timecodeScale);
            pEncodedFrameInfo->clusterPts = MKV_TIMECODE_TO_TIMESTAMP(pStreamMkvGenerator->lastClusterPts, pStreamMkvGenerator->timecodeScale);
            pEncodedFrameInfo->clusterDts = MKV_TIMECODE_TO_TIMESTAMP(pStreamMkvGenerator->lastClusterDts, pStreamMkvGenerator->timecodeScale);
            pEncodedFrameInfo->framePts = MKV_TIMECODE_TO_TIMESTAMP(pts, pStreamMkvGenerator->timecodeScale);
            pEncodedFrameInfo->frameDts = MKV_TIMECODE_TO_TIMESTAMP(dts, pStreamMkvGenerator->timecodeScale);
            pEncodedFrameInfo->duration = MKV_TIMECODE_TO_TIMESTAMP(duration, pStreamMkvGenerator->timecodeScale);
            pEncodedFrameInfo->dataOffset = (UINT16) dataOffset;
            pEncodedFrameInfo->streamState = streamState;
        }
    }

    LEAVES();
    return retStatus;
}

/**
 * Sets codec private data for a track
 */
STATUS mkvgenSetCodecPrivateData(PMkvGenerator pMkvGenerator, UINT64 trackId, UINT32 codecPrivateDataSize, PBYTE codecPrivateData)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStreamMkvGenerator pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    UINT32 trackIndex;
    PTrackInfo pTrackInfo = NULL;

    // Check the input params
    CHK(pStreamMkvGenerator != NULL, STATUS_NULL_ARG);
    CHK(codecPrivateDataSize <= MKV_MAX_CODEC_PRIVATE_LEN, STATUS_MKV_INVALID_CODEC_PRIVATE_LENGTH);
    CHK(codecPrivateDataSize == 0 || codecPrivateData != NULL, STATUS_MKV_CODEC_PRIVATE_NULL);

    // Find the right track
    CHK_STATUS(mkvgenGetTrackInfo(pStreamMkvGenerator->trackInfoList, pStreamMkvGenerator->trackInfoCount, trackId, &pTrackInfo, &trackIndex));

    // Free the CPD if any
    if (pTrackInfo->codecPrivateData != NULL) {
        MEMFREE(pTrackInfo->codecPrivateData);
        pTrackInfo->codecPrivateData = NULL;
        pTrackInfo->codecPrivateDataSize = 0;
    }

    // See if we need to do anything
    CHK(codecPrivateDataSize != 0, retStatus);

    CHK_STATUS(mkvgenAdaptCodecPrivateData(pStreamMkvGenerator, pTrackInfo->trackType, pTrackInfo->codecId, codecPrivateDataSize, codecPrivateData,
                                           &pTrackInfo->codecPrivateDataSize, &pTrackInfo->codecPrivateData, &pTrackInfo->trackCustomData));

CleanUp:

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
    PStreamMkvGenerator pStreamMkvGenerator = NULL;
    UINT32 bufferSize, encodedLen, packagedSize = 0;
    PBYTE pCurrentPnt = pBuffer;

    // Check the input params
    CHK(pSize != NULL && pMkvGenerator != NULL, STATUS_NULL_ARG);

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;

    // Calculate the necessary size

    // Get the overhead when packaging MKV
    packagedSize = mkvgenGetMkvHeaderSize(pStreamMkvGenerator);

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

    CHK_STATUS(mkvgenEbmlEncodeSegmentInfo(pStreamMkvGenerator, pCurrentPnt, bufferSize, &encodedLen));
    bufferSize -= encodedLen;
    pCurrentPnt += encodedLen;

    CHK_STATUS(mkvgenEbmlEncodeTrackInfo(pCurrentPnt, bufferSize, pStreamMkvGenerator, &encodedLen));
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
 * Gets the generator timestamps
 */
STATUS mkvgenGetCurrentTimestamps(PMkvGenerator pMkvGenerator, PUINT64 pStreamStartTs, PUINT64 pClusterStartTs, PUINT64 pClusterStartDts)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStreamMkvGenerator pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;

    // Check the input params
    CHK(pStreamStartTs != NULL && pClusterStartTs != NULL && pMkvGenerator != NULL && pClusterStartDts != NULL, STATUS_NULL_ARG);

    *pStreamStartTs = MKV_TIMECODE_TO_TIMESTAMP(pStreamMkvGenerator->streamStartTimestamp, pStreamMkvGenerator->timecodeScale);
    *pClusterStartTs = MKV_TIMECODE_TO_TIMESTAMP(pStreamMkvGenerator->lastClusterPts, pStreamMkvGenerator->timecodeScale);
    *pClusterStartDts = MKV_TIMECODE_TO_TIMESTAMP(pStreamMkvGenerator->lastClusterDts, pStreamMkvGenerator->timecodeScale);

CleanUp:

    LEAVES();
    return retStatus;
}

/**
 * Packages MKV tag element
 */
STATUS mkvgenGenerateTag(PMkvGenerator pMkvGenerator, PBYTE pBuffer, PCHAR tagName, PCHAR tagValue, PUINT32 pSize)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStreamMkvGenerator pStreamMkvGenerator;
    UINT32 bufferSize, encodedLen, packagedSize = 0, tagNameLen, tagValueLen;
    UINT64 encodedElementLength;
    PBYTE pCurrentPnt = pBuffer;
    PBYTE pStartPnt = pBuffer;

    // Check the input params
    CHK(pSize != NULL && pMkvGenerator != NULL && tagName != NULL && tagValue != NULL, STATUS_NULL_ARG);
    CHK((tagNameLen = (UINT32) STRNLEN(tagName, MKV_MAX_TAG_NAME_LEN + 1)) <= MKV_MAX_TAG_NAME_LEN && tagNameLen > 0,
        STATUS_MKV_INVALID_TAG_NAME_LENGTH);
    CHK((tagValueLen = (UINT32) STRNLEN(tagValue, MKV_MAX_TAG_VALUE_LEN + 1)) <= MKV_MAX_TAG_VALUE_LEN, STATUS_MKV_INVALID_TAG_VALUE_LENGTH);

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;

    // Calculate the necessary size

    // Get the overhead when packaging MKV
    packagedSize = MKV_TAGS_BITS_SIZE + MKV_TAG_NAME_BITS_SIZE + MKV_TAG_STRING_BITS_SIZE + tagNameLen + tagValueLen;

    // Check if we are asked for size only and early return if so
    CHK(pBuffer != NULL, STATUS_SUCCESS);

    // Preliminary check for the buffer size
    CHK(*pSize >= packagedSize, STATUS_NOT_ENOUGH_MEMORY);

    // Start with the full buffer
    bufferSize = *pSize;

    // Check the size and copy the structure first
    encodedLen = MKV_TAGS_BITS_SIZE;
    CHK(bufferSize >= encodedLen, STATUS_NOT_ENOUGH_MEMORY);
    MEMCPY(pCurrentPnt, MKV_TAGS_BITS, encodedLen);
    bufferSize -= encodedLen;
    pCurrentPnt += encodedLen;

    // Copy the tag name
    encodedLen = MKV_TAG_NAME_BITS_SIZE;
    CHK(bufferSize >= encodedLen + tagNameLen, STATUS_NOT_ENOUGH_MEMORY);
    MEMCPY(pCurrentPnt, MKV_TAG_NAME_BITS, encodedLen);
    bufferSize -= encodedLen;
    pCurrentPnt += encodedLen;

    // Copy the name
    MEMCPY(pCurrentPnt, tagName, tagNameLen);

    // Reduce the size of the buffer
    bufferSize -= tagNameLen;
    pCurrentPnt += tagNameLen;

    // Copy the tag string value
    encodedLen = MKV_TAG_STRING_BITS_SIZE;
    CHK(bufferSize >= encodedLen + tagValueLen, STATUS_NOT_ENOUGH_MEMORY);
    MEMCPY(pCurrentPnt, MKV_TAG_STRING_BITS, encodedLen);
    bufferSize -= encodedLen;
    pCurrentPnt += encodedLen;

    // Copy the name
    MEMCPY(pCurrentPnt, tagValue, tagValueLen);

    // Reduce the size of the buffer
    bufferSize -= tagValueLen;
    pCurrentPnt += tagValueLen;

    // Fix-up the tags element size
    encodedElementLength = 0x100000000000000ULL | (UINT64)(packagedSize - MKV_TAG_ELEMENT_OFFSET);
    PUT_UNALIGNED_BIG_ENDIAN((PINT64)(pStartPnt + MKV_TAGS_ELEMENT_SIZE_OFFSET), encodedElementLength);

    // Fix-up the tag element size
    encodedElementLength = 0x100000000000000ULL | (UINT64)(packagedSize - MKV_SIMPLE_TAG_ELEMENT_OFFSET);
    PUT_UNALIGNED_BIG_ENDIAN((PINT64)(pStartPnt + MKV_TAG_ELEMENT_SIZE_OFFSET), encodedElementLength);

    // Fix-up the simple tag element size
    encodedElementLength = 0x100000000000000ULL | (UINT64)(packagedSize - MKV_TAGS_BITS_SIZE);
    PUT_UNALIGNED_BIG_ENDIAN((PINT64)(pStartPnt + MKV_SIMPLE_TAG_ELEMENT_SIZE_OFFSET), encodedElementLength);

    // Fix-up the tag name element size
    encodedElementLength = 0x100000000000000ULL | (UINT64)(tagNameLen);
    PUT_UNALIGNED_BIG_ENDIAN((PINT64)(pStartPnt + MKV_TAGS_BITS_SIZE + MKV_TAG_NAME_ELEMENT_SIZE_OFFSET), encodedElementLength);

    // Fix-up the tag string element size
    encodedElementLength = 0x100000000000000ULL | (UINT64)(tagValueLen);
    PUT_UNALIGNED_BIG_ENDIAN((PINT64)(pStartPnt + MKV_TAGS_BITS_SIZE + MKV_TAG_NAME_BITS_SIZE + tagNameLen + MKV_TAG_STRING_ELEMENT_SIZE_OFFSET),
                             encodedElementLength);

    // Validate the size
    CHK(packagedSize == (UINT32)(pCurrentPnt - pBuffer), STATUS_INTERNAL_ERROR);

CleanUp:

    if (STATUS_SUCCEEDED(retStatus)) {
        // Set the size and the state before return
        *pSize = packagedSize;
    }

    LEAVES();
    return retStatus;
}

/**
 * Gets the MKV overhead size
 */
STATUS mkvgenGetMkvOverheadSize(PMkvGenerator pMkvGenerator, MKV_STREAM_STATE mkvStreamState, PUINT32 pOverhead)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStreamMkvGenerator pStreamMkvGenerator;

    // Check the input params
    CHK(pOverhead != NULL && pMkvGenerator != NULL, STATUS_NULL_ARG);

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;

    *pOverhead = mkvgenGetFrameOverhead(pStreamMkvGenerator, mkvStreamState);

CleanUp:

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
STATUS mkvgenValidateFrame(PStreamMkvGenerator pStreamMkvGenerator, PFrame pFrame, PTrackInfo pTrackInfo, PUINT64 pPts, PUINT64 pDts,
                           PUINT64 pDuration, PMKV_STREAM_STATE pStreamState)
{
    UINT64 dts = 0, pts = 0, duration = 0;
    MKV_STREAM_STATE streamState;
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pStreamMkvGenerator != NULL && pFrame != NULL && pTrackInfo != NULL && pPts != NULL && pDts != NULL && pDuration != NULL &&
            pStreamState != NULL,
        STATUS_NULL_ARG);
    CHK(pFrame->size > 0 && pFrame->frameData != NULL && pFrame->trackId == pTrackInfo->trackId, STATUS_MKV_INVALID_FRAME_DATA);

    // Calculate the timestamp - based on in stream or current time
    if (pStreamMkvGenerator->streamTimestamps) {
        dts = pFrame->decodingTs;
        pts = pFrame->presentationTs;
    } else {
        pts = dts = pStreamMkvGenerator->getTimeFn(pStreamMkvGenerator->customData);
    }

    duration = pFrame->duration;

    // Ensure we have sane values
    CHK(dts <= MAX_TIMESTAMP_VALUE && pts <= MAX_TIMESTAMP_VALUE, STATUS_MKV_MAX_FRAME_TIMECODE);
    CHK(duration <= MAX_TIMESTAMP_VALUE, STATUS_MKV_MAX_FRAME_TIMECODE);

    // Adjust to timescale - slight chance of an overflow if we didn't check for sane values
    pts = TIMESTAMP_TO_MKV_TIMECODE(pts, pStreamMkvGenerator->timecodeScale);
    dts = TIMESTAMP_TO_MKV_TIMECODE(dts, pStreamMkvGenerator->timecodeScale);
    duration = TIMESTAMP_TO_MKV_TIMECODE(duration, pStreamMkvGenerator->timecodeScale);

    CHK(pts >= pStreamMkvGenerator->lastClusterPts && dts >= pStreamMkvGenerator->lastClusterDts, STATUS_MKV_INVALID_FRAME_TIMESTAMP);

    // Evaluate the current state
    streamState = mkvgenGetStreamState(pStreamMkvGenerator, pFrame->flags, pts);

    // Set the return values
    *pPts = pts;
    *pDts = dts;
    *pStreamState = streamState;
    *pDuration = duration;

CleanUp:
    return retStatus;
}

/**
 * Gets the state of the stream
 */
MKV_STREAM_STATE mkvgenGetStreamState(PStreamMkvGenerator pStreamMkvGenerator, FRAME_FLAGS flags, UINT64 timeCode)
{
    // Input params have been already validated
    if (pStreamMkvGenerator->generatorState == MKV_GENERATOR_STATE_START ||
        pStreamMkvGenerator->generatorState == MKV_GENERATOR_STATE_SEGMENT_HEADER) {
        // We haven't yet started the stream
        return MKV_STATE_START_STREAM;
    }

    // If we have a key frame we might or might not yet require a cluster
    if (CHECK_FRAME_FLAG_KEY_FRAME(flags)) {
        // if we are processing on key frame boundary then return just the cluster
        // or if enough time has passed since the last cluster start
        if (pStreamMkvGenerator->keyFrameClustering || timeCode - pStreamMkvGenerator->lastClusterPts >= pStreamMkvGenerator->clusterDuration) {
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

    switch (streamState) {
        case MKV_STATE_START_STREAM:
            if (pStreamMkvGenerator->generatorState == MKV_GENERATOR_STATE_SEGMENT_HEADER) {
                overhead = mkvgenGetMkvSegmentTrackInfoOverhead(pStreamMkvGenerator);
            } else {
                overhead = mkvgenGetMkvHeaderOverhead(pStreamMkvGenerator);
            }
            break;
        case MKV_STATE_START_CLUSTER:
            overhead += MKV_CLUSTER_OVERHEAD;
            break;
        case MKV_STATE_START_BLOCK:
            overhead = MKV_SIMPLE_BLOCK_OVERHEAD;
            break;
    }

    return overhead;
}

/**
 * Returns the track type from the content type
 */
MKV_CONTENT_TYPE mkvgenGetContentTypeFromContentTypeString(PCHAR contentTypeStr)
{
    PCHAR pStart = contentTypeStr, pEnd = contentTypeStr;
    UINT64 contentType = MKV_CONTENT_TYPE_NONE;

    // Quick check if we need to do anything
    if (contentTypeStr == NULL || contentTypeStr[0] == '\0') {
        return MKV_CONTENT_TYPE_NONE;
    }

    // Iterate until the end of the string and tokenize using the delimiter
    while (*pEnd != '\0') {
        if (*pEnd == MKV_CONTENT_TYPE_DELIMITER) {
            contentType |= mkvgenGetContentTypeFromContentTypeTokenString(pStart, (UINT32)(pEnd - pStart));
            pStart = pEnd + 1;
        }
        pEnd++;
    }

    // See if we have some more at the end
    if (pEnd != pStart) {
        contentType |= mkvgenGetContentTypeFromContentTypeTokenString(pStart, (UINT32)(pEnd - pStart));
    }

    return (MKV_CONTENT_TYPE) contentType;
}

/**
 * Return the content type from a content type token
 */
MKV_CONTENT_TYPE mkvgenGetContentTypeFromContentTypeTokenString(PCHAR contentTypeToken, UINT32 tokenLen)
{
    UINT32 typeStrLen;

    // Quick check if anything needs to be done
    if (tokenLen == 0 || contentTypeToken == NULL || *contentTypeToken == '\0') {
        return MKV_CONTENT_TYPE_NONE;
    }

    typeStrLen = (UINT32) STRLEN(MKV_H264_CONTENT_TYPE);
    if ((typeStrLen == tokenLen) && (0 == STRNCMP(contentTypeToken, MKV_H264_CONTENT_TYPE, tokenLen))) {
        return MKV_CONTENT_TYPE_H264;
    }

    typeStrLen = (UINT32) STRLEN(MKV_AAC_CONTENT_TYPE);
    if ((typeStrLen == tokenLen) && (0 == STRNCMP(contentTypeToken, MKV_AAC_CONTENT_TYPE, tokenLen))) {
        return MKV_CONTENT_TYPE_AAC;
    }

    typeStrLen = (UINT32) STRLEN(MKV_ALAW_CONTENT_TYPE);
    if ((typeStrLen == tokenLen) && (0 == STRNCMP(contentTypeToken, MKV_ALAW_CONTENT_TYPE, tokenLen))) {
        return MKV_CONTENT_TYPE_ALAW;
    }

    typeStrLen = (UINT32) STRLEN(MKV_MULAW_CONTENT_TYPE);
    if ((typeStrLen == tokenLen) && (0 == STRNCMP(contentTypeToken, MKV_MULAW_CONTENT_TYPE, tokenLen))) {
        return MKV_CONTENT_TYPE_MULAW;
    }

    typeStrLen = (UINT32) STRLEN(MKV_H265_CONTENT_TYPE);
    if ((typeStrLen == tokenLen) && (0 == STRNCMP(contentTypeToken, MKV_H265_CONTENT_TYPE, tokenLen))) {
        return MKV_CONTENT_TYPE_H265;
    }

    typeStrLen = (UINT32) STRLEN(MKV_X_MKV_VIDEO_CONTENT_TYPE);
    if ((typeStrLen == tokenLen) && (0 == STRNCMP(contentTypeToken, MKV_X_MKV_VIDEO_CONTENT_TYPE, tokenLen))) {
        return MKV_CONTENT_TYPE_X_MKV_VIDEO;
    }

    typeStrLen = (UINT32) STRLEN(MKV_X_MKV_AUDIO_CONTENT_TYPE);
    if ((typeStrLen == tokenLen) && (0 == STRNCMP(contentTypeToken, MKV_X_MKV_AUDIO_CONTENT_TYPE, tokenLen))) {
        return MKV_CONTENT_TYPE_X_MKV_AUDIO;
    }

    return MKV_CONTENT_TYPE_UNKNOWN;
}

/**
 * EBML encodes a number
 */
STATUS mkvgenEbmlEncodeNumber(UINT64 number, PBYTE pBuffer, UINT32 bufferSize, PUINT32 pEncodedLen)
{
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
            bits = (BYTE)(number >> ((i - 1) * 8));
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
STATUS mkvgenEbmlEncodeSegmentInfo(PStreamMkvGenerator pStreamMkvGenerator, PBYTE pBuffer, UINT32 bufferSize, PUINT32 pEncodedLen)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 size = 0, encodedLen;
    PBYTE pCurPtr = pBuffer;

    CHK(pEncodedLen != NULL && pStreamMkvGenerator != NULL, STATUS_NULL_ARG);

    // Set the size first
    size = MKV_SEGMENT_INFO_BITS_SIZE;

    // Account for the title
    size += MKV_TITLE_BITS_SIZE + MKV_GENERATOR_APPLICATION_NAME_STRING_SIZE;

    // Account for the muxing app
    size += MKV_MUXING_APP_BITS_SIZE + MKV_GENERATOR_APPLICATION_NAME_STRING_SIZE + pStreamMkvGenerator->versionSize;

    // Account for the writing app
    size += MKV_WRITING_APP_BITS_SIZE + MKV_GENERATOR_APPLICATION_NAME_STRING_SIZE + pStreamMkvGenerator->versionSize;

    // Internal check - ensure the size is less than what can be accommodated in 2 bytes
    CHK(size < 0x4000 - 1, STATUS_INTERNAL_ERROR);

    // Quick return if we just need to calculate the size
    CHK(pBuffer != NULL, retStatus);

    // Check the buffer size
    CHK(bufferSize >= size, STATUS_NOT_ENOUGH_MEMORY);

    // Copy the main structure
    MEMCPY(pCurPtr, MKV_SEGMENT_INFO_BITS, MKV_SEGMENT_INFO_BITS_SIZE);
    pCurPtr += MKV_SEGMENT_INFO_BITS_SIZE;

    // Copy the title
    MEMCPY(pCurPtr, MKV_TITLE_BITS, MKV_TITLE_BITS_SIZE);
    MEMCPY(pCurPtr + MKV_TITLE_BITS_SIZE, MKV_GENERATOR_APPLICATION_NAME_STRING, MKV_GENERATOR_APPLICATION_NAME_STRING_SIZE);

    // Fix-up the title size
    *(pCurPtr + MKV_APP_ELEMENT_SIZE_OFFSET) = (UINT8)(0x80 | (UINT8)(MKV_GENERATOR_APPLICATION_NAME_STRING_SIZE));

    // Current should be advanced by the bits size + the string itself
    pCurPtr += MKV_TITLE_BITS_SIZE + MKV_GENERATOR_APPLICATION_NAME_STRING_SIZE;

    // Copy the muxing app
    MEMCPY(pCurPtr, MKV_MUXING_APP_BITS, MKV_MUXING_APP_BITS_SIZE);
    MEMCPY(pCurPtr + MKV_MUXING_APP_BITS_SIZE, MKV_GENERATOR_APPLICATION_NAME_STRING, MKV_GENERATOR_APPLICATION_NAME_STRING_SIZE);
    MEMCPY(pCurPtr + MKV_MUXING_APP_BITS_SIZE + MKV_GENERATOR_APPLICATION_NAME_STRING_SIZE, pStreamMkvGenerator->version,
           pStreamMkvGenerator->versionSize);

    // Fix-up the title size
    *(pCurPtr + MKV_APP_ELEMENT_SIZE_OFFSET) = (UINT8)(0x80 | (UINT8)(MKV_GENERATOR_APPLICATION_NAME_STRING_SIZE + pStreamMkvGenerator->versionSize));

    // Current should be advanced by the bits size + the string itself
    pCurPtr += MKV_MUXING_APP_BITS_SIZE + MKV_GENERATOR_APPLICATION_NAME_STRING_SIZE + pStreamMkvGenerator->versionSize;

    // Copy the writing app
    MEMCPY(pCurPtr, MKV_WRITING_APP_BITS, MKV_WRITING_APP_BITS_SIZE);
    MEMCPY(pCurPtr + MKV_WRITING_APP_BITS_SIZE, MKV_GENERATOR_APPLICATION_NAME_STRING, MKV_GENERATOR_APPLICATION_NAME_STRING_SIZE);
    MEMCPY(pCurPtr + MKV_WRITING_APP_BITS_SIZE + MKV_GENERATOR_APPLICATION_NAME_STRING_SIZE, pStreamMkvGenerator->version,
           pStreamMkvGenerator->versionSize);

    // Fix-up the title size
    *(pCurPtr + MKV_APP_ELEMENT_SIZE_OFFSET) = (UINT8)(0x80 | (UINT8)(MKV_GENERATOR_APPLICATION_NAME_STRING_SIZE + pStreamMkvGenerator->versionSize));

    // Current should be advanced by the bits size + the string itself
    pCurPtr += MKV_WRITING_APP_BITS_SIZE + MKV_GENERATOR_APPLICATION_NAME_STRING_SIZE + pStreamMkvGenerator->versionSize;

    // Fix up the segment UID
    MEMCPY(pBuffer + MKV_SEGMENT_UID_OFFSET, pStreamMkvGenerator->segmentUuid, MKV_SEGMENT_UUID_LEN);

    // Fix up the default timecode scale
    PUT_UNALIGNED_BIG_ENDIAN((PINT64)(pBuffer + MKV_SEGMENT_TIMECODE_SCALE_OFFSET), pStreamMkvGenerator->timecodeScale);

    // Validate the size in case we have miscalculated
    CHK((UINT64) pCurPtr == (UINT64) pBuffer + size, STATUS_INTERNAL_ERROR);

    // Encode and fix-up the size - encode 2 bytes
    encodedLen = 0x4000 | (size - MKV_SEGMENT_INFO_HEADER_SIZE);
    PUT_UNALIGNED_BIG_ENDIAN((PINT16)(pBuffer + MKV_SEGMENT_INFO_SIZE_OFFSET), (UINT16) encodedLen);

CleanUp:

    if (pEncodedLen != NULL) {
        *pEncodedLen = size;
    }

    return retStatus;
}

/**
 * EBML encodes a track info
 */
STATUS mkvgenEbmlEncodeTrackInfo(PBYTE pBuffer, UINT32 bufferSize, PStreamMkvGenerator pMkvGenerator, PUINT32 pEncodedLen)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 j, encodedCpdLen = 0, cpdSize = 0, encodedLen = 0, cpdOffset = 0;
    UINT32 mkvTrackDataSize = 0, mkvTracksDataSize = 0, codecIdDataSize = 0, trackNameDataSize = 0;
    PBYTE pTrackStart = NULL;
    UINT32 trackSpecificDataOffset = 0, mkvAudioBitsSize = 0;
    PTrackInfo pTrackInfo = NULL;

    CHK(pEncodedLen != NULL && pMkvGenerator != NULL, STATUS_NULL_ARG);

    // Set the size first
    *pEncodedLen = mkvgenGetMkvTrackHeaderSize(pMkvGenerator->trackInfoList, pMkvGenerator->trackInfoCount);

    // Quick return if we just need to calculate the size
    CHK(pBuffer != NULL, retStatus);

    CHK(bufferSize >= *pEncodedLen, STATUS_NOT_ENOUGH_MEMORY);

    // Copy over Tracks Header bytes (element + size)
    MEMCPY(pBuffer, MKV_TRACKS_ELEM_BITS, MKV_TRACKS_ELEM_BITS_SIZE);
    // Save the beginning of TrackEntry element
    pTrackStart = pBuffer + MKV_TRACK_ENTRY_OFFSET;

    for (j = 0; j < pMkvGenerator->trackInfoCount; ++j) {
        pTrackInfo = &pMkvGenerator->trackInfoList[j];

        MEMCPY(pTrackStart, MKV_TRACK_INFO_BITS, MKV_TRACK_INFO_BITS_SIZE);
        mkvTrackDataSize += MKV_TRACK_INFO_BITS_SIZE - MKV_TRACK_ENTRY_HEADER_BITS_SIZE;
        // Offset into available spots to store elements not in default TrackEntry template.
        // Such elements include audio, video, cpd.
        trackSpecificDataOffset = MKV_TRACK_INFO_BITS_SIZE;

        // Fix-up the track type
        *(pTrackStart + MKV_TRACK_TYPE_OFFSET) = (BYTE) pTrackInfo->trackType;

        // Package track name
        trackNameDataSize = (UINT32) STRNLEN(pTrackInfo->trackName, MKV_MAX_TRACK_NAME_LEN);
        if (trackNameDataSize > 0) {
            MEMCPY(pTrackStart + trackSpecificDataOffset, MKV_TRACK_NAME_BITS, MKV_TRACK_NAME_BITS_SIZE);
            trackSpecificDataOffset += MKV_TRACK_NAME_BITS_SIZE;

            // Encode the track name
            CHK_STATUS(
                mkvgenEbmlEncodeNumber(trackNameDataSize, pTrackStart + trackSpecificDataOffset, bufferSize - trackSpecificDataOffset, &encodedLen));
            trackSpecificDataOffset += encodedLen;

            MEMCPY(pTrackStart + trackSpecificDataOffset, pTrackInfo->trackName, trackNameDataSize);
            trackSpecificDataOffset += trackNameDataSize;
            mkvTrackDataSize += MKV_TRACK_NAME_BITS_SIZE + encodedLen + trackNameDataSize;
        }
        // done packaging track name

        // Package codec id
        codecIdDataSize = (UINT32) STRNLEN(pTrackInfo->codecId, MKV_MAX_CODEC_ID_LEN);
        if (codecIdDataSize > 0) {
            MEMCPY(pTrackStart + trackSpecificDataOffset, MKV_CODEC_ID_BITS, MKV_CODEC_ID_BITS_SIZE);
            trackSpecificDataOffset += MKV_CODEC_ID_BITS_SIZE;

            // Encode the codec id size
            CHK_STATUS(
                mkvgenEbmlEncodeNumber(codecIdDataSize, pTrackStart + trackSpecificDataOffset, bufferSize - trackSpecificDataOffset, &encodedLen));
            trackSpecificDataOffset += encodedLen;
            mkvTrackDataSize += MKV_CODEC_ID_BITS_SIZE + encodedLen + codecIdDataSize;
            MEMCPY(pTrackStart + trackSpecificDataOffset, pTrackInfo->codecId, codecIdDataSize);
            trackSpecificDataOffset += codecIdDataSize;
        }
        // done packaging codec id

        // Fix up track number. Use trackInfo's index as mkv track number. Mkv track number starts from 1
        *(pTrackStart + MKV_TRACK_NUMBER_OFFSET) = (BYTE)(j + 1);

        // Fix up the track UID
        PUT_UNALIGNED_BIG_ENDIAN((PINT64)(pTrackStart + MKV_TRACK_ID_OFFSET), pTrackInfo->trackId);

        // Append the video config if any
        if (GENERATE_VIDEO_CONFIG(&pMkvGenerator->trackInfoList[j])) {
            // Record the amount of storage used in order to skip to next track later
            mkvTrackDataSize += MKV_TRACK_VIDEO_BITS_SIZE;

            // Copy the element first
            MEMCPY(pTrackStart + trackSpecificDataOffset, MKV_TRACK_VIDEO_BITS, MKV_TRACK_VIDEO_BITS_SIZE);

            // Fix-up the width and height
            PUT_UNALIGNED_BIG_ENDIAN((PINT16)(pTrackStart + trackSpecificDataOffset + MKV_TRACK_VIDEO_WIDTH_OFFSET),
                                     pTrackInfo->trackCustomData.trackVideoConfig.videoWidth);
            PUT_UNALIGNED_BIG_ENDIAN((PINT16)(pTrackStart + trackSpecificDataOffset + MKV_TRACK_VIDEO_HEIGHT_OFFSET),
                                     pTrackInfo->trackCustomData.trackVideoConfig.videoHeight);

            trackSpecificDataOffset += MKV_TRACK_VIDEO_BITS_SIZE;

        } else if (GENERATE_AUDIO_CONFIG(&pMkvGenerator->trackInfoList[j])) {
            mkvAudioBitsSize = mkvgenGetMkvAudioBitsSize(&pMkvGenerator->trackInfoList[j]);

            // Record the amount of storage used in order to skip to next track later
            mkvTrackDataSize += mkvAudioBitsSize;

            // Copy the element first
            MEMCPY(pTrackStart + trackSpecificDataOffset, MKV_TRACK_AUDIO_BITS, MKV_TRACK_AUDIO_BITS_SIZE);

            PUT_UNALIGNED_BIG_ENDIAN((PINT64)(pTrackStart + trackSpecificDataOffset + MKV_TRACK_AUDIO_SAMPLING_RATE_OFFSET),
                                     *((PINT64)(&pTrackInfo->trackCustomData.trackAudioConfig.samplingFrequency)));
            *(pTrackStart + trackSpecificDataOffset + MKV_TRACK_AUDIO_CHANNELS_OFFSET) =
                (UINT8) pTrackInfo->trackCustomData.trackAudioConfig.channelConfig;

            if (pMkvGenerator->trackInfoList[j].trackCustomData.trackAudioConfig.bitDepth != 0) {
                MEMCPY(pTrackStart + trackSpecificDataOffset + MKV_TRACK_AUDIO_BITS_SIZE, MKV_TRACK_AUDIO_BIT_DEPTH_BITS,
                       MKV_TRACK_AUDIO_BIT_DEPTH_BITS_SIZE);
                *(pTrackStart + trackSpecificDataOffset + MKV_TRACK_AUDIO_BIT_DEPTH_OFFSET) =
                    (UINT8) pTrackInfo->trackCustomData.trackAudioConfig.bitDepth;

                // fix up audio element data size
                encodedLen = (0x10000000 | (UINT32)(mkvAudioBitsSize)) - MKV_TRACK_AUDIO_EBML_HEADER_SIZE;
                // +1 to skip the audio element
                PUT_UNALIGNED_BIG_ENDIAN((PINT32)(pTrackStart + trackSpecificDataOffset + 1), encodedLen);
            }

            trackSpecificDataOffset += mkvAudioBitsSize;
        }

        // Append the codec private data if any
        if (pTrackInfo->codecPrivateDataSize != 0 && pTrackInfo->codecPrivateData != NULL) {
            // Offset of the CPD right after the main structure and the video config if any
            cpdOffset = trackSpecificDataOffset;

            // Copy the element first
            MEMCPY(pTrackStart + cpdOffset, MKV_CODEC_PRIVATE_DATA_ELEM, MKV_CODEC_PRIVATE_DATA_ELEM_SIZE);
            cpdSize = MKV_CODEC_PRIVATE_DATA_ELEM_SIZE;

            // Encode the CPD size
            CHK_STATUS(mkvgenEbmlEncodeNumber(pTrackInfo->codecPrivateDataSize, pTrackStart + cpdOffset + cpdSize, bufferSize - cpdOffset - cpdSize,
                                              &encodedCpdLen));
            cpdSize += encodedCpdLen;

            CHK(cpdOffset + cpdSize + pTrackInfo->codecPrivateDataSize <= bufferSize, STATUS_NOT_ENOUGH_MEMORY);

            // Copy the actual CPD bits
            MEMCPY(pTrackStart + cpdOffset + cpdSize, pTrackInfo->codecPrivateData, pTrackInfo->codecPrivateDataSize);
            cpdSize += pTrackInfo->codecPrivateDataSize;
            mkvTrackDataSize += cpdSize;
        }

        // Important! Need to fix-up the overall track header element size and the track entry element size
        // Encode and fix-up the size - encode 4 bytes
        encodedLen = 0x10000000 | (UINT32)(mkvTrackDataSize);
        PUT_UNALIGNED_BIG_ENDIAN((PINT32)(pTrackStart + MKV_TRACK_ENTRY_SIZE_OFFSET), encodedLen);

        // Need to add back TrackEntry header bits to skip to the end of current TrackEntry
        pTrackStart += mkvTrackDataSize + MKV_TRACK_ENTRY_HEADER_BITS_SIZE;
        // TrackEntry header bits size also need to be accounted when calculating overall Tracks size
        mkvTracksDataSize += mkvTrackDataSize + MKV_TRACK_ENTRY_HEADER_BITS_SIZE;
        // Reset mkvTrackDataSize as we are starting for a new track
        mkvTrackDataSize = 0;
    }
    // Important! Need to fix-up the overall track header element size and the track entry element size
    // Encode and fix-up the size - encode 4 bytes
    encodedLen = 0x10000000 | (UINT32)(mkvTracksDataSize);
    PUT_UNALIGNED_BIG_ENDIAN((PINT32)(pBuffer + MKV_TRACK_HEADER_SIZE_OFFSET), encodedLen);

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
    PUT_UNALIGNED_BIG_ENDIAN((PINT64)(pBuffer + MKV_CLUSTER_TIMECODE_OFFSET), timestamp);

CleanUp:

    return retStatus;
}

/**
 * EBML encodes a simple block
 */
STATUS mkvgenEbmlEncodeSimpleBlock(PBYTE pBuffer, UINT32 bufferSize, INT16 timestamp, PFrame pFrame, MKV_NALS_ADAPTATION nalsAdaptation,
                                   UINT32 adaptedFrameSize, PStreamMkvGenerator pStreamMkvGenerator, PUINT32 pEncodedLen)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 encodedLength;
    BYTE flags;
    UINT32 size, trackIndex;

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
            CHK_STATUS(adaptFrameNalsFromAvccToAnnexB(pBuffer + MKV_SIMPLE_BLOCK_BITS_SIZE, adaptedFrameSize));
            break;

        case MKV_NALS_ADAPT_ANNEXB:
            // Adapt from Annex-B to Avcc nals. NOTE: The conversion is not 'in-place'
            CHK_STATUS(
                adaptFrameNalsFromAnnexBToAvcc(pFrame->frameData, pFrame->size, FALSE, pBuffer + MKV_SIMPLE_BLOCK_BITS_SIZE, &adaptedFrameSize));
    }

    // Encode and fix-up the size - encode 8 bytes
    encodedLength = 0x100000000000000ULL | (UINT64)(adaptedFrameSize + MKV_SIMPLE_BLOCK_PAYLOAD_HEADER_SIZE);
    PUT_UNALIGNED_BIG_ENDIAN((PINT64)(pBuffer + MKV_SIMPLE_BLOCK_SIZE_OFFSET), encodedLength);

    // Fix up the timecode
    PUT_UNALIGNED_BIG_ENDIAN((PINT16)(pBuffer + MKV_SIMPLE_BLOCK_TIMECODE_OFFSET), timestamp);

    // track must exist because we already checked in putKinesisVideoFrame
    CHK_STATUS(mkvgenGetTrackInfo(pStreamMkvGenerator->trackInfoList, pStreamMkvGenerator->trackInfoCount, pFrame->trackId, NULL, &trackIndex));

    // fix up track number for each block
    *(pBuffer + MKV_SIMPLE_BLOCK_TRACK_NUMBER_OFFSET) = (UINT8)(0x80 | (UINT8)(trackIndex + 1));

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
            CHK_STATUS(adaptFrameNalsFromAnnexBToAvcc(pFrame->frameData, pFrame->size, FALSE, NULL, &adaptedFrameSize));
            break;
    }

    // Set the return value
    *pAdaptedFrameSize = adaptedFrameSize;

CleanUp:

    return retStatus;
}

UINT32 mkvgenGetTrackEntrySize(PTrackInfo pTrackInfo)
{
    UINT32 trackEntrySize = MKV_TRACK_INFO_BITS_SIZE, dataSize = 0, encodedLen = 0;

    if (pTrackInfo->codecPrivateDataSize != 0) {
        mkvgenEbmlEncodeNumber(pTrackInfo->codecPrivateDataSize, NULL, 0, &encodedLen);

        // Add size of the cpd element, the ebml size, and actual data size
        trackEntrySize += MKV_CODEC_PRIVATE_DATA_ELEM_SIZE + encodedLen + pTrackInfo->codecPrivateDataSize;
    }

    dataSize = (UINT32) STRNLEN(pTrackInfo->codecId, MKV_MAX_CODEC_ID_LEN);
    if (dataSize > 0) {
        mkvgenEbmlEncodeNumber(dataSize, NULL, 0, &encodedLen);
        trackEntrySize += MKV_CODEC_ID_BITS_SIZE + encodedLen + dataSize;
    }

    dataSize = (UINT32) STRNLEN(pTrackInfo->trackName, MKV_MAX_TRACK_NAME_LEN);
    if (dataSize > 0) {
        mkvgenEbmlEncodeNumber(dataSize, NULL, 0, &encodedLen);
        trackEntrySize += MKV_TRACK_NAME_BITS_SIZE + encodedLen + dataSize;
    }

    if (GENERATE_AUDIO_CONFIG(pTrackInfo)) {
        trackEntrySize += mkvgenGetMkvAudioBitsSize(pTrackInfo);
    } else if (GENERATE_VIDEO_CONFIG(pTrackInfo)) {
        trackEntrySize += MKV_TRACK_VIDEO_BITS_SIZE;
    }

    return trackEntrySize;
}

UINT32 mkvgenGetMkvTrackHeaderSize(PTrackInfo trackInfoList, UINT32 trackInfoCount)
{
    UINT32 trackHeaderSize = MKV_TRACKS_ELEM_BITS_SIZE, i = 0;

    for (i = 0; i < trackInfoCount; ++i) {
        trackHeaderSize += mkvgenGetTrackEntrySize(&trackInfoList[i]);
    }

    return trackHeaderSize;
}

UINT32 mkvgenGetMkvSegmentTrackHeaderSize(PStreamMkvGenerator pStreamMkvGenerator)
{
    UINT32 segmentInfoLen;
    mkvgenEbmlEncodeSegmentInfo(pStreamMkvGenerator, NULL, 0, &segmentInfoLen);
    return segmentInfoLen + mkvgenGetMkvTrackHeaderSize(pStreamMkvGenerator->trackInfoList, pStreamMkvGenerator->trackInfoCount);
}

UINT32 mkvgenGetMkvHeaderSize(PStreamMkvGenerator pStreamMkvGenerator)
{
    return MKV_EBML_SEGMENT_SIZE + mkvgenGetMkvSegmentTrackHeaderSize(pStreamMkvGenerator);
}

UINT32 mkvgenGetMkvHeaderOverhead(PStreamMkvGenerator pStreamMkvGenerator)
{
    return MKV_CLUSTER_OVERHEAD + mkvgenGetMkvHeaderSize(pStreamMkvGenerator);
}

UINT32 mkvgenGetMkvSegmentTrackInfoOverhead(PStreamMkvGenerator pStreamMkvGenerator)
{
    return MKV_CLUSTER_OVERHEAD + mkvgenGetMkvSegmentTrackHeaderSize(pStreamMkvGenerator);
}

UINT32 mkvgenGetMkvAudioBitsSize(PTrackInfo pTrackInfo)
{
    return pTrackInfo->trackCustomData.trackAudioConfig.bitDepth == 0 ? MKV_TRACK_AUDIO_BITS_SIZE
                                                                      : MKV_TRACK_AUDIO_BITS_SIZE + MKV_TRACK_AUDIO_BIT_DEPTH_BITS_SIZE;
}

/**
 * Returns the number of bytes required to encode
 */
UINT32 mkvgenGetByteCount(UINT64 number)
{
    UINT32 count = 0;
    while (number != 0) {
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

/**
 * Parse AAC Cpd
 */
STATUS getSamplingFreqAndChannelFromAacCpd(PBYTE pCpd, UINT32 cpdSize, PDOUBLE pSamplingFrequency, PUINT16 pChannelConfig)
{
    STATUS retStatus = STATUS_SUCCESS;
    INT16 cpdContainer;
    UINT16 samplingRateIdx, channelConfig;

    CHK(pSamplingFrequency != NULL && pChannelConfig != NULL, STATUS_NULL_ARG);
    CHK(cpdSize >= KVS_AAC_CPD_SIZE_BYTE && pCpd != NULL, STATUS_MKV_INVALID_AAC_CPD);

    // AAC cpd are encoded in the first 2 bytes of the cpd
    cpdContainer = GET_UNALIGNED_BIG_ENDIAN((PINT16) pCpd);

    /*
     * aac cpd structure
     * 5 bits (Audio Object Type) | 4 bits (frequency index) | 4 bits (channel configuration) | 3 bits (not used)
     */
    channelConfig = (UINT16)((cpdContainer >> 3) & 0x000f);
    CHK(channelConfig < MKV_AAC_CHANNEL_CONFIG_MAX, STATUS_MKV_INVALID_AAC_CPD_CHANNEL_CONFIG);
    *pChannelConfig = channelConfig;

    samplingRateIdx = (UINT16)((cpdContainer >> 7) & 0x000f);
    CHK(samplingRateIdx < MKV_AAC_SAMPLING_FREQUNECY_IDX_MAX, STATUS_MKV_INVALID_AAC_CPD_SAMPLING_FREQUENCY_INDEX);
    *pSamplingFrequency = gMkvAACSamplingFrequencies[samplingRateIdx];

CleanUp:
    return retStatus;
}

STATUS mkvgenGenerateAacCpd(KVS_MPEG4_AUDIO_OBJECT_TYPES objectType, UINT32 samplingFrequency, UINT16 channelConfig, PBYTE pBuffer, UINT32 bufferLen)
{
    STATUS retStatus = STATUS_SUCCESS;
    BOOL samplingFreqFound = FALSE;
    UINT16 samplingFreqIndex = 0, i, objectTypeInt16 = 0, cpdInt16 = 0;

    CHK(pBuffer != NULL, STATUS_NULL_ARG);
    CHK(channelConfig > 0 && channelConfig < MKV_AAC_CHANNEL_CONFIG_MAX && bufferLen >= KVS_AAC_CPD_SIZE_BYTE, STATUS_INVALID_ARG);

    for (i = 0; i < (UINT16) MKV_AAC_SAMPLING_FREQUNECY_IDX_MAX && !samplingFreqFound; ++i) {
        if (gMkvAACSamplingFrequencies[i] == samplingFrequency) {
            samplingFreqIndex = i;
            samplingFreqFound = TRUE;
        }
    }
    CHK_ERR(samplingFreqFound, STATUS_INVALID_ARG, "Invalid sampling frequency %u", samplingFrequency);

    MEMSET(pBuffer, 0x00, KVS_AAC_CPD_SIZE_BYTE);

    // just in case
    initializeEndianness();

    /*
     * aac cpd structure
     * 5 bits (Audio Object Type) | 4 bits (frequency index) | 4 bits (channel configuration) | 3 bits (not used)
     */
    objectTypeInt16 = (UINT16) objectType;
    cpdInt16 |= objectTypeInt16 << 11;
    cpdInt16 |= samplingFreqIndex << 7;
    cpdInt16 |= channelConfig << 3;

    PUT_UNALIGNED_BIG_ENDIAN((PINT16) pBuffer, cpdInt16);

CleanUp:

    CHK_LOG_ERR(retStatus);
    return retStatus;
}

/**
 * Parse A_MS/ACM Cpd
 */
STATUS getAudioConfigFromAmsAcmCpd(PBYTE pCpd, UINT32 cpdSize, PDOUBLE pSamplingFrequency, PUINT16 pChannelCount, PUINT16 pBitDepth)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT16 channelCount = 0, bitDepth = 0, formatCode = 0;
    UINT32 samplingFrequency = 0;
    volatile DOUBLE samplingFrequencyDouble = 0;

    CHK(pSamplingFrequency != NULL && pChannelCount != NULL && pBitDepth != NULL && pCpd != NULL, STATUS_NULL_ARG);
    CHK(cpdSize >= MIN_AMS_ACM_CPD_SIZE, STATUS_MKV_INVALID_AMS_ACM_CPD);

    /*
     * http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
     * cpd structure (little endian):
     * - 2 bytes format code (0x06 0x00 for pcm alaw)
     * - 2 bytes number of channels
     * - 4 bytes sampling rate
     * - 4 bytes average bytes per second
     * - 2 bytes block align
     * - 2 bytes bit depth
     * - 2 bytes extra data (usually 0)
     */
    formatCode = (UINT16)(*((PINT16) pCpd));
    channelCount = (UINT16) * ((PINT16)(pCpd + 2));
    samplingFrequency = (UINT32) * ((PINT32)(pCpd + 4));
    bitDepth = (UINT16) * ((PINT16)(pCpd + 14));

    // cpd is in little endian
    if (isBigEndian()) {
        formatCode = (UINT16) SWAP_INT16(formatCode);
        channelCount = (UINT16) SWAP_INT16(channelCount);
        samplingFrequency = (UINT32) SWAP_INT32(samplingFrequency);
        bitDepth = (UINT16) SWAP_INT16(bitDepth);
    }

    CHK(formatCode == (UINT16) MKV_WAV_FORMAT_ALAW || formatCode == (UINT16) MKV_WAV_FORMAT_MULAW, STATUS_MKV_INVALID_AMS_ACM_CPD);
    // Directly castting and assigning samplingFrequency to *pSamplingFrequency can cause bus error on raspberry pi.
    // Suspecting that it is compiler specific. Similar issue also happened in AckParser.c, processAckValue function,
    // FRAGMENT_ACK_KEY_NAME_FRAGMENT_TIMECODE case
    samplingFrequencyDouble = (DOUBLE) samplingFrequency;
    *pSamplingFrequency = samplingFrequencyDouble;
    *pChannelCount = channelCount;
    *pBitDepth = bitDepth;

CleanUp:
    return retStatus;
}

STATUS mkvgenGeneratePcmCpd(KVS_PCM_FORMAT_CODE format, UINT32 samplingRate, UINT16 channels, PBYTE buffer, UINT32 bufferLen)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT16 blockAlign = 0;
    UINT32 bitrate = 0;
    PBYTE pCurPtr = NULL;

    CHK(buffer != NULL, STATUS_NULL_ARG);
    CHK_ERR(bufferLen >= KVS_PCM_CPD_SIZE_BYTE, STATUS_INVALID_ARG, "Buffer is too small");
    CHK_ERR(format == KVS_PCM_FORMAT_CODE_ALAW || format == KVS_PCM_FORMAT_CODE_MULAW, STATUS_INVALID_ARG,
            "Invalid pcm format, should be alaw (0x%04x) or mulaw (0x%04x)", KVS_PCM_FORMAT_CODE_ALAW, KVS_PCM_FORMAT_CODE_MULAW);
    CHK_ERR(samplingRate <= MAX_PCM_SAMPLING_RATE && samplingRate >= MIN_PCM_SAMPLING_RATE, STATUS_INVALID_ARG, "Invalid sampling rate %u",
            samplingRate);
    CHK_ERR(channels == 2 || channels == 1, STATUS_INVALID_ARG, "Invalid channels count %u", channels);

    blockAlign = channels;
    bitrate = blockAlign * samplingRate / 8;

    // just in case
    initializeEndianness();

    MEMSET(buffer, 0x00, KVS_PCM_CPD_SIZE_BYTE);

    pCurPtr = buffer;
    putUnalignedInt16LittleEndian((PINT16) pCurPtr, (UINT16) format);
    pCurPtr += SIZEOF(UINT16);
    putUnalignedInt16LittleEndian((PINT16) pCurPtr, channels);
    pCurPtr += SIZEOF(UINT16);
    putUnalignedInt32LittleEndian((PINT32) pCurPtr, samplingRate);
    pCurPtr += SIZEOF(UINT32);
    putUnalignedInt32LittleEndian((PINT32) pCurPtr, bitrate);
    pCurPtr += SIZEOF(UINT32);
    putUnalignedInt16LittleEndian((PINT16) pCurPtr, blockAlign);
    // leave remaining bits as 0

CleanUp:

    CHK_LOG_ERR(retStatus);
    return retStatus;
}

/**
 * Adapts the CPD.
 *
 * NOTE: The input has been validated.
 */
STATUS mkvgenAdaptCodecPrivateData(PStreamMkvGenerator pMkvGenerator, MKV_TRACK_INFO_TYPE trackType, PCHAR codecId, UINT32 cpdSize, PBYTE cpd,
                                   PUINT32 pCpdSize, PBYTE* ppCpd, PTrackCustomData pData)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 adaptedCodecPrivateDataSize;
    PBYTE pCpd = NULL;

    // Initialize to existing size
    adaptedCodecPrivateDataSize = cpdSize;

    // Early return on empty cpd
    CHK(adaptedCodecPrivateDataSize != 0, retStatus);

    // Check if the CPD needs to be adapted
    if (trackType == MKV_TRACK_INFO_TYPE_VIDEO && pMkvGenerator->adaptCpdNals) {
        if ((pMkvGenerator->contentType & MKV_CONTENT_TYPE_H264) != MKV_CONTENT_TYPE_NONE) {
            CHK_STATUS(adaptH264CpdNalsFromAnnexBToAvcc(cpd, cpdSize, NULL, &adaptedCodecPrivateDataSize));
        } else if ((pMkvGenerator->contentType & MKV_CONTENT_TYPE_H265) != MKV_CONTENT_TYPE_NONE) {
            CHK_STATUS(adaptH265CpdNalsFromAnnexBToHvcc(cpd, cpdSize, NULL, &adaptedCodecPrivateDataSize));
        }
    }

    // Adapt/copy the CPD
    pCpd = (PBYTE) MEMALLOC(adaptedCodecPrivateDataSize);
    CHK(pCpd != NULL, STATUS_NOT_ENOUGH_MEMORY);

    if (trackType == MKV_TRACK_INFO_TYPE_VIDEO && pMkvGenerator->adaptCpdNals) {
        if ((pMkvGenerator->contentType & MKV_CONTENT_TYPE_H264) != MKV_CONTENT_TYPE_NONE) {
            CHK_STATUS(adaptH264CpdNalsFromAnnexBToAvcc(cpd, cpdSize, pCpd, &adaptedCodecPrivateDataSize));
        } else if ((pMkvGenerator->contentType & MKV_CONTENT_TYPE_H265) != MKV_CONTENT_TYPE_NONE) {
            CHK_STATUS(adaptH265CpdNalsFromAnnexBToHvcc(cpd, cpdSize, pCpd, &adaptedCodecPrivateDataSize));
        }
    } else {
        MEMCPY(pCpd, cpd, adaptedCodecPrivateDataSize);
    }

    // Check whether we need to generate a video config element for
    // H264, H265 or M-JJPG content type if the CPD is present
    switch (trackType) {
        case MKV_TRACK_INFO_TYPE_UNKOWN:
            break;

        case MKV_TRACK_INFO_TYPE_VIDEO:
            // Check and process the H264 then H265 and later M-JPG content type
            if ((pMkvGenerator->contentType & MKV_CONTENT_TYPE_H264) != MKV_CONTENT_TYPE_NONE) {
                // Important: The assumption here is that if this is not in AvCC format and it's in Annex-B or RAW then
                // the first NALu is the SPS. We will not skip over possible SEI or AUD NALus
                retStatus = getVideoWidthAndHeightFromH264Sps(pCpd, adaptedCodecPrivateDataSize, &pData->trackVideoConfig.videoWidth,
                                                              &pData->trackVideoConfig.videoHeight);

            } else if ((pMkvGenerator->contentType & MKV_CONTENT_TYPE_H265) != MKV_CONTENT_TYPE_NONE) {
                // Important: The assumption here is that if this is not in HvCC format and it's in Annex-B or RAW then
                // the first NALu is the SPS. We will not skip over possible SEI or AUD NALus
                retStatus = getVideoWidthAndHeightFromH265Sps(pCpd, adaptedCodecPrivateDataSize, &pData->trackVideoConfig.videoWidth,
                                                              &pData->trackVideoConfig.videoHeight);

            } else if (((pMkvGenerator->contentType & MKV_CONTENT_TYPE_X_MKV_VIDEO) != MKV_CONTENT_TYPE_NONE) &&
                       (0 == STRCMP(codecId, MKV_FOURCC_CODEC_ID))) {
                // For M-JPG we have content type as video/x-matroska and the codec
                // type set as V_MS/VFW/FOURCC
                retStatus = getVideoWidthAndHeightFromBih(pCpd, adaptedCodecPrivateDataSize, &pData->trackVideoConfig.videoWidth,
                                                          &pData->trackVideoConfig.videoHeight);
            }

            if (STATUS_FAILED(retStatus)) {
                // This might not be yet fatal so warn and reset the status
                DLOGW("Failed extracting video configuration from SPS with %08x.", retStatus);
                MEMSET(&pData->trackVideoConfig, 0x00, SIZEOF(TrackVideoConfig));
                retStatus = STATUS_SUCCESS;
            }

            break;

        case MKV_TRACK_INFO_TYPE_AUDIO:
            // zero out the fields
            MEMSET(&pData->trackAudioConfig, 0x00, SIZEOF(TrackCustomData));

            if ((pMkvGenerator->contentType & MKV_CONTENT_TYPE_AAC) != MKV_CONTENT_TYPE_NONE) {
                retStatus = getSamplingFreqAndChannelFromAacCpd(pCpd, adaptedCodecPrivateDataSize, &pData->trackAudioConfig.samplingFrequency,
                                                                &pData->trackAudioConfig.channelConfig);
            } else if ((pMkvGenerator->contentType & (MKV_CONTENT_TYPE_ALAW | MKV_CONTENT_TYPE_MULAW)) != MKV_CONTENT_TYPE_NONE) {
                retStatus = getAudioConfigFromAmsAcmCpd(pCpd, adaptedCodecPrivateDataSize, &pData->trackAudioConfig.samplingFrequency,
                                                        &pData->trackAudioConfig.channelConfig, &pData->trackAudioConfig.bitDepth);
            }

            if (STATUS_FAILED(retStatus)) {
                // This might not be yet fatal so warn and reset the status
                DLOGW("Failed extracting audio configuration from codec private data with %08x.", retStatus);
                MEMSET(&pData->trackAudioConfig, 0x00, SIZEOF(TrackAudioConfig));
                retStatus = STATUS_SUCCESS;
            }

            break;
    }

CleanUp:

    *ppCpd = pCpd;
    *pCpdSize = adaptedCodecPrivateDataSize;

    LEAVES();
    return retStatus;
}

STATUS mkvgenGetTrackInfo(PTrackInfo pTrackInfos, UINT32 trackInfoCount, UINT64 trackId, PTrackInfo* ppTrackInfo, PUINT32 pIndex)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PTrackInfo pTrackInfo = NULL;
    UINT32 i = 0;

    CHK(pTrackInfos != NULL || trackInfoCount == 0, STATUS_INVALID_ARG);

    for (i = 0; i < trackInfoCount; i++) {
        if (pTrackInfos[i].trackId == trackId) {
            pTrackInfo = &pTrackInfos[i];
            break;
        }
    }

    CHK(pTrackInfo != NULL, STATUS_MKV_TRACK_INFO_NOT_FOUND);

CleanUp:

    if (ppTrackInfo != NULL) {
        *ppTrackInfo = pTrackInfo;
    }

    if (pIndex != NULL) {
        *pIndex = i;
    }

    LEAVES();
    return retStatus;
}

STATUS mkvgenExtractCpdFromAnnexBFrame(PStreamMkvGenerator pStreamMkvGenerator, PFrame pFrame, PTrackInfo pTrackInfo)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 cpdSize, runLen, parsedSize = 0;
    PBYTE pCpd = NULL;
    PUINT32 pCurPtr;
    BOOL iterate = TRUE;
    BYTE naluHeader;

    CHK(pFrame != NULL && pTrackInfo != NULL, STATUS_NULL_ARG);

    // Convert to AvCC format
    CHK_STATUS(adaptFrameNalsFromAnnexBToAvcc(pFrame->frameData, pFrame->size, FALSE, NULL, &cpdSize));
    CHK(cpdSize > MIN_H264_H265_CPD_SIZE, STATUS_INVALID_ARG);
    pCpd = (PBYTE) MEMALLOC(cpdSize);
    CHK(pCpd != NULL, STATUS_NOT_ENOUGH_MEMORY);
    CHK_STATUS(adaptFrameNalsFromAnnexBToAvcc(pFrame->frameData, pFrame->size, FALSE, pCpd, &cpdSize));

    // Here we will assume that all of the runs prior the I-frame are part of the CPD and will use in it's entirety
    // The CPD adapter and/or height/width extracting will need to handle the rest
    pCurPtr = (PUINT32) pCpd;

    while (iterate && (PBYTE)(pCurPtr + 1) < pCpd + cpdSize) {
        runLen = GET_UNALIGNED_BIG_ENDIAN(pCurPtr);
        naluHeader = *(PBYTE)(pCurPtr + 1);

        // Check for the H264 format IDR slice NAL type if h264 or IDR_W_RADL_NALU_TYPE if h265
        if (((pStreamMkvGenerator->contentType & MKV_CONTENT_TYPE_H264) != MKV_CONTENT_TYPE_NONE && (naluHeader & 0x80) == 0 &&
             (naluHeader & 0x60) != 0 && (naluHeader & 0x1f) == IDR_NALU_TYPE) ||
            ((pStreamMkvGenerator->contentType & MKV_CONTENT_TYPE_H265) != MKV_CONTENT_TYPE_NONE &&
             ((naluHeader >> 1) == IDR_W_RADL_NALU_TYPE || (naluHeader >> 1) == IDR_N_LP_NALU_TYPE))) {
            parsedSize = (UINT32)((PBYTE) pCurPtr - pCpd);
            iterate = FALSE;
        }

        // Advance the current ptr taking into account the 4 byte length and the size of the run
        pCurPtr = (PUINT32)((PBYTE)(pCurPtr + 1) + runLen);
    }

    // Ensure that we extracted CPD
    CHK(parsedSize != 0, STATUS_MKV_INVALID_ANNEXB_CPD_NALUS);

    // Assume it's all OK - convert back to AnnexB
    CHK_STATUS(adaptFrameNalsFromAvccToAnnexB(pCpd, parsedSize));

    // Set the CPD for the track
    CHK_STATUS(mkvgenSetCodecPrivateData((PMkvGenerator) pStreamMkvGenerator, pFrame->trackId, parsedSize, pCpd));

CleanUp:

    if (pCpd != NULL) {
        MEMFREE(pCpd);
    }

    LEAVES();
    return retStatus;
}
