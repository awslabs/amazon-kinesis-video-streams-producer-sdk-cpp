/**
 * Endianness routines
 */

#include "Include_i.h"

//
// Global var defining runtime endianness
//
BOOL g_BigEndian;

//
// Functions dealing with endianness and swapping
//
INT16 getInt16Swap(INT16 x)
{
    return SWAP_INT16(x);
}

INT16 getInt16NoSwap(INT16 x)
{
    return x;
}

INT32 getInt32Swap(INT32 x)
{
    return SWAP_INT32(x);
}

INT32 getInt32NoSwap(INT32 x)
{
    return x;
}

INT64 getInt64Swap(INT64 x)
{
    return SWAP_INT64(x);
}

INT64 getInt64NoSwap(INT64 x)
{
    return x;
}

VOID putInt16Swap(PINT16 px, INT16 x)
{
    *px = SWAP_INT16(x);
}

VOID putInt16NoSwap(PINT16 px, INT16 x)
{
    *px = x;
}

VOID putInt32Swap(PINT32 px, INT32 x)
{
    *px = SWAP_INT32(x);
}

VOID putInt32NoSwap(PINT32 px, INT32 x)
{
    *px = x;
}

VOID putInt64Swap(PINT64 px, INT64 x)
{
    *px = SWAP_INT64(x);
}

VOID putInt64NoSwap(PINT64 px, INT64 x)
{
    *px = x;
}

//
// Default initialization of the global functions
//
getInt16Func getInt16 = getInt16NoSwap;
getInt32Func getInt32 = getInt32NoSwap;
getInt64Func getInt64 = getInt64NoSwap;

putInt16Func putInt16 = putInt16NoSwap;
putInt32Func putInt32 = putInt32NoSwap;
putInt64Func putInt64 = putInt64NoSwap;


//////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////

//
// Checking run-time endianness.
// Other methods checking for byte placement might fail due to compiler optimization
//
BOOL isBigEndian()
{
    union
    {
        BYTE  c[4];
        INT32 i;
    } u;

    u.i = 0x01020304;

    return (0x01 == u.c[0]);
}

VOID initializeEndianness()
{
    if (isBigEndian()) {
        // Big-endian
        g_BigEndian = TRUE;

        getInt16 = getInt16NoSwap;
        getInt32 = getInt32NoSwap;
        getInt64 = getInt64NoSwap;

        putInt16 = putInt16NoSwap;
        putInt32 = putInt32NoSwap;
        putInt64 = putInt64NoSwap;
    }
    else {
        // Little-endian
        g_BigEndian = FALSE;

        getInt16 = getInt16Swap;
        getInt32 = getInt32Swap;
        getInt64 = getInt64Swap;

        putInt16 = putInt16Swap;
        putInt32 = putInt32Swap;
        putInt64 = putInt64Swap;
    }
}
