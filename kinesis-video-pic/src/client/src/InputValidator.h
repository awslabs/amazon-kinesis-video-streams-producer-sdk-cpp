/*******************************************
* Input validator include file
*******************************************/
#ifndef __INPUT_VALIDATOR_H__
#define __INPUT_VALIDATOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#pragma once

////////////////////////////////////////////////////
// General defines and data structures
////////////////////////////////////////////////////
/**
 * Forward declarations
 */
typedef struct __ClientCallbacks ClientCallbacks;
typedef __ClientCallbacks* PClientCallbacks;

typedef struct __DeviceInfo DeviceInfo;
typedef __DeviceInfo* PDeviceInfo;

typedef struct __StreamInfo StreamInfo;
typedef __StreamInfo* PStreamInfo;

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
STATUS validateTags(UINT32, PTag);

#ifdef __cplusplus
}
#endif

#endif // __INPUT_VALIDATOR_H__
