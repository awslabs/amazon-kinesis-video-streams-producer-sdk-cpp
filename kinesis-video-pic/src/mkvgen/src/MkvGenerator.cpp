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
                          PBYTE segmentUuid, PTrackInfo trackInfoList, UINT32 trackInfoCount,
                          GetCurrentTimeFunc getTimeFn, UINT64 customData, PMkvGenerator* ppMkvGenerator)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStreamMkvGenerator pMkvGenerator = NULL;
    UINT32 allocationSize, adaptedCodecPrivateDataSize;
    BOOL adaptAnnexB = FALSE, adaptAvcc = FALSE, adaptCpdAnnexB = FALSE;
    UINT32 i;
    const UINT32 videoTrackIndex = 0;

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

    for (i = 0; i < trackInfoCount; ++i) {
        PTrackInfo pTrackInfo = &trackInfoList[i];
        CHK(STRNLEN(pTrackInfo->codecId, MKV_MAX_CODEC_ID_LEN) < MKV_MAX_CODEC_ID_LEN, STATUS_MKV_INVALID_CODEC_ID_LENGTH);
        CHK(STRNLEN(pTrackInfo->trackName, MKV_MAX_TRACK_NAME_LEN) < MKV_MAX_TRACK_NAME_LEN, STATUS_MKV_INVALID_TRACK_NAME_LENGTH);
        CHK(pTrackInfo->codecPrivateDataSize <= MKV_MAX_CODEC_PRIVATE_LEN, STATUS_MKV_INVALID_CODEC_PRIVATE_LENGTH);
        CHK(pTrackInfo->codecPrivateDataSize == 0 || pTrackInfo->codecPrivateData != NULL, STATUS_MKV_CODEC_PRIVATE_NULL);
    }

    // Initialize the endianness for the library
    initializeEndianness();

    for (i = 0; i < trackInfoCount; ++i) {
        if (trackInfoList[i].codecPrivateDataSize != 0) {
            DLOGS("TrackName: %s, CodecId: %s", trackInfoList[i].trackName, trackInfoList[i].codecId);
            dumpMemoryHex(trackInfoList[i].codecPrivateData, trackInfoList[i].codecPrivateDataSize);
        }
    }

    // Calculate the adapted CPD size
    adaptedCodecPrivateDataSize = 0;
    for (i = 0; i < trackInfoCount; ++i) {
        if (trackInfoList[i].trackType == MKV_TRACK_INFO_TYPE_VIDEO && trackInfoList[i].codecPrivateDataSize != 0 && adaptCpdAnnexB) {
            if (0 == STRCMP(contentType, MKV_H264_CONTENT_TYPE)) {
                CHK_STATUS(adaptH264CpdNalsFromAnnexBToAvcc(trackInfoList[i].codecPrivateData,
                                                            trackInfoList[i].codecPrivateDataSize,
                                                            NULL,
                                                            &adaptedCodecPrivateDataSize));
            } else if (0 == STRCMP(contentType, MKV_H265_CONTENT_TYPE)) {
                CHK_STATUS(adaptH265CpdNalsFromAnnexBToHvcc(trackInfoList[i].codecPrivateData,
                                                            trackInfoList[i].codecPrivateDataSize,
                                                            NULL,
                                                            &adaptedCodecPrivateDataSize));
            }
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
    pMkvGenerator->clusterDuration = clusterDuration * DEFAULT_TIME_UNIT_IN_NANOS / pMkvGenerator->timecodeScale; // No chance of an overflow as we check against max earlier
    pMkvGenerator->trackType = mkvgenGetTrackTypeFromContentType(contentType);
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
    // Copy TrackInfoList to the end of MkvGenerator struct
    pMkvGenerator->trackInfoList = (PTrackInfo) (pMkvGenerator + 1);
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
        MEMCPY(pMkvGenerator->segmentUuid, segmentUuid, MKV_SEGMENT_UUID_LEN);
    }

    // Store the custom data as well
    pMkvGenerator->customData = customData;

    // Set the codec private data
    for (i = 0; i < trackInfoCount; ++i) {
        PTrackInfo pTrackInfo = &pMkvGenerator->trackInfoList[i];
        if (i == videoTrackIndex && adaptedCodecPrivateDataSize != 0) {
            pTrackInfo->codecPrivateData = (PBYTE) MEMALLOC(adaptedCodecPrivateDataSize);
            if (pMkvGenerator->adaptCpdNals) {
                if (0 == STRCMP(contentType, MKV_H264_CONTENT_TYPE)) {
                    CHK_STATUS(adaptH264CpdNalsFromAnnexBToAvcc(trackInfoList[i].codecPrivateData,
                                                                trackInfoList[i].codecPrivateDataSize,
                                                                pTrackInfo->codecPrivateData,
                                                                &pTrackInfo->codecPrivateDataSize));
                } else if (0 == STRCMP(contentType, MKV_H265_CONTENT_TYPE)) {
                    CHK_STATUS(adaptH265CpdNalsFromAnnexBToHvcc(trackInfoList[i].codecPrivateData,
                                                                trackInfoList[i].codecPrivateDataSize,
                                                                pTrackInfo->codecPrivateData,
                                                                &pTrackInfo->codecPrivateDataSize));
                }
            } else {
                MEMCPY(pTrackInfo->codecPrivateData, trackInfoList[i].codecPrivateData, trackInfoList[i].codecPrivateDataSize);
            }
        } else {
            pTrackInfo->codecPrivateData = (PBYTE) MEMALLOC(pTrackInfo->codecPrivateDataSize);
            MEMCPY(pTrackInfo->codecPrivateData, trackInfoList[i].codecPrivateData, trackInfoList[i].codecPrivateDataSize);
        }
    }

    // Check whether we need to generate a video config element for
    // H264, H265 or M-JJPG content type if the CPD is present
    for (i = 0; i < trackInfoCount; ++i) {
        if (pMkvGenerator->trackInfoList[i].codecPrivateData != NULL && pMkvGenerator->trackInfoList[i].codecPrivateDataSize != 0) {
            PTrackInfo pTrackInfo = &pMkvGenerator->trackInfoList[i];
            switch (pTrackInfo->trackType) {
                case MKV_TRACK_INFO_TYPE_VIDEO :
                    // Check and process the H264 then H265 and later M-JPG content type
                    if (0 == STRCMP(contentType, MKV_H264_CONTENT_TYPE)) {
                        retStatus = getVideoWidthAndHeightFromH264Sps(pTrackInfo->codecPrivateData,
                                                                      pTrackInfo->codecPrivateDataSize,
                                                                      &pTrackInfo->trackCustomData.trackVideoConfig.videoWidth,
                                                                      &pTrackInfo->trackCustomData.trackVideoConfig.videoHeight);

                    } else if (0 == STRCMP(contentType, MKV_H265_CONTENT_TYPE)) {
                        retStatus = getVideoWidthAndHeightFromH265Sps(pTrackInfo->codecPrivateData,
                                                                      pTrackInfo->codecPrivateDataSize,
                                                                      &pTrackInfo->trackCustomData.trackVideoConfig.videoWidth,
                                                                      &pTrackInfo->trackCustomData.trackVideoConfig.videoHeight);

                    } else if ((0 == STRCMP(contentType, MKV_X_MKV_CONTENT_TYPE)) &&
                               (0 == STRCMP(pTrackInfo->codecId, MKV_FOURCC_CODEC_ID))) {
                        // For M-JPG we have content type as video/x-matroska and the codec
                        // type set as V_MS/VFW/FOURCC
                        retStatus = getVideoWidthAndHeightFromBih(pTrackInfo->codecPrivateData,
                                                                  pTrackInfo->codecPrivateDataSize,
                                                                  &pTrackInfo->trackCustomData.trackVideoConfig.videoWidth,
                                                                  &pTrackInfo->trackCustomData.trackVideoConfig.videoHeight);
                    }

                    if (STATUS_FAILED(retStatus)) {
                        // This might not be yet fatal so warn and reset the status
                        DLOGW("Failed extracting video configuration from SPS with %08x.", retStatus);

                        retStatus = STATUS_SUCCESS;
                    }
                    break;
                case MKV_TRACK_INFO_TYPE_AUDIO :
                    getSamplingFreqAndChannelFromAacCpd(pTrackInfo->codecPrivateData,
                                                        pTrackInfo->codecPrivateDataSize,
                                                        &pTrackInfo->trackCustomData.trackAudioConfig.samplingFrequency,
                                                        &pTrackInfo->trackCustomData.trackAudioConfig.channelConfig);

                    if (STATUS_FAILED(retStatus)) {
                        // This might not be yet fatal so warn and reset the status
                        DLOGW("Failed extracting audio configuration from codec private data with %08x.", retStatus);

                        retStatus = STATUS_SUCCESS;
                    }
                    break;
            }
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
    PStreamMkvGenerator pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;
    UINT32 i;

    // Call is idempotent
    CHK(pMkvGenerator != NULL, retStatus);
    for(i = 0; i < pStreamMkvGenerator->trackInfoCount; ++i) {
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
STATUS mkvgenPackageFrame(PMkvGenerator pMkvGenerator, PFrame pFrame, PBYTE pBuffer, PUINT32 pSize, PEncodedFrameInfo pEncodedFrameInfo)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStreamMkvGenerator pStreamMkvGenerator = NULL;
    MKV_STREAM_STATE streamState = MKV_STATE_START_STREAM;
    UINT32 bufferSize = 0, encodedLen = 0, packagedSize = 0, adaptedFrameSize = 0, overheadSize = 0;
    // Evaluated presentation and decode timestamps
    UINT64 pts = 0, dts = 0, duration = 0;
    PBYTE pCurrentPnt = pBuffer;

    // Check the input params
    CHK(pSize != NULL && pMkvGenerator != NULL, STATUS_NULL_ARG);

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;

    // Validate and extract the timestamp
    CHK_STATUS(mkvgenValidateFrame(pStreamMkvGenerator, pFrame, &pts, &dts, &duration, &streamState));

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

                CHK_STATUS(mkvgenEbmlEncodeTrackInfo(pCurrentPnt,
                                                     bufferSize,
                                                     pStreamMkvGenerator,
                                                     &encodedLen));
                bufferSize -= encodedLen;
                pCurrentPnt += encodedLen;

                pStreamMkvGenerator->generatorState = MKV_GENERATOR_STATE_CLUSTER_INFO;
            }

            // Fall-through
        case MKV_STATE_START_CLUSTER:
            // If we just added tags then we need to add the segment and track info
            if (pStreamMkvGenerator->generatorState == MKV_GENERATOR_STATE_SEGMENT_HEADER) {
                CHK_STATUS(mkvgenEbmlEncodeSegmentInfo(pStreamMkvGenerator, pCurrentPnt, bufferSize, &encodedLen));
                bufferSize -= encodedLen;
                pCurrentPnt += encodedLen;

                CHK_STATUS(mkvgenEbmlEncodeTrackInfo(pCurrentPnt,
                                                     bufferSize,
                                                     pStreamMkvGenerator,
                                                     &encodedLen));
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
            CHK_STATUS(mkvgenEbmlEncodeClusterInfo(pCurrentPnt,
                                                   bufferSize,
                                                   pStreamMkvGenerator->absoluteTimeClusters ?
                                                       pts : pts - pStreamMkvGenerator->streamStartTimestamp,
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
            CHK_STATUS(mkvgenEbmlEncodeSimpleBlock(pCurrentPnt,
                                                   bufferSize,
                                                   (INT16) pts,
                                                   pFrame,
                                                   adaptedFrameSize,
                                                   pStreamMkvGenerator,
                                                   &encodedLen));
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
            pEncodedFrameInfo->streamStartTs = MKV_TIMECODE_TO_TIMESTAMP(pStreamMkvGenerator->streamStartTimestamp, pStreamMkvGenerator->timecodeScale);
            pEncodedFrameInfo->clusterPts = MKV_TIMECODE_TO_TIMESTAMP(pStreamMkvGenerator->lastClusterPts, pStreamMkvGenerator->timecodeScale);
            pEncodedFrameInfo->clusterDts = MKV_TIMECODE_TO_TIMESTAMP(pStreamMkvGenerator->lastClusterDts, pStreamMkvGenerator->timecodeScale);
            pEncodedFrameInfo->framePts = MKV_TIMECODE_TO_TIMESTAMP(pts, pStreamMkvGenerator->timecodeScale);
            pEncodedFrameInfo->frameDts = MKV_TIMECODE_TO_TIMESTAMP(dts, pStreamMkvGenerator->timecodeScale);
            pEncodedFrameInfo->duration = MKV_TIMECODE_TO_TIMESTAMP(duration, pStreamMkvGenerator->timecodeScale);
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
    PStreamMkvGenerator pStreamMkvGenerator = NULL;
    UINT32 bufferSize, encodedLen, packagedSize = 0;
    PBYTE pCurrentPnt = pBuffer;

    // Check the input params
    CHK(pSize != NULL && pMkvGenerator != NULL, STATUS_NULL_ARG);

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;

    // Calculate the necessary size

    // Get the overhead when packaging MKV
    packagedSize = mkvgenGetMkvHeaderSize(pStreamMkvGenerator->trackInfoList, pStreamMkvGenerator->trackInfoCount);

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
    UINT32 bufferSize, encodedLen, packagedSize = 0, tagNameLen, tagValueLen, overheadSize = 0;
    UINT64 encodedElementLength;
    PBYTE pCurrentPnt = pBuffer;
    PBYTE pStartPnt = pBuffer;

    // Check the input params
    CHK(pSize != NULL && pMkvGenerator != NULL && tagName != NULL && tagValue != NULL, STATUS_NULL_ARG);
    CHK((tagNameLen = (UINT32) STRNLEN(tagName, MKV_MAX_TAG_NAME_LEN + 1)) <= MKV_MAX_TAG_NAME_LEN &&
        tagNameLen > 0, STATUS_MKV_INVALID_TAG_NAME_LENGTH);
    CHK((tagValueLen = (UINT32) STRNLEN(tagValue, MKV_MAX_TAG_VALUE_LEN + 1)) <= MKV_MAX_TAG_VALUE_LEN,
        STATUS_MKV_INVALID_TAG_VALUE_LENGTH);

    pStreamMkvGenerator = (PStreamMkvGenerator) pMkvGenerator;

    // Calculate the necessary size

    // Get the overhead size. If we are just starting then we need to generate the MKV header
    if (pStreamMkvGenerator->generatorState == MKV_GENERATOR_STATE_START) {
        overheadSize = MKV_EBML_SEGMENT_SIZE;
    }

    // Get the overhead when packaging MKV
    packagedSize = overheadSize +
                   MKV_TAGS_BITS_SIZE +
                   MKV_TAG_NAME_BITS_SIZE +
                   MKV_TAG_STRING_BITS_SIZE +
                   tagNameLen +
                   tagValueLen;

    // Check if we are asked for size only and early return if so
    CHK(pBuffer != NULL, STATUS_SUCCESS);

    // Preliminary check for the buffer size
    CHK(*pSize >= packagedSize, STATUS_NOT_ENOUGH_MEMORY);

    // Start with the full buffer
    bufferSize = *pSize;

    if (pStreamMkvGenerator->generatorState == MKV_GENERATOR_STATE_START) {
        // Encode in sequence and subtract the size
        CHK_STATUS(mkvgenEbmlEncodeHeader(pCurrentPnt, bufferSize, &encodedLen));
        bufferSize -= encodedLen;
        pCurrentPnt += encodedLen;

        CHK_STATUS(mkvgenEbmlEncodeSegmentHeader(pCurrentPnt, bufferSize, &encodedLen));
        bufferSize -= encodedLen;
        pCurrentPnt += encodedLen;

        pStartPnt = pCurrentPnt;
    }

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
    encodedElementLength = 0x100000000000000ULL | (UINT64) (packagedSize - overheadSize - MKV_TAG_ELEMENT_OFFSET);
    putInt64((PINT64)(pStartPnt + MKV_TAGS_ELEMENT_SIZE_OFFSET), encodedElementLength);

    // Fix-up the tag element size
    encodedElementLength = 0x100000000000000ULL | (UINT64) (packagedSize - overheadSize - MKV_SIMPLE_TAG_ELEMENT_OFFSET);
    putInt64((PINT64)(pStartPnt + MKV_TAG_ELEMENT_SIZE_OFFSET), encodedElementLength);

    // Fix-up the simple tag element size
    encodedElementLength = 0x100000000000000ULL | (UINT64) (packagedSize - overheadSize - MKV_TAGS_BITS_SIZE);
    putInt64((PINT64)(pStartPnt + MKV_SIMPLE_TAG_ELEMENT_SIZE_OFFSET), encodedElementLength);

    // Fix-up the tag name element size
    encodedElementLength = 0x100000000000000ULL | (UINT64) (tagNameLen);
    putInt64((PINT64)(pStartPnt + MKV_TAGS_BITS_SIZE + MKV_TAG_NAME_ELEMENT_SIZE_OFFSET), encodedElementLength);

    // Fix-up the tag string element size
    encodedElementLength = 0x100000000000000ULL | (UINT64) (tagValueLen);
    putInt64((PINT64)(pStartPnt +
                      MKV_TAGS_BITS_SIZE +
                      MKV_TAG_NAME_BITS_SIZE +
                      tagNameLen +
                      MKV_TAG_STRING_ELEMENT_SIZE_OFFSET), encodedElementLength);

    // Validate the size
    CHK(packagedSize == (UINT32)(pCurrentPnt - pBuffer), STATUS_INTERNAL_ERROR);

    // Move the generator to the started state if all successful and we are actually
    // generating the data and not only returning the generated size.
    // This will also indicate that the header has already been generated.
    switch (pStreamMkvGenerator->generatorState) {
        case MKV_GENERATOR_STATE_START:
            pStreamMkvGenerator->generatorState = MKV_GENERATOR_STATE_SEGMENT_HEADER;
            break;

        case MKV_GENERATOR_STATE_SEGMENT_HEADER:
            // Leave as is
            break;

        default:
            // Set to tags state
            pStreamMkvGenerator->generatorState = MKV_GENERATOR_STATE_TAGS;
    }

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
STATUS mkvgenValidateFrame(PStreamMkvGenerator pStreamMkvGenerator, PFrame pFrame, PUINT64 pPts, PUINT64 pDts, PUINT64 pDuration, PMKV_STREAM_STATE pStreamState)
{
    UINT64 dts = 0, pts = 0, duration = 0;
    MKV_STREAM_STATE streamState;
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pStreamMkvGenerator != NULL
        && pFrame != NULL
        && pPts != NULL
        && pDts != NULL
        && pDuration != NULL
        && pStreamState != NULL, STATUS_NULL_ARG);
    CHK(pFrame->size > 0 && pFrame->frameData != NULL, STATUS_MKV_INVALID_FRAME_DATA);

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

    CHK(pts >= pStreamMkvGenerator->lastClusterPts && dts >= pStreamMkvGenerator->lastClusterDts,
        STATUS_MKV_INVALID_FRAME_TIMESTAMP);

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
        if (pStreamMkvGenerator->keyFrameClustering ||
                timeCode - pStreamMkvGenerator->lastClusterPts >= pStreamMkvGenerator->clusterDuration) {
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
            if (pStreamMkvGenerator->generatorState == MKV_GENERATOR_STATE_SEGMENT_HEADER) {
                overhead = mkvgenGetMkvSegmentTrackInfoOverhead(pStreamMkvGenerator->trackInfoList, pStreamMkvGenerator->trackInfoCount);
            } else {
                overhead = mkvgenGetMkvHeaderOverhead(pStreamMkvGenerator->trackInfoList, pStreamMkvGenerator->trackInfoCount);
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
BYTE mkvgenGetTrackTypeFromContentType(PCHAR contentType)
{
    if (contentType != NULL && contentType[0] != '\0') {
        // Check if it starts with audio or video prefix (not counting the null terminator)
        if (0 == STRNCMP(contentType, MKV_CONTENT_TYPE_PREFIX_AUDIO, STRLEN(MKV_CONTENT_TYPE_PREFIX_AUDIO))) {
            return MKV_TRACK_TYPE_AUDIO;
        }

        if (0 == STRNCMP(contentType, MKV_CONTENT_TYPE_PREFIX_VIDEO, STRLEN(MKV_CONTENT_TYPE_PREFIX_VIDEO))) {
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
STATUS mkvgenEbmlEncodeSegmentInfo(PStreamMkvGenerator pStreamMkvGenerator, PBYTE pBuffer, UINT32 bufferSize, PUINT32 pEncodedLen)
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
    MEMCPY(pBuffer + MKV_SEGMENT_UID_OFFSET, pStreamMkvGenerator->segmentUuid, MKV_SEGMENT_UUID_LEN);

    // Fix up the default timecode scale
    putInt64((PINT64)(pBuffer + MKV_SEGMENT_TIMECODE_SCALE_OFFSET), pStreamMkvGenerator->timecodeScale);

CleanUp:

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
    UINT32 trackSpecificDataOffset = 0;
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
        *(pTrackStart + MKV_TRACK_TYPE_OFFSET) = pMkvGenerator->trackType;

        // Package track name
        trackNameDataSize = STRNLEN(pTrackInfo->trackName, MKV_MAX_TRACK_NAME_LEN);
        if (trackNameDataSize > 0) {
            MEMCPY(pTrackStart + trackSpecificDataOffset, MKV_TRACK_NAME_BITS, MKV_TRACK_NAME_BITS_SIZE);
            trackSpecificDataOffset += MKV_TRACK_NAME_BITS_SIZE;

            // Encode the track name
            CHK_STATUS(mkvgenEbmlEncodeNumber(trackNameDataSize,
                                              pTrackStart + trackSpecificDataOffset,
                                              bufferSize - trackSpecificDataOffset,
                                              &encodedLen));
            trackSpecificDataOffset += encodedLen;

            MEMCPY(pTrackStart + trackSpecificDataOffset, pTrackInfo->trackName, trackNameDataSize);
            trackSpecificDataOffset += trackNameDataSize;
            mkvTrackDataSize += MKV_TRACK_NAME_BITS_SIZE + encodedLen + trackNameDataSize;
        }
        // done packaging track name

        // Package codec id
        codecIdDataSize = STRNLEN(pTrackInfo->codecId, MKV_MAX_CODEC_ID_LEN);
        if (codecIdDataSize > 0) {
            MEMCPY(pTrackStart + trackSpecificDataOffset, MKV_CODEC_ID_BITS, MKV_CODEC_ID_BITS_SIZE);
            trackSpecificDataOffset += MKV_CODEC_ID_BITS_SIZE;

            // Encode the codec id size
            CHK_STATUS(mkvgenEbmlEncodeNumber(codecIdDataSize,
                                              pTrackStart + trackSpecificDataOffset,
                                              bufferSize - trackSpecificDataOffset,
                                              &encodedLen));
            trackSpecificDataOffset += encodedLen;
            mkvTrackDataSize += MKV_CODEC_ID_BITS_SIZE + encodedLen + codecIdDataSize;
            MEMCPY(pTrackStart + trackSpecificDataOffset, pTrackInfo->codecId, codecIdDataSize);
            trackSpecificDataOffset += codecIdDataSize;
        }
        // done packaging codec id

        // Fix up track number. Use trackInfo's index as mkv track number. Mkv track number starts from 1
        *(pTrackStart + MKV_TRACK_NUMBER_OFFSET) = (UINT8) (j + 1);
        switch (pTrackInfo->trackType) {
            case MKV_TRACK_INFO_TYPE_VIDEO:
                // Default template use video type
                break;
            case MKV_TRACK_INFO_TYPE_AUDIO:
                // Fix up track type
                *(pTrackStart + MKV_TRACK_TYPE_OFFSET) = (UINT8) (MKV_TRACK_TYPE_AUDIO);
                break;
            default:
                break;
        }

        // Fix up the track UID
        putInt64((PINT64)(pTrackStart + MKV_TRACK_ID_OFFSET), pTrackInfo->trackId);

        // Append the video config if any
        if (GENERATE_VIDEO_CONFIG(&pMkvGenerator->trackInfoList[j])) {
            // Record the amount of storage used in order to skip to next track later
            mkvTrackDataSize += MKV_TRACK_VIDEO_BITS_SIZE;

            // Copy the element first
            MEMCPY(pTrackStart + trackSpecificDataOffset, MKV_TRACK_VIDEO_BITS, MKV_TRACK_VIDEO_BITS_SIZE);

            // Fix-up the width and height
            putInt16((PINT16)(pTrackStart + trackSpecificDataOffset + MKV_TRACK_VIDEO_WIDTH_OFFSET), pTrackInfo->trackCustomData.trackVideoConfig.videoWidth);
            putInt16((PINT16)(pTrackStart + trackSpecificDataOffset + MKV_TRACK_VIDEO_HEIGHT_OFFSET), pTrackInfo->trackCustomData.trackVideoConfig.videoHeight);

            trackSpecificDataOffset += MKV_TRACK_VIDEO_BITS_SIZE;

        } else if (GENERATE_AUDIO_CONFIG(&pMkvGenerator->trackInfoList[j])) {
            // Record the amount of storage used in order to skip to next track later
            mkvTrackDataSize += MKV_TRACK_AUDIO_BITS_SIZE;

            // Copy the element first
            MEMCPY(pTrackStart + trackSpecificDataOffset, MKV_TRACK_AUDIO_BITS, MKV_TRACK_AUDIO_BITS_SIZE);

            putInt64((PINT64) (pTrackStart + trackSpecificDataOffset + MKV_TRACK_AUDIO_SAMPLING_RATE_OFFSET), *((PINT64) (&pTrackInfo->trackCustomData.trackAudioConfig.samplingFrequency)));
            *(pTrackStart + trackSpecificDataOffset + MKV_TRACK_AUDIO_CHANNELS_OFFSET) = (UINT8) pTrackInfo->trackCustomData.trackAudioConfig.channelConfig;

            trackSpecificDataOffset += MKV_TRACK_AUDIO_BITS_SIZE;
        }

        // Append the codec private data if any
        if (pTrackInfo->codecPrivateDataSize != 0 && pTrackInfo->codecPrivateData != NULL) {
            // Offset of the CPD right after the main structure and the video config if any
            cpdOffset = trackSpecificDataOffset;

            // Copy the element first
            MEMCPY(pTrackStart + cpdOffset, MKV_CODEC_PRIVATE_DATA_ELEM, MKV_CODEC_PRIVATE_DATA_ELEM_SIZE);
            cpdSize = MKV_CODEC_PRIVATE_DATA_ELEM_SIZE;

            // Encode the CPD size
            CHK_STATUS(mkvgenEbmlEncodeNumber(pTrackInfo->codecPrivateDataSize,
                                              pTrackStart + cpdOffset + cpdSize,
                                              bufferSize - cpdOffset - cpdSize,
                                              &encodedCpdLen));
            cpdSize += encodedCpdLen;

            CHK(cpdOffset + cpdSize + pTrackInfo->codecPrivateDataSize <= bufferSize, STATUS_NOT_ENOUGH_MEMORY);

            // Copy the actual CPD bits
            MEMCPY(pTrackStart + cpdOffset + cpdSize, pTrackInfo->codecPrivateData, pTrackInfo->codecPrivateDataSize);
            cpdSize += pTrackInfo->codecPrivateDataSize;;
            mkvTrackDataSize += cpdSize;
        }


        // Important! Need to fix-up the overall track header element size and the track entry element size
        // Encode and fix-up the size - encode 4 bytes
        encodedLen = 0x10000000 | (UINT32) (mkvTrackDataSize);
        putInt32((PINT32)(pTrackStart + MKV_TRACK_ENTRY_SIZE_OFFSET), encodedLen);

        // Need to add back TrackEntry header bits to skip to the end of current TrackEntry
        pTrackStart += mkvTrackDataSize + MKV_TRACK_ENTRY_HEADER_BITS_SIZE;
        // TrackEntry header bits size also need to be accounted when calculating overall Tracks size
        mkvTracksDataSize += mkvTrackDataSize + MKV_TRACK_ENTRY_HEADER_BITS_SIZE;
        // Reset mkvTrackDataSize as we are starting for a new track
        mkvTrackDataSize = 0;

    }
    // Important! Need to fix-up the overall track header element size and the track entry element size
    // Encode and fix-up the size - encode 4 bytes
    encodedLen = 0x10000000 | (UINT32) (mkvTracksDataSize);
    putInt32((PINT32)(pBuffer + MKV_TRACK_HEADER_SIZE_OFFSET), encodedLen);

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
STATUS mkvgenEbmlEncodeSimpleBlock(PBYTE pBuffer, UINT32 bufferSize, INT16 timestamp, PFrame pFrame, UINT32 adaptedFrameSize, PStreamMkvGenerator pStreamMkvGenerator, PUINT32 pEncodedLen)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 encodedLength;
    BYTE flags;
    UINT32 size;
    MKV_NALS_ADAPTATION nalsAdaptation = pStreamMkvGenerator->nalsAdaptation;
    UINT32 i;
    BOOL trackInfoFound = FALSE;

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
                                                      FALSE,
                                                      pBuffer + MKV_SIMPLE_BLOCK_BITS_SIZE,
                                                      &adaptedFrameSize));
    }

    // Encode and fix-up the size - encode 8 bytes
    encodedLength = 0x100000000000000ULL | (UINT64) (adaptedFrameSize + MKV_SIMPLE_BLOCK_PAYLOAD_HEADER_SIZE);
    putInt64((PINT64)(pBuffer + MKV_SIMPLE_BLOCK_SIZE_OFFSET), encodedLength);

    // Fix up the timecode
    putInt16((PINT16)(pBuffer + MKV_SIMPLE_BLOCK_TIMECODE_OFFSET), timestamp);

    // track must exist because we already checked in putKinesisVideoFrame
    for(i = 0; i < pStreamMkvGenerator->trackInfoCount; ++i) {
        if (pStreamMkvGenerator->trackInfoList[i].trackId == pFrame->trackId) {
            trackInfoFound = TRUE;
            break;
        }
    }
    CHK(trackInfoFound, STATUS_MKV_TRACK_INFO_NOT_FOUND);

    // fix up track number for each block
    *(pBuffer + MKV_SIMPLE_BLOCK_TRACK_NUMBER_OFFSET) = (UINT8) (0x80 | (UINT8) (i + 1));

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
                                                      FALSE,
                                                      NULL,
                                                      &adaptedFrameSize));
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

    dataSize = STRNLEN(pTrackInfo->codecId, MKV_MAX_CODEC_ID_LEN);
    if (dataSize > 0) {
        mkvgenEbmlEncodeNumber(dataSize, NULL, 0, &encodedLen);
        trackEntrySize += MKV_CODEC_ID_BITS_SIZE + encodedLen + dataSize;
    }

    dataSize = STRNLEN(pTrackInfo->trackName, MKV_MAX_TRACK_NAME_LEN);
    if (dataSize > 0) {
        mkvgenEbmlEncodeNumber(dataSize, NULL, 0, &encodedLen);
        trackEntrySize += MKV_TRACK_NAME_BITS_SIZE + encodedLen + dataSize;
    }

    if (GENERATE_AUDIO_CONFIG(pTrackInfo)) {
        trackEntrySize += MKV_TRACK_AUDIO_BITS_SIZE;
    } else if (GENERATE_VIDEO_CONFIG(pTrackInfo)) {
        trackEntrySize += MKV_TRACK_VIDEO_BITS_SIZE;
    }

    return trackEntrySize;
}

UINT32 mkvgenGetMkvTrackHeaderSize(PTrackInfo trackInfoList, UINT32 trackInfoCount) {
    UINT32 trackHeaderSize = MKV_TRACKS_ELEM_BITS_SIZE, i = 0;

    for (i = 0; i < trackInfoCount; ++i) {
        trackHeaderSize += mkvgenGetTrackEntrySize(&trackInfoList[i]);
    }

    return trackHeaderSize;
}

UINT32 mkvgenGetMkvSegmentTrackHeaderSize(PTrackInfo trackInfoList, UINT32 trackInfoCount) {
    return MKV_SEGMENT_INFO_BITS_SIZE + mkvgenGetMkvTrackHeaderSize(trackInfoList, trackInfoCount);
}

UINT32 mkvgenGetMkvHeaderSize(PTrackInfo trackInfoList, UINT32 trackInfoCount) {
    return MKV_EBML_SEGMENT_SIZE + mkvgenGetMkvSegmentTrackHeaderSize(trackInfoList, trackInfoCount);
}

UINT32 mkvgenGetMkvHeaderOverhead(PTrackInfo trackInfoList, UINT32 trackInfoCount) {
    return MKV_CLUSTER_OVERHEAD + mkvgenGetMkvHeaderSize(trackInfoList, trackInfoCount);
}

UINT32 mkvgenGetMkvSegmentTrackInfoOverhead(PTrackInfo trackInfoList, UINT32 trackInfoCount) {
    return MKV_CLUSTER_OVERHEAD + mkvgenGetMkvSegmentTrackHeaderSize(trackInfoList, trackInfoCount);
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

/**
 * Parse AAC Cpd
 */
STATUS getSamplingFreqAndChannelFromAacCpd(PBYTE pCpd,
                                           UINT32 cpdSize,
                                           PDOUBLE pSamplingFrequency, PUINT16 pChannelConfig){
    STATUS retStatus = STATUS_SUCCESS;
    INT16 cpdContainer;
    UINT16 samplingRateIdx, channelConfig;

    CHK(pSamplingFrequency != NULL && pChannelConfig != NULL, STATUS_NULL_ARG);
    CHK(cpdSize >= MIN_AAC_CPD_SIZE && pCpd != NULL, STATUS_MKV_INVALID_AAC_CPD);

    // AAC cpd are encoded in the first 2 bytes of the cpd
    cpdContainer = getInt16(*((PINT16) pCpd));

    /*
     * aac cpd structure
     * 5 bits (Audio Object Type) | 4 bits (frequency index) | 4 bits (channel configuration) | 3 bits (not used)
     */
    channelConfig = (UINT16) ((cpdContainer >> 3) & 0x000f);
    CHK(channelConfig < MKV_AAC_CHANNEL_CONFIG_MAX, STATUS_MKV_INVALID_AAC_CPD_CHANNEL_CONFIG);
    *pChannelConfig = channelConfig;

    samplingRateIdx = (UINT16) ((cpdContainer >> 7) & 0x000f);
    CHK(samplingRateIdx < MKV_AAC_SAMPLING_FREQUNECY_IDX_MAX, STATUS_MKV_INVALID_AAC_CPD_SAMPLING_FREQUENCY_INDEX);
    *pSamplingFrequency = gMkvAACSamplingFrequencies[samplingRateIdx];

CleanUp:
    return retStatus;
}
