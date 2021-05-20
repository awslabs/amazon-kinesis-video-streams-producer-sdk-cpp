#include "gtest/gtest.h"
#include "src/heap/src/Include_i.h"

#define NUM_ITERATIONS  100
#define MOCK_LIB_HANDLE 0xfefefefe

class HeapTestBase : public ::testing::Test {
  public:
    HeapTestBase()
    {
        SetUpInternal();
    };

  protected:
    static VOID SetUpInternal();

    virtual VOID SetUp();

    static PVOID mockDlOpen(PCHAR filename, UINT32 flag);
    static INT32 mockDlClose(PVOID handle);
    static PVOID mockDlSym(PVOID handle, PCHAR symbol);
    static PCHAR mockDlError();

    static PVOID mDlOpen;
    static INT32 mDlClose;

    static BOOL mDlSym;

    static UINT32 mDlOpenCount;
    static UINT32 mDlErrorCount;
    static UINT32 mDlSymCount;
    static UINT32 mDlCloseCount;

    static UINT32 mVramGetMax;
    static UINT32 mVramInit;
    static UINT32 mVramAlloc;
    static UINT32 mVramFree;
    static PVOID mVramLock;
    static UINT32 mVramUnlock;
    static UINT32 mVramUninit;

    static UINT32 mVramGetMaxCount;
    static UINT32 mVramInitCount;
    static UINT32 mVramAllocCount;
    static UINT32 mVramFreeCount;
    static UINT32 mVramLockCount;
    static UINT32 mVramUnlockCount;
    static UINT32 mVramUninitCount;

    /**
     * Declaring the mock hybrid heap thunk functions
     */
    static UINT32 mockVramInit();
    static UINT32 mockVramAlloc(UINT32);
    static UINT32 mockVramFree(UINT32);
    static PVOID mockVramLock(UINT32);
    static UINT32 mockVramUnlock(UINT32);
    static UINT32 mockVramUninit();
    static UINT32 mockVramGetMax();

    /**
     * Static scratch buffer
     */
    static UINT8 mScratchBuf[500000];
};
