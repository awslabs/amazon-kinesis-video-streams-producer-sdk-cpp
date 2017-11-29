/**
 * ISO 8601 period format parser for DASH
 */
#ifndef __PERIOD_PARSER_H__
#define __PERIOD_PARSER_H__

#ifdef __cplusplus
extern "C" {
#endif

#pragma once

#include <com/amazonaws/kinesis/video/common/CommonDefs.h>

/**
 * Represents an invalid duration which might be returned from the parser
 */
#define INVALID_DURATION_VALUE (0xFFFFFFFFFFFFFFFF)

/**
 * Parses the DASH duration and returns a 64 bit integer representing the
 * number of 100 ns corresponding to the period
 */
UINT64 parseDuration(PCHAR, PCHAR);

#ifdef __cplusplus
}
#endif

#endif // __PERIOD_PARSER_H__
