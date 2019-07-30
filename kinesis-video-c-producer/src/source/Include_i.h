/*******************************************
Main internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_PRODUCER_INCLUDE_I__
#define __KINESIS_VIDEO_PRODUCER_INCLUDE_I__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////
// Project forward declarations
////////////////////////////////////////////////////
typedef struct __ConnectionStaleStateMachine ConnectionStaleStateMachine;
typedef ConnectionStaleStateMachine* PConnectionStaleStateMachine;
typedef struct __StreamLatencyStateMachine StreamLatencyStateMachine;
typedef StreamLatencyStateMachine* PStreamLatencyStateMachine;

////////////////////////////////////////////////////
// Project include files
////////////////////////////////////////////////////
#include <com/amazonaws/kinesis/video/cproducer/Include.h>
#include <com/amazonaws/kinesis/video/client/Include.h>
#include <jsmn.h>
#include <curl/curl.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>

#if !defined __WINDOWS_BUILD__
#include <signal.h>
#endif

// For tight packing
#pragma pack(push, include_i, 1) // for byte alignment

/**
 * Continuous retry state machinery states
 */
typedef enum {
    STREAM_CALLBACK_HANDLING_STATE_NORMAL_STATE,
    STREAM_CALLBACK_HANDLING_STATE_RESET_CONNECTION_STATE,
    STREAM_CALLBACK_HANDLING_STATE_RESET_STREAM_STATE,
    STREAM_CALLBACK_HANDLING_STATE_THROTTLE_PIPELINE_STATE,
    STREAM_CALLBACK_HANDLING_STATE_INFINITE_RETRY_STATE
} STREAM_CALLBACK_HANDLING_STATE;

/**
 * Default stream mapping hash table bucket count/length
 */
#define STREAM_MAPPING_HASH_TABLE_BUCKET_LENGTH        2
#define STREAM_MAPPING_HASH_TABLE_BUCKET_COUNT         100

////////////////////////////////////////////////////
// Project internal includes
////////////////////////////////////////////////////
#include "Version.h"
#include "Auth.h"
#include "Request.h"
#include "Response.h"
#include "AwsV4Signer.h"
#include "CallbacksProvider.h"
#include "FileAuthCallbacks.h"
#include "CurlApiCallbacks.h"
#include "DeviceInfoProvider.h"
#include "CallbacksProvider.h"
#include "ConnectionStaleStateMachine.h"
#include "StreamLatencyStateMachine.h"
#include "ContinuousRetryStreamCallbacks.h"
#include "StaticAuthCallbacks.h"
#include "StreamInfoProvider.h"
#include "IotAuthCallback.h"
#include "Util.h"
#include "FileLoggerPlatformCallbackProvider.h"

////////////////////////////////////////////////////
// Project internal defines
////////////////////////////////////////////////////

////////////////////////////////////////////////////
// Project internal functions
////////////////////////////////////////////////////

#pragma pack(pop, include_i)

#ifdef  __cplusplus
}
#endif
#endif  /* __KINESIS_VIDEO_PRODUCER_INCLUDE_I__ */
