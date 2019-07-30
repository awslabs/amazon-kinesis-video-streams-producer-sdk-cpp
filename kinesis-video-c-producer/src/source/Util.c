/*
 *  Kinesis Video C Producer Util
 */
#define LOG_CLASS "Util"

#include "Include_i.h"

#define EARLY_EXPIRATION_FACTOR                         1.0
#define IOT_EXPIRATION_PARSE_CONVERSION_BASE                     10

STATUS convertTimestampToEpoch(PCHAR expirationTimestampStr, UINT64 nowTime, PUINT64 pExpiration)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 iotExpirationInEpoch = 0, diff;

    UINT64 currentTimeInEpoch = (UINT64) ((MKTIME(GMTIME((time_t*) &nowTime))));
    struct tm ioTExpiration;

    CHK(expirationTimestampStr != NULL && pExpiration != NULL, STATUS_NULL_ARG);

    MEMSET(&ioTExpiration, 0x00, SIZEOF(struct tm));

    // iot expiration timestamp format "YYYY-MM-DDTHH:mm:SSZ"

    CHK_STATUS(STRTOUI32(expirationTimestampStr,
                         expirationTimestampStr + 4,
                         IOT_EXPIRATION_PARSE_CONVERSION_BASE,
                         (PUINT32) &ioTExpiration.tm_year));
    ioTExpiration.tm_year -= 1900;

    CHK_STATUS(STRTOUI32(expirationTimestampStr + 5,
                         expirationTimestampStr + 7,
                         IOT_EXPIRATION_PARSE_CONVERSION_BASE,
                         (PUINT32) &ioTExpiration.tm_mon));
    ioTExpiration.tm_mon -= 1;

    CHK_STATUS(STRTOUI32(expirationTimestampStr + 8,
                         expirationTimestampStr + 10,
                         IOT_EXPIRATION_PARSE_CONVERSION_BASE,
                         (PUINT32) &ioTExpiration.tm_mday));

    CHK_STATUS(STRTOUI32(expirationTimestampStr + 11,
                         expirationTimestampStr + 13,
                         IOT_EXPIRATION_PARSE_CONVERSION_BASE,
                         (PUINT32) &ioTExpiration.tm_hour));

    CHK_STATUS(STRTOUI32(expirationTimestampStr + 14,
                         expirationTimestampStr + 16,
                         IOT_EXPIRATION_PARSE_CONVERSION_BASE,
                         (PUINT32) &ioTExpiration.tm_min));

    CHK_STATUS(STRTOUI32(expirationTimestampStr + 17,
                         expirationTimestampStr + 19,
                         IOT_EXPIRATION_PARSE_CONVERSION_BASE,
                         (PUINT32) &ioTExpiration.tm_sec));

    CHK(ioTExpiration.tm_mon < 12 && ioTExpiration.tm_mon >= 0 &&
            ioTExpiration.tm_mday < 32 && ioTExpiration.tm_mday >= 1 &&
            ioTExpiration.tm_hour < 24 && ioTExpiration.tm_hour >= 0 &&
            ioTExpiration.tm_min < 60 && ioTExpiration.tm_min >= 0 &&
            ioTExpiration.tm_sec < 61 && ioTExpiration.tm_sec >= 0, STATUS_IOT_EXPIRATION_PARSING_FAILED);

    DLOGV("Expiration timestamp conversion into tm structure  %d-%d-%dT%d:%d:%d",
          ioTExpiration.tm_year,
          ioTExpiration.tm_mon,
          ioTExpiration.tm_mday,
          ioTExpiration.tm_hour,
          ioTExpiration.tm_min,
          ioTExpiration.tm_sec);

    iotExpirationInEpoch = (UINT64) MKTIME(&ioTExpiration);

    CHK(iotExpirationInEpoch >= currentTimeInEpoch, STATUS_IOT_EXPIRATION_OCCURS_IN_PAST);

    diff = iotExpirationInEpoch - currentTimeInEpoch;

    DLOGD("Difference between current time and iot expiration is %llu ", diff);

    *pExpiration = (UINT64) (nowTime + ((DOUBLE) diff * EARLY_EXPIRATION_FACTOR)) * HUNDREDS_OF_NANOS_IN_A_SECOND;

CleanUp:

    return retStatus;
}
