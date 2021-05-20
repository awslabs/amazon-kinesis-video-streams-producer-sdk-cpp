#ifndef __KINESISVIDEO_FILE_LOGGER_CALLBACKS_INCLUDE_I__
#define __KINESISVIDEO_FILE_LOGGER_CALLBACKS_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////
// File logging functionality
/////////////////////////////////////////

/**
 * This callback is supposed to be called when callbacks are getting freed. It will free the underlying PFileLogger.
 *
 * @return - STATUS of execution
 */
STATUS freeFileLoggerPlatformCallbacksFunc(PUINT64);

#ifdef __cplusplus
}
#endif
#endif /* __KINESISVIDEO_FILE_LOGGER_CALLBACKS_INCLUDE_I__ */