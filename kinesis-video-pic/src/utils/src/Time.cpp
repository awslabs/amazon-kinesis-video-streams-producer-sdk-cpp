#include "Include_i.h"

#if defined _WIN32 || defined _WIN64 || defined __CYGWIN__

//
// Stub thread library functions
//
INLINE UINT64 stubGetTime()
{
    return 0;
}

getTime globalGetTime = stubGetTime;

#else

getTime globalGetTime = defaultGetTime;

#endif
