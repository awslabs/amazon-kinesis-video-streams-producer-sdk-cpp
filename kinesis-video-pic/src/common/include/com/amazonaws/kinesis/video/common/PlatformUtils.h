/**
 * Platform specific definitions for logging, assert, etc..
 */
#ifndef __PLATFORM_UTILS_DEFINES_H__
#define __PLATFORM_UTILS_DEFINES_H__

#ifdef __cplusplus
extern "C" {
#endif

#pragma once

// Tag for the logging
#ifndef LOG_TAG
#define LOG_TAG "platform-utils"
#endif

#ifdef ANDROID_BUILD
// Compiling with NDK
#include <android/log.h>
#define __LOG(p1, p2, p3, ...)     __android_log_print(p1, p2, p3, ##__VA_ARGS__)
#define __ASSERT(p1, p2, p3, ...)  __android_log_assert(p1, p2, p3, ##__VA_ARGS__)
#else
// Compiling under non-NDK
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#define __LOG(p1, p2, p3, ...)     printf(p3, ##__VA_ARGS__)
#define __ASSERT(p1, p2, p3, ...)  assert(p1)


#define ANDROID_LOG_VERBOSE         1
#define ANDROID_LOG_DEBUG           2
#define ANDROID_LOG_INFO            3
#define ANDROID_LOG_WARN            4
#define ANDROID_LOG_ERROR           5
#define ANDROID_LOG_FATAL           6
#define ANDROID_LOG_SILENT          7
#endif // ANDROID_BUILD

// Extra logging macros
#ifndef DLOGE
#define DLOGE(fmt, ...) __LOG(ANDROID_LOG_ERROR, LOG_TAG, "\n%s(): " fmt, __FUNCTION__, ##__VA_ARGS__)
#endif
#ifndef DLOGW
#define DLOGW(fmt, ...) __LOG(ANDROID_LOG_WARN, LOG_TAG, "\n%s(): " fmt, __FUNCTION__, ##__VA_ARGS__)
#endif
#ifndef DLOGI
#define DLOGI(fmt, ...) __LOG(ANDROID_LOG_INFO, LOG_TAG, "\n%s(): " fmt, __FUNCTION__, ##__VA_ARGS__)
#endif
#ifndef DLOGV
#define DLOGV(fmt, ...) __LOG(ANDROID_LOG_VERBOSE, LOG_TAG, "\n%s(): " fmt, __FUNCTION__, ##__VA_ARGS__)
#endif
#ifndef ENTER
#define ENTER() DLOGV("Enter")
#endif
#ifndef LEAVE
#define LEAVE() DLOGV("Leave")
#endif
#ifndef ENTERS
#define ENTERS() DLOGS("Enter")
#endif
#ifndef LEAVES
#define LEAVES() DLOGS("Leave")
#endif

// Optionally log very verbose data (>200 msgs/second) about the streaming process
#ifndef DLOGS
#ifdef LOG_STREAMING
    #define DLOGS DLOGV
#else
    #define DLOGS(...) do {} while (0)
#endif
#endif

// Assertions
#ifndef CONDITION
#ifdef __GNUC__
#define CONDITION(cond)     (__builtin_expect((cond)!=0, 0))
#else
#define CONDITION(cond)     (((cond)!=0) == 0)
#endif
#endif
#ifndef LOG_ALWAYS_FATAL_IF
#define LOG_ALWAYS_FATAL_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)__ASSERT(FALSE, LOG_TAG, ## __VA_ARGS__)) \
    : (void)0 )
#endif

#ifndef LOG_ALWAYS_FATAL
#define LOG_ALWAYS_FATAL(...) \
    ( ((void)__ASSERT(FALSE, LOG_TAG, ## __VA_ARGS__)) )
#endif

#ifndef SANITIZED_FILE
#define SANITIZED_FILE (STRRCHR(__FILE__, '/') ? STRRCHR(__FILE__, '/') + 1 : __FILE__)
#endif
#ifndef CRASH
#define CRASH(fmt, ...) LOG_ALWAYS_FATAL("%s::%s: " fmt, LOG_TAG, __FUNCTION__, ##__VA_ARGS__)
#endif
#ifndef CHECK
#define CHECK(x) LOG_ALWAYS_FATAL_IF(!(x), "%s::%s: ASSERTION FAILED at %s:%d: " #x, LOG_TAG, __FUNCTION__, SANITIZED_FILE, __LINE__)
#endif
#ifndef CHECK_EXT
#define CHECK_EXT(x, fmt, ...) LOG_ALWAYS_FATAL_IF(!(x), "%s::%s: ASSERTION FAILED at %s:%d: " fmt, LOG_TAG, __FUNCTION__, SANITIZED_FILE, __LINE__, ##__VA_ARGS__)
#endif

#ifdef __cplusplus
}
#endif

#endif // __PLATFORM_UTILS_DEFINES_H__
