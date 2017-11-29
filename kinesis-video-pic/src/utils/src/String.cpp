#include "Include_i.h"

CHAR ALPHA_NUM[] = "0123456789abcdefghijklmnopqrstuvwxyz";

/**
 * Converts an uint64 to string. This implementation is due to the fact that not all platforms support itoa type of functionality
 */
STATUS ulltostr(UINT64 value, PCHAR pStr, UINT32 size, UINT32 base, PUINT32 pSize)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT32 remainder;
    UINT32 i;
    UINT32 curSize = 0;
    CHAR ch;

    CHK(pStr != NULL, STATUS_NULL_ARG);

    // Should have at least 2 bytes - including null terminator
    CHK(size >= 2, STATUS_BUFFER_TOO_SMALL);

    // Check the base
    CHK(base >= 2 && base <= MAX_STRING_CONVERSION_BASE, STATUS_INVALID_BASE);

    // Quick special case check for 0
    if (value == 0) {
        pStr[0] = '0';
        pStr[1] = '\0';

        if (pSize != NULL) {
            *pSize = 1;
        }

        // Return SUCCESS
        CHK(FALSE, STATUS_SUCCESS);
    }

    while (value != 0) {
        remainder = value % base;
        value = value / base;

        // Need space for the NULL terminator
        if (curSize >= size - 1) {
            return STATUS_BUFFER_TOO_SMALL;
        }

        pStr[curSize] = ALPHA_NUM[remainder];
        curSize++;
    }

    // Swap the chars
    for (i = 0; i < curSize / 2; i++) {
        ch = pStr[i];
        pStr[i] = pStr[curSize - i - 1];
        pStr[curSize - i - 1] = ch;
    }

    // Set the string terminator
    pStr[curSize] = '\0';

    // Set the last character pointer
    if (pSize != NULL) {
        *pSize = curSize;
    }

CleanUp:

    return retStatus;
}

STATUS ultostr(UINT32 value, PCHAR pStr, UINT32 size, UINT32 base, PUINT32 pSize)
{
    return ulltostr(value, pStr, size, base, pSize);
}

STATUS strtoint(PCHAR pStart, PCHAR pEnd, UINT32 base, PUINT64 pRet, PBOOL pSign)
{
    STATUS retStatus = STATUS_SUCCESS;
    PCHAR pCur = pStart;
    BOOL seenChars = FALSE;
    BOOL positive = TRUE;
    UINT64 result = 0;
    UINT64 digit;
    CHAR curChar;

    // Simple check for NULL
    CHK(pCur != NULL && pRet != NULL && pSign != NULL, STATUS_NULL_ARG);

    // Check for start and end pointers if end is specified
    CHK(pEnd == NULL || pEnd >= pCur, STATUS_INVALID_ARG);

    // Check the base
    CHK(base >= 2 && base <= MAX_STRING_CONVERSION_BASE, STATUS_INVALID_BASE);

    // Check the sign
    switch (*pCur) {
        case '-':
            positive = FALSE;
            // Deliberate fall-through
        case '+':
            pCur++;
        default:
            break;
    }

    while (pCur != pEnd && *pCur != '\0')
    {
        curChar = *pCur;
        if (curChar >= '0' && curChar <= '9') {
            digit = curChar - '0';
        } else if (curChar >= 'a' && curChar <= 'z') {
            digit = curChar - 'a' + 10;
        } else if (curChar >= 'A' && curChar <= 'Z') {
            digit = curChar - 'A' + 10;
        } else {
            CHK(FALSE, STATUS_INVALID_DIGIT);
        }

        // Set as processed
        seenChars = TRUE;

        // Check against the base
        CHK(digit < base, STATUS_INVALID_BASE);

        // Safe operation which results in
        // result = result * base + digit;
        CHK_STATUS(unsignedSafeMultiplyAdd(result, base, digit, &result));

        pCur++;
    }

    CHK(seenChars, STATUS_EMPTY_STRING);

    if (!positive) {
        result = (UINT64)((INT64)result * -1);
    }

    *pRet = result;
    *pSign = positive;

CleanUp:

    return retStatus;
}

STATUS strtoi64(PCHAR pStart, PCHAR pEnd, UINT32 base, PINT64 pRet)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 result;
    BOOL sign;

    CHK(pRet != NULL, STATUS_NULL_ARG);

    // Convert to UINT64
    CHK_STATUS(strtoint(pStart, pEnd, base, &result, &sign));

    // Check for the overflow
    if (sign) {
        CHK((INT64) result >= 0, STATUS_INT_OVERFLOW);
    } else {
        CHK((INT64) result <= 0, STATUS_INT_OVERFLOW);
    }

    *pRet = (INT64) result;

    CleanUp:

    return retStatus;
}

STATUS strtoui64(PCHAR pStart, PCHAR pEnd, UINT32 base, PUINT64 pRet)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 result;
    BOOL sign;

    CHK(pRet != NULL, STATUS_NULL_ARG);

    // Convert to UINT64
    CHK_STATUS(strtoint(pStart, pEnd, base, &result, &sign));

    // Check for the overflow
    CHK(sign, STATUS_INVALID_DIGIT);

    *pRet = result;

CleanUp:

    return retStatus;
}

STATUS strtoi32(PCHAR pStart, PCHAR pEnd, UINT32 base, PINT32 pRet)
{
    STATUS retStatus = STATUS_SUCCESS;
    INT64 result;
    CHK(pRet != NULL, STATUS_NULL_ARG);

    // Convert to INT64
    CHK_STATUS(strtoi64(pStart, pEnd, base, &result));

    // Check for the overflow
    CHK(result >= (INT64) MIN_INT32 && result <= (INT64) MAX_INT32, STATUS_INT_OVERFLOW);

    *pRet = (INT32) result;

CleanUp:

    return retStatus;
}

STATUS strtoui32(PCHAR pStart, PCHAR pEnd, UINT32 base, PUINT32 pRet)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 result;
    CHK(pRet != NULL, STATUS_NULL_ARG);

    // Convert to UINT64
    CHK_STATUS(strtoui64(pStart, pEnd, base, &result));

    // Check for the overflow
    CHK(result <= (UINT64) MAX_UINT32, STATUS_INT_OVERFLOW);

    *pRet = (UINT32) result;

CleanUp:

    return retStatus;
}

STATUS unsignedSafeMultiplyAdd(UINT64 multiplicand, UINT64 multiplier, UINT64 addend, PUINT64 pResult)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 multiplicandHi, multiplicandLo, multiplierLo, multiplierHi, intermediate, result;

    CHK(pResult != NULL, STATUS_NULL_ARG);

    // Set the result to 0 to start with
    *pResult = 0;

    // Quick special case handling
    if (multiplicand == 0 || multiplier == 0) {
        // Early successfull return - just return the addend
        *pResult = addend;

        CHK(FALSE, STATUS_SUCCESS);
    }

    // Perform the multiplication first
    // multiplicand * multiplier == (multiplicandHi + multiplicandLo) * (multiplierHi + multiplierLo)
    // which evaluates to
    // multiplicandHi * multiplierHi +
    // multiplicandHi * multiplierLo + multiplicandLo * multiplierHi +
    // multiplicandLo * multiplierLo
    multiplicandLo = multiplicand & 0x00000000ffffffff;
    multiplicandHi = (multiplicand & 0xffffffff00000000) >> 32;
    multiplierLo = multiplier & 0x00000000ffffffff;
    multiplierHi = (multiplier & 0xffffffff00000000) >> 32;

    // If both high parts are non-0 then we do have an overflow
    if (multiplicandHi != 0 && multiplierHi != 0) {
        CHK_STATUS(STATUS_INT_OVERFLOW);
    }

    // Intermediate result shouldn't overflow
    // intermediate = multiplicandHi * multiplierLo + multiplicandLo * multiplierHi;
    // as we have multiplicandHi or multiplierHi being 0
    intermediate = multiplicandHi * multiplierLo + multiplicandLo * multiplierHi;

    // Check if we overflowed the 32 bit
    if (intermediate > 0x00000000ffffffff) {
        CHK_STATUS(STATUS_INT_OVERFLOW);
    }

    // The resulting multiplication is
    // result = intermediate << 32 + multiplicandLo * multiplierLo
    // after which we need to add the addend
    intermediate <<= 32;

    result = intermediate + multiplicandLo * multiplierLo;

    if (result < intermediate) {
        CHK_STATUS(STATUS_INT_OVERFLOW);
    }

    // Finally, add the addend
    intermediate = result;
    result += addend;

    if (result < intermediate) {
        CHK_STATUS(STATUS_INT_OVERFLOW);
    }

    *pResult = result;

CleanUp:

    return retStatus;
}
