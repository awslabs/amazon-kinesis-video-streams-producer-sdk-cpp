#include "Include_i.h"

#if defined _WIN32 || defined _WIN64 || defined __CYGWIN__

//
// Stub dynamic library loading functions
//
INLINE PVOID stubDlOpen(PCHAR filename, UINT32 flag)
{
    UNUSED_PARAM(filename);
    UNUSED_PARAM(flag);
    return NULL;
}

INLINE INT32 stubDlClose(PVOID handle)
{
    UNUSED_PARAM(handle);
    return 0;
}

INLINE PVOID stubDlSym(PVOID handle, PCHAR symbol)
{
    UNUSED_PARAM(handle);
    UNUSED_PARAM(symbol);
    return NULL;
}

INLINE PCHAR stubDlError()
{
    return (PCHAR) NULL;
}

dlOpen globalDlOpen = stubDlOpen;
dlClose globalDlClose = stubDlClose;
dlSym globalDlSym = stubDlSym;
dlError globalDlError = stubDlError;

#else

dlOpen globalDlOpen = defaultDlOpen;
dlClose globalDlClose = defaultDlClose;
dlSym globalDlSym = defaultDlSym;
dlError globalDlError = defaultDlError;

#endif
