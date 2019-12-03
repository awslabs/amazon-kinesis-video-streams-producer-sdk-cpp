/*******************************************
* Input validator include file
*******************************************/
#ifndef __INPUT_VALIDATOR_H__
#define __INPUT_VALIDATOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#pragma once

// For tight packing
#pragma pack(push, include_i, 1) // for byte alignment

////////////////////////////////////////////////////
// Internal functionality
////////////////////////////////////////////////////
/**
 * Validates the client callbacks structure
 *
 * @param 1 PDeviceInfo - Device info struct.
 * @param 2 PClientCallbacks - Client callbacks struct.
 *
 * @return Status of the function call.
 */
STATUS validateClientCallbacks(PDeviceInfo, PClientCallbacks);

/**
 * Validates the device info structure
 *
 * @param 1 PDeviceInfo - Device info struct.
 *
 * @return Status of the function call.
 */
STATUS validateDeviceInfo(PDeviceInfo);

/**
 * Validates the client info structure
 *
 * @param 1 PClientInfo - Client info struct.
 *
 * @return Status of the function call.
 */
STATUS validateClientInfo(PClientInfo);

/**
 * Validates the stream info structure
 *
 * @param 1 PStreamInfo - Stream info struct.
 * @param 2 PClientCallbacks - Client callbacks struct.
 *
 * @return Status of the function call.
 */
STATUS validateStreamInfo(PStreamInfo, PClientCallbacks);

/**
 * Validates the tags
 *
 * @param 1 UINT32 - Number of tags
 * @param 2 PTag - Array of tags
 *
 * @return Status of the function call.
 */
STATUS validateClientTags(UINT32, PTag);

/**
 * Sets the default values of the client info if needed
 *
 * @param 1 PClientInfo - Client info object to fixup
 *
 */
VOID fixupClientInfo(PClientInfo);

/**
 * Returns the size of the device info structure in bytes
 *
 * @param 1 PDeviceInfo - Device info object
 *
 * @return The size of the object
 */
SIZE_T sizeOfDeviceInfo(PDeviceInfo);

/**
 * Sets the default values of the stream description if needed
 *
 * @param 1 PStreamDescription - Stream description object to fixup
 *
 */
VOID fixupStreamDescription(PStreamDescription);

/**
 * Sets the default values of the StreamInfo if needed
 *
 * @param 1 PStreamInfo - StreamInfo object to fixup
 *
 */
VOID fixupStreamInfo(PStreamInfo pStreamInfo);

/**
 * Sets the default values of the TrackInfo if needed
 *
 * @param 1 PTrackInfo - TrackInfo object to fixup
 *
 */
VOID fixupTrackInfo(PTrackInfo pTrackInfo, UINT32 trackInfoCount);

/**
 * Sets the default values of the Frame if needed
 *
 * @param 1 PFrame - Frame object to fixup
 *
 */
VOID fixupFrame(PFrame pFrame);

/**
 * Returns the size of the stream description structure in bytes
 *
 * @param 1 PStreamDescription - Stream description object
 *
 * @return The size of the object
 */
SIZE_T sizeOfStreamDescription(PStreamDescription);

#pragma pack(pop, include_i)

#ifdef __cplusplus
}
#endif

#endif // __INPUT_VALIDATOR_H__
