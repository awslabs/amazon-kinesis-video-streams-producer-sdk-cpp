#include "UtilTestFixture.h"

class VersionsFunctionalityTest : public UtilTestBase {
};

TEST_F(VersionsFunctionalityTest, getOsVersion_Negative_Positive_Variations)
{
    CHAR buffer[MAX_OS_VERSION_STRING_LEN];
    EXPECT_EQ(STATUS_NULL_ARG, GET_OS_VERSION(NULL, 0));
    EXPECT_EQ(STATUS_NULL_ARG, GET_OS_VERSION(NULL, 1));
    EXPECT_EQ(STATUS_NULL_ARG, GET_OS_VERSION(NULL, SIZEOF(buffer) / SIZEOF(CHAR)));
    EXPECT_EQ(STATUS_NOT_ENOUGH_MEMORY, GET_OS_VERSION(buffer, 1));

    EXPECT_EQ(STATUS_SUCCESS, GET_OS_VERSION(buffer, SIZEOF(buffer) / SIZEOF(CHAR)));
    EXPECT_NE('\0', buffer[0]);

    DLOGI("OS Version: %s", buffer);
}

TEST_F(VersionsFunctionalityTest, getPlatformName_Negative_Positive_Variations)
{
    CHAR buffer[MAX_PLATFORM_NAME_STRING_LEN];
    EXPECT_EQ(STATUS_NULL_ARG, GET_PLATFORM_NAME(NULL, 0));
    EXPECT_EQ(STATUS_NULL_ARG, GET_PLATFORM_NAME(NULL, 1));
    EXPECT_EQ(STATUS_NULL_ARG, GET_PLATFORM_NAME(NULL, SIZEOF(buffer) / SIZEOF(CHAR)));
    EXPECT_EQ(STATUS_NOT_ENOUGH_MEMORY, GET_PLATFORM_NAME(buffer, 1));

    EXPECT_EQ(STATUS_SUCCESS, GET_PLATFORM_NAME(buffer, SIZEOF(buffer) / SIZEOF(CHAR)));
    EXPECT_NE('\0', buffer[0]);

    DLOGI("Platform name: %s", buffer);
}

TEST_F(VersionsFunctionalityTest, getCompilerInfo_Negative_Positive_Variations)
{
    CHAR buffer[MAX_COMPILER_INFO_STRING_LEN];
    EXPECT_EQ(STATUS_NULL_ARG, GET_COMPILER_INFO(NULL, 0));
    EXPECT_EQ(STATUS_NULL_ARG, GET_COMPILER_INFO(NULL, 1));
    EXPECT_EQ(STATUS_NULL_ARG, GET_COMPILER_INFO(NULL, SIZEOF(buffer) / SIZEOF(CHAR)));
    EXPECT_EQ(STATUS_NOT_ENOUGH_MEMORY, GET_COMPILER_INFO(buffer, 1));

    EXPECT_EQ(STATUS_SUCCESS, GET_COMPILER_INFO(buffer, SIZEOF(buffer) / SIZEOF(CHAR)));
    EXPECT_NE('\0', buffer[0]);

    DLOGI("Compiler info: %s", buffer);
}
