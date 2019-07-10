#include "Include_i.h"

#if defined _WIN32 || defined _WIN64 || defined __CYGWIN__

//
// Stub dynamic library loading functions
//
PVOID stubDlOpen(PCHAR filename, UINT32 flag)
{
    UNUSED_PARAM(filename);
    UNUSED_PARAM(flag);
    return NULL;
}

INT32 stubDlClose(PVOID handle)
{
    UNUSED_PARAM(handle);
    return 0;
}

PVOID stubDlSym(PVOID handle, PCHAR symbol)
{
    UNUSED_PARAM(handle);
    UNUSED_PARAM(symbol);
    return NULL;
}

PCHAR stubDlError()
{
    return (PCHAR) NULL;
}

dlOpen globalDlOpen = stubDlOpen;
dlClose globalDlClose = stubDlClose;
dlSym globalDlSym = stubDlSym;
dlError globalDlError = stubDlError;

#else

//
// Default dynamic library loading functions
//
PVOID defaultDlOpen(PCHAR filename, UINT32 flag)
{
    return dlopen((const PCHAR) filename, flag);
}

INT32 defaultDlClose(PVOID handle)
{
    return dlclose(handle);
}

PVOID defaultDlSym(PVOID handle, PCHAR symbol)
{
    return dlsym(handle, symbol);
}

PCHAR defaultDlError()
{
    return (PCHAR) dlerror();
}

dlOpen globalDlOpen = defaultDlOpen;
dlClose globalDlClose = defaultDlClose;
dlSym globalDlSym = defaultDlSym;
dlError globalDlError = defaultDlError;

#endif
