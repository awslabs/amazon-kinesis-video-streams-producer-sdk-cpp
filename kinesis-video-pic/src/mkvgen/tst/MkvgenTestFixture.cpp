#include "MkvgenTestFixture.h"

UINT64 getTimeCallback(UINT64 customData)
{
    EXPECT_EQ(MKV_TEST_CUSTOM_DATA, customData);
    gTimeCallbackCalled = TRUE;
    return GETTIME();
}