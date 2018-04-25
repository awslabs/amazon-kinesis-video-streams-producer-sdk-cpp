/**
 * Common definition file
 */

#ifndef __COMMON_DEFINITIONS__
#define __COMMON_DEFINITIONS__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////
// Project defines
////////////////////////////////////////////////////
#if defined _WIN32 || defined __CYGWIN__
    #if defined __GNUC__ || defined __GNUG__
        #define INLINE              inline
        #define LIB_EXPORT          __attribute__((dllexport))
        #define LIB_IMPORT          __attribute__((dllimport))
        #define LIB_INTERNAL
        #define ALIGN_4             __attribute__((aligned(4)))
        #define ALIGN_8             __attribute__((aligned(8)))
        #define PACKED              __attribute__((__packed__))
        #define DISCARDABLE
    #else
        #define INLINE              _inline
        #define LIB_EXPORT          __declspec(dllexport)
        #define LIB_IMPORT          __declspec(dllimport)
        #define LIB_INTERNAL
        #define ALIGN_4             __declspec(align(4))
        #define ALIGN_8             __declspec(align(8))
        #define PACKED
        #define DISCARDABLE         __declspec(selectany)
    #endif
#else
    #if __GNUC__ >= 4
        #define INLINE              inline
        #define LIB_EXPORT          __attribute__ ((visibility ("default")))
        #define LIB_IMPORT          __attribute__ ((visibility ("default")))
        #define LIB_INTERNAL        __attribute__ ((visibility ("hidden")))
        #define ALIGN_4             __attribute__((aligned(4)))
        #define ALIGN_8             __attribute__((aligned(8)))
        #define PACKED              __attribute__((__packed__))
        #define DISCARDABLE
    #else
        #define LIB_EXPORT
        #define LIB_IMPORT
        #define LIB_INTERNAL
        #define ALIGN_4
        #define ALIGN_8
        #define DISCARDABLE
        #define PACKED
    #endif
#endif

#define PUBLIC_API          LIB_EXPORT
#define PRIVATE_API         LIB_INTERNAL

//
// 64/32 bit check on Windows platforms
//
#if defined _WIN32 || defined _WIN64
    #if defined _WIN64
        #define SIZE_64
	    #define __LLP64__ // win64 uses LLP64 data model
    #else
        #define SIZE_32
    #endif
#endif

//
// 64/32 bit check on GCC
//
#if defined __GNUC__ || defined __GNUG__
    #if defined __x86_64__ || defined __ppc64__
        #define SIZE_64
        #define __LLP64__ // win64 uses LLP64 data model
    #else
        #define SIZE_32
    #endif
#endif

#if defined _MSC_VER

#ifndef _WINNT_
typedef char                    CHAR;
typedef short                   WCHAR;
typedef unsigned __int8         UINT8;
typedef __int8                  INT8;
typedef unsigned __int16        UINT16;
typedef __int16                 INT16;
typedef unsigned __int32        UINT32;
typedef __int32                 INT32;
typedef unsigned __int64        UINT64;
typedef __int64                 INT64;
typedef double                  DOUBLE;
typedef float                   FLOAT;
#endif

#elif defined (__GNUC__)

#include <stdint.h>

typedef char                    CHAR;
typedef short                   WCHAR;
typedef uint8_t                 UINT8;
typedef int8_t                  INT8;
typedef uint16_t                UINT16;
typedef int16_t                 INT16;
typedef uint32_t                UINT32;
typedef int32_t                 INT32;
typedef uint64_t                UINT64;
typedef int64_t                 INT64;
typedef double                  DOUBLE;
typedef float                   FLOAT;

#else

typedef char                    CHAR;
typedef short                   WCHAR;
typedef unsigned char           UINT8;
typedef char                    INT8;
typedef unsigned short          UINT16;
typedef short                   INT16;
typedef unsigned long           UINT32;
typedef long                    INT32;
typedef unsigned long long      UINT64;
typedef long long               INT64;
typedef double                  DOUBLE;
typedef float                   FLOAT;

#endif

#ifndef VOID
#define VOID    void
#endif

#ifdef __APPLE__
    #ifndef OBJC_BOOL_DEFINED
        #include "TargetConditionals.h"
        #if TARGET_IPHONE_SIMULATOR
            // iOS Simulator
            typedef bool                BOOL;
        #elif TARGET_OS_IPHONE
            // iOS device
            typedef signed char         BOOL;
        #elif TARGET_OS_MAC
            // Other kinds of Mac OS
            typedef INT32                BOOL;
        #else
            #   error "Unknown Apple platform"
        #endif
    #endif
#else
    typedef INT32                BOOL;
#endif

typedef UINT8                BYTE;
typedef VOID*                PVOID;
typedef BYTE*                PBYTE;
typedef BOOL*                PBOOL;
typedef CHAR*                PCHAR;
typedef WCHAR*               PWCHAR;
typedef INT8*                PINT8;
typedef UINT8*               PUINT8;
typedef INT16*               PINT16;
typedef UINT16*              PUINT16;
typedef INT32*               PINT32;
typedef UINT32*              PUINT32;
typedef INT64*               PINT64;
typedef UINT64*              PUINT64;
typedef long                 LONG, *PLONG;
typedef unsigned long        ULONG, *PULONG;
typedef DOUBLE*              PDOUBLE;
typedef FLOAT*               PFLOAT;

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

// Thread Id
typedef UINT64              TID;

// Mutex typedef
typedef UINT64              MUTEX;

// Max thread name buffer length - similar to Linux platforms
#ifndef MAX_THREAD_NAME
#define MAX_THREAD_NAME 16
#endif

// Max mutex name
#ifndef MAX_MUTEX_NAME
#define MAX_MUTEX_NAME 32
#endif

// Content ID - 64 bit uint
typedef UINT64              CID;
typedef CID*                PCID;

//
// int and long ptr definitions
//
#if defined SIZE_64

    typedef INT64                INT_PTR, *PINT_PTR;
    typedef UINT64               UINT_PTR, *PUINT_PTR;

    typedef INT64                LONG_PTR, *PLONG_PTR;
    typedef UINT64               ULONG_PTR, *PULONG_PTR;

    #ifndef PRIu64
        #ifdef __LLP64__
            #define PRIu64 "llu"
        #else // __LP64__
            #define PRIu64 "lu"
        #endif
    #endif

    #ifndef PRIx64
        #ifdef __LLP64__
            #define PRIx64 "llx"
        #else // __LP64__
            #define PRIx64 "lx"
        #endif
    #endif

    #ifndef PRId64
        #ifdef __LLP64__
            #define PRId64 "lld"
        #else // __LP64__
            #define PRId64 "ld"
        #endif
    #endif

#elif defined SIZE_32

    typedef INT32                INT_PTR, *PINT_PTR;
    typedef UINT32               UINT_PTR, *PUINT_PTR;

    #ifndef __int3264 // defined in Windows 'basetsd.h'
        typedef INT64                LONG_PTR, *PLONG_PTR;
        typedef UINT64               ULONG_PTR, *PULONG_PTR;
    #endif

    #ifndef PRIu64
        #define PRIu64 "llu"
    #endif

    #ifndef PRIx64
        #define PRIx64 "llx"
    #endif

    #ifndef PRId64
        #define PRId64 "lld"
    #endif

#else
    #error "Environment not 32 or 64-bit."
#endif

#ifndef NULL
    #ifdef __cplusplus
        #define NULL    0
    #else  /* __cplusplus */
        #define NULL    ((void *)0)
    #endif  /* __cplusplus */
#endif  /* NULL */

//
// Max and Min definitions
//
#define MAX_UINT8           ((UINT8) 0xff)
#define MAX_INT8            ((INT8) 0x7f)
#define MIN_INT8            ((INT8) 0x80)

#define MAX_UINT16          ((UINT16) 0xffff)
#define MAX_INT16           ((INT16) 0x7fff)
#define MIN_INT16           ((INT16) 0x8000)

#define MAX_UINT32          ((UINT32) 0xffffffff)
#define MAX_INT32           ((INT32) 0x7fffffff)
#define MIN_INT32           ((INT32) 0x80000000)

#define MAX_UINT64          ((UINT64) 0xffffffffffffffff)
#define MAX_INT64           ((INT64) 0x7fffffffffffffff)
#define MIN_INT64           ((INT64) 0x8000000000000000)

/**
 * NOTE: Timer precision is in 100ns intervals. This is used in heuristics and in time functionality
 */
#define DEFAULT_TIME_UNIT_IN_NANOS                          100
#define HUNDREDS_OF_NANOS_IN_A_MICROSECOND                  10LL
#define HUNDREDS_OF_NANOS_IN_A_MILLISECOND                  (HUNDREDS_OF_NANOS_IN_A_MICROSECOND * 1000LL)
#define HUNDREDS_OF_NANOS_IN_A_SECOND                       (HUNDREDS_OF_NANOS_IN_A_MILLISECOND * 1000LL)
#define HUNDREDS_OF_NANOS_IN_A_MINUTE                       (HUNDREDS_OF_NANOS_IN_A_SECOND * 60LL)
#define HUNDREDS_OF_NANOS_IN_AN_HOUR                        (HUNDREDS_OF_NANOS_IN_A_MINUTE * 60LL)

//
// Some standard definitions/macros
//
#ifndef SIZEOF
#define SIZEOF(x) (sizeof(x))
#endif

#ifndef UNUSED_PARAM
    #define UNUSED_PARAM(expr) do { (void)(expr); } while (0)
#endif

#ifndef ARRAY_SIZE
    #define ARRAY_SIZE(array) (sizeof(array) / sizeof *(array))
#endif

#ifndef SAFE_MEMFREE
    #define SAFE_MEMFREE(p) do {if (p) {MEMFREE(p); (p)=NULL;}} while (0)
#endif

#ifndef SAFE_DELETE
    #define SAFE_DELETE(p) do {if (p) {delete (p); (p)=NULL;}} while (0)
#endif

#ifndef SAFE_DELETE_ARRAY
    #define SAFE_DELETE_ARRAY(p) do {if (p) {delete[] (p); (p)=NULL;}} while (0)
#endif

#ifndef MIN
    #define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
    #define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef IS_ALIGNED_TO
    #define IS_ALIGNED_TO(m, n) ((m) % (n) == 0)
#endif

////////////////////////////////////////////////////
// Status definitions
////////////////////////////////////////////////////
#define STATUS UINT32

#define STATUS_SUCCESS    0x00000000

#define STATUS_FAILED(x) (((STATUS)(x)) != STATUS_SUCCESS)
#define STATUS_SUCCEEDED(x) (!STATUS_FAILED(x))

#define STATUS_BASE                                 0x00000000
#define STATUS_NULL_ARG                             STATUS_BASE + 0x00000001
#define STATUS_INVALID_ARG                          STATUS_BASE + 0x00000002
#define STATUS_INVALID_ARG_LEN                      STATUS_BASE + 0x00000003
#define STATUS_NOT_ENOUGH_MEMORY                    STATUS_BASE + 0x00000004
#define STATUS_BUFFER_TOO_SMALL                     STATUS_BASE + 0x00000005
#define STATUS_UNEXPECTED_EOF                       STATUS_BASE + 0x00000006
#define STATUS_FORMAT_ERROR                         STATUS_BASE + 0x00000007
#define STATUS_INVALID_HANDLE_ERROR                 STATUS_BASE + 0x00000008
#define STATUS_OPEN_FILE_FAILED                     STATUS_BASE + 0x00000009
#define STATUS_READ_FILE_FAILED                     STATUS_BASE + 0x0000000a
#define STATUS_WRITE_TO_FILE_FAILED                 STATUS_BASE + 0x0000000b
#define STATUS_INTERNAL_ERROR                       STATUS_BASE + 0x0000000c
#define STATUS_INVALID_OPERATION                    STATUS_BASE + 0x0000000d
#define STATUS_NOT_IMPLEMENTED                      STATUS_BASE + 0x0000000e
#define STATUS_OPERATION_TIMED_OUT                  STATUS_BASE + 0x0000000f
#define STATUS_NOT_FOUND                            STATUS_BASE + 0x00000010

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>

#include <dirent.h>
#include <unistd.h>

#if !defined(_MSC_VER) && !defined(__MINGW64__) && !defined(__MINGW32__) && !defined (__MACH__)
// NOTE!!! For some reason memalign is not included for Linux builds in stdlib.h
#include <malloc.h>
#endif

#if defined _WIN32 || defined _WIN64 || defined __CYGWIN__

#include <../utils/src/dlfcn_win_stub.h>

// Definition of the static global mutexes
#define GLOBAL_REENTRANT_MUTEX                      (MUTEX) 0
#define GLOBAL_NON_REENTRANT_MUTEX                  (MUTEX) 0

// Definition of the S_ISLNK for Windows
#define S_ISLNK(x) FALSE

// Definition of the mkdir for Windows with 1 param
#define GLOBAL_MKDIR(p1, p2) mkdir(p1)

#else

#include <dlfcn.h>
#include <pthread.h>

#if !defined (__MACH__)
#include <sys/prctl.h>
#endif

// Definition of the static global mutexes
extern pthread_mutex_t globalReentrantMutex;
extern pthread_mutex_t globalNonReentrantMutex;
#define GLOBAL_REENTRANT_MUTEX                      ((MUTEX) &globalReentrantMutex)
#define GLOBAL_NON_REENTRANT_MUTEX                  ((MUTEX) &globalNonReentrantMutex)

// Definition of the mkdir for non-Windows platforms with 2 params
#define GLOBAL_MKDIR(p1, p2) mkdir((p1), (p2))

#endif
/**
 * Allocator function definitions
 */
typedef PVOID (*memAlloc)(UINT32 size);
typedef PVOID (*memAlignAlloc)(UINT32 size, UINT32 alignment);
typedef PVOID (*memCalloc)(UINT32 num, UINT32 size);
typedef VOID (*memFree)(PVOID ptr);

//
// Default allocator functions
//
INLINE PVOID defaultMemAlloc(UINT32 size)
{
    return malloc(size);
}

INLINE PVOID defaultMemAlignAlloc(UINT32 size, UINT32 alignment)
{
#if defined (__MACH__)
    // On Mac allocations are 16 byte aligned. There is hardly an equivalent anyway
    UNUSED_PARAM(alignment);
    return malloc(size);
#elif defined(_MSC_VER) || defined(__MINGW64__) || defined(__MINGW32__)
    return _aligned_malloc(size, alignment);
#else
    return memalign(size, alignment);
#endif
}

INLINE PVOID defaultMemCalloc(UINT32 num, UINT32 size)
{
    return calloc(num, size);
}

INLINE VOID defaultMemFree(VOID* ptr)
{
    free(ptr);
}

/**
 * Global allocator function pointers
 */
extern memAlloc globalMemAlloc;
extern memAlignAlloc globalMemAlignAlloc;
extern memCalloc globalMemCalloc;
extern memFree globalMemFree;

/**
 * Dynamic library loading function definitions
 */
typedef PVOID (*dlOpen)(PCHAR filename, UINT32 flag);
typedef INT32 (*dlClose)(PVOID handle);
typedef PVOID (*dlSym)(PVOID handle, PCHAR symbol);
typedef PCHAR (*dlError)();

//
// Default dynamic library loading functions
//
INLINE PVOID defaultDlOpen(PCHAR filename, UINT32 flag)
{
    return dlopen((const PCHAR) filename, flag);
}

INLINE INT32 defaultDlClose(PVOID handle)
{
    return dlclose(handle);
}

INLINE PVOID defaultDlSym(PVOID handle, PCHAR symbol)
{
    return dlsym(handle, symbol);
}

INLINE PCHAR defaultDlError()
{
    return (PCHAR) dlerror();
}

/**
 * Global dynamic library loading function pointers
 */
extern dlOpen globalDlOpen;
extern dlClose globalDlClose;
extern dlSym globalDlSym;
extern dlError globalDlError;

/**
 * Thread library function definitions
 */
typedef TID (*getTId)();
typedef STATUS (*getTName)(TID, PCHAR, UINT32);

/**
 * Thread related functionality
 */
extern getTId globalGetThreadId;
extern getTName globalGetThreadName;

/**
 * Time library function definitions
 */
typedef UINT64 (*getTime)();

//
// Default time library functions
//
INLINE UINT64 defaultGetTime()
{
#if defined __MACH__ || defined _WIN32 || defined _WIN64 || defined __CYGWIN__
    struct timeval nowTime;
    if (0 != gettimeofday(&nowTime, NULL)) {
        return 0;
    }

    return (UINT64) nowTime.tv_sec * HUNDREDS_OF_NANOS_IN_A_SECOND + (UINT64) nowTime.tv_usec * HUNDREDS_OF_NANOS_IN_A_MICROSECOND;
#else
    struct timespec nowTime;
    clock_gettime(CLOCK_MONOTONIC, &nowTime);

    // The precision needs to be on a 100th nanosecond resolution
    return (UINT64) nowTime.tv_sec * HUNDREDS_OF_NANOS_IN_A_SECOND + (UINT64) nowTime.tv_nsec / 100;
#endif
}

/**
 * Thread related functionality
 */
extern getTime globalGetTime;

/**
 * Thread library function definitions
 */
typedef MUTEX (*createMutex)(BOOL);
typedef VOID (*lockMutex)(MUTEX);
typedef VOID (*unlockMutex)(MUTEX);
typedef VOID (*tryLockMutex)(MUTEX);
typedef VOID (*freeMutex)(MUTEX);

/**
 * Mutex related functionality
 */
extern createMutex globalCreateMutex;
extern lockMutex globalLockMutex;
extern unlockMutex globalUnlockMutex;
extern tryLockMutex globalTryLockMutex;
extern freeMutex globalFreeMutex;

/**
 * Memory allocation and operations
 */
#define MEMALLOC                   globalMemAlloc
#define MEMALIGNALLOC              globalMemAlignAlloc
#define MEMCALLOC                  globalMemCalloc
#define MEMFREE                    globalMemFree
#define MEMCMP                     memcmp
#define MEMCPY                     memcpy
#define MEMSET                     memset
#define MEMMOVE                    memmove

/**
 * String operations
 */
#define STRCAT                     strcat
#define STRNCAT                    strncat
#define STRCPY                     strcpy
#define STRNCPY                    strncpy
#define STRLEN                     strlen
#define STRNLEN                    strnlen
#define STRCHR                     strchr
#define STRRCHR                    strrchr
#define STRCMP                     strcmp
#define STRNCMP                    strncmp
#define PRINTF                     printf
#define SPRINTF                    sprintf
#define SNPRINTF                   snprintf

/**
 * Pseudo-random functionality
 */
#ifndef SRAND
#define SRAND                           srand
#endif
#ifndef RAND
#define RAND                            rand
#endif

/**
 * CRT functionality
 */
#define STRTOUL                    strtoul
#define ULLTOSTR                   ulltostr
#define ULTOSTR                    ultostr

/**
 * File operations
 */
#ifndef FOPEN
    #define FOPEN                       fopen
#endif
#ifndef FCLOSE
    #define FCLOSE                      fclose
#endif
#ifndef FWRITE
    #define FWRITE                      fwrite
#endif
#ifndef FPUTC
    #define FPUTC                       fputc
#endif
#ifndef FREAD
    #define FREAD                       fread
#endif
#ifndef FSEEK
    #define FSEEK                       fseek
#endif
#ifndef FTELL
    #define FTELL                       ftell
#endif
#ifndef FREMOVE
    #define FREMOVE                     remove
#endif
#ifndef FUNLINK
    #define FUNLINK                     unlink
#endif
#ifndef FREWIND
    #define FREWIND                     rewind
#endif
#ifndef FRENAME
    #define FRENAME                     rename
#endif
#ifndef FTMPFILE
    #define FTMPFILE                    tmpfile
#endif
#ifndef FTMPNAME
    #define FTMPNAME                    tmpnam
#endif
#ifndef FOPENDIR
    #define FOPENDIR                    opendir
#endif
#ifndef FCLOSEDIR
    #define FCLOSEDIR                   closedir
#endif
#ifndef FREADDIR
    #define FREADDIR                    readdir
#endif
#ifndef FMKDIR
    #define FMKDIR                      GLOBAL_MKDIR
#endif
#ifndef FRMDIR
    #define FRMDIR                      rmdir
#endif
#ifndef FSTAT
    #define FSTAT                       stat
#endif

#if defined _WIN32 || defined _WIN64 || defined __CYGWIN__
    #define FPATHSEPARATOR              '\\'
#else
    #define FPATHSEPARATOR              '/'
#endif
/**
 * String to integer conversion
 */
#define STRTOUI32                   strtoui32
#define STRTOI32                    strtoi32
#define STRTOUI64                   strtoui64
#define STRTOI64                    strtoi64

/**
 * Dynamic library loading routines
 */
#define DLOPEN                      globalDlOpen
#define DLCLOSE                     globalDlClose
#define DLSYM                       globalDlSym
#define DLERROR                     globalDlError

/**
 * Thread functionality
 */
#define GETTID                      globalGetThreadId
#define GETTNAME                    globalGetThreadName

/**
 * Time functionality
 */
#define GETTIME                     globalGetTime

/**
 * Mutex functionality
 */
#define MUTEX_CREATE                globalCreateMutex
#define MUTEX_LOCK                  globalLockMutex
#define MUTEX_UNLOCK                globalUnlockMutex
#define MUTEX_TRYLOCK               globalTryLockMutex
#define MUTEX_FREE                  globalFreeMutex

#ifndef SQRT
#include <math.h>
    #define SQRT sqrt
#endif

//
// Calculate the byte offset of a field in a structure of type type.
//
#define FIELD_OFFSET(type, field)    ((LONG)(LONG_PTR)&(((type *)0)->field))

//
// Aligns an integer X value up or down to alignment A
//
#define ROUND_DOWN(X, A)        ((X) & ~((A) - 1))
#define ROUND_UP(X, A)          (((X) + (A) - 1) & ~((A) - 1))

//
// Macros to swap endinanness
//
#define LOW_BYTE(x)              ((BYTE)(x))
#define HIGH_BYTE(x)             ((BYTE)(((INT16)(x) >> 8) & 0xFF))
#define LOW_INT16(x)             ((INT16)(x))
#define HIGH_INT16(x)            ((INT16)(((INT32)(x) >> 16) & 0xFFFF))
#define LOW_INT32(x)             ((INT32)(x))
#define HIGH_INT32(x)            ((INT32)(((INT64)(x) >> 32) & 0xFFFFFFFF))

#define MAKE_INT16(a, b)         ((INT16)(((UINT8)((UINT16)(a) & 0xff)) | ((UINT16)((UINT8)((UINT16)(b) & 0xff))) << 8))
#define MAKE_INT32(a, b)         ((INT32)(((UINT16)((UINT32)(a) & 0xffff)) | ((UINT32)((UINT16)((UINT32)(b) & 0xffff))) << 16))
#define MAKE_INT64(a, b)         ((INT64)(((UINT32)((UINT64)(a) & 0xffffffff)) | ((UINT64)((UINT32)((UINT64)(b) & 0xffffffff))) << 32))

#define SWAP_INT16(x) MAKE_INT16( \
    HIGH_BYTE(x), \
    LOW_BYTE(x) \
    )

#define SWAP_INT32(x) MAKE_INT32( \
    SWAP_INT16(HIGH_INT16(x)), \
    SWAP_INT16(LOW_INT16(x)) \
    )

#define SWAP_INT64(x) MAKE_INT64( \
    SWAP_INT32(HIGH_INT32(x)), \
    SWAP_INT32(LOW_INT32(x)) \
    )

/**
 * Check if at most 1 bit is set
 */
#define CHECK_POWER_2(x)            !((x) & ((x) - 1))

/**
 * Checks if only 1 bit is set
 */
#define CHECK_SINGLE_BIT_SET(x)    ((x) && CHECK_POWER_2(x))

//
// Handle definitions
//
#if !defined(HANDLE) && !defined(_WINNT_)
typedef UINT64 HANDLE;
#endif
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((UINT64) NULL)
#endif
#ifndef IS_VALID_HANDLE
#define IS_VALID_HANDLE(h) ((h) != INVALID_HANDLE_VALUE)
#endif
#ifndef POINTER_TO_HANDLE
#define POINTER_TO_HANDLE(h) ((UINT64) (h))
#endif
#ifndef HANDLE_TO_POINTER
#define HANDLE_TO_POINTER(h) ((PBYTE) (h))
#endif

////////////////////////////////////////////////////
// Conditional checks
////////////////////////////////////////////////////
#define CHK(condition, errRet) \
    do { \
        if (!(condition)) { \
            retStatus = (errRet); \
            goto CleanUp; \
        } \
    } while (FALSE)

#define CHK_ERR(condition, errRet, errorMessage, ...) \
    do { \
        if (!(condition)) { \
            retStatus = (errRet); \
            DLOGE(errorMessage, ##__VA_ARGS__); \
            goto CleanUp; \
        } \
    } while (FALSE)

#define CHK_STATUS_ERR(condition, errRet, errorMessage, ...) \
    do { \
        STATUS __status = condition; \
        if (STATUS_FAILED(__status)) { \
            retStatus = (__status); \
            DLOGE(errorMessage, ##__VA_ARGS__); \
            goto CleanUp; \
        } \
    } while (FALSE)

#define CHK_STATUS(condition) \
    do { \
        STATUS __status = condition; \
        if (STATUS_FAILED(__status)) { \
            retStatus = (__status); \
            goto CleanUp; \
        } \
    } while (FALSE)

#define CHK_HANDLE(handle) \
    do { \
        if (IS_VALID_HANDLE(handle)) { \
            retStatus = (STATUS_INVALID_HANDLE_ERROR); \
            goto CleanUp; \
        } \
    } while (FALSE)

#define CHK_LOG(condition, logMessage) \
    do { \
        STATUS __status = condition; \
        if (STATUS_FAILED(__status)) { \
            DLOGS("%s Returned status code: 0x%08x", logMessage, __status); \
        } \
    } while (FALSE)

#ifdef  __cplusplus
}
#endif
#endif  /* __COMMON_DEFINITIONS__ */
