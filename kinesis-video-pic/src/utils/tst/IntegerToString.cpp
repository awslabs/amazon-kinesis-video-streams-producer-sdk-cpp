#include "UtilTestFixture.h"

class ItoEFunctionalityTest : public UtilTestBase {
};

TEST_F(ItoEFunctionalityTest, InvalidInput_NullStrInput) {
    UINT32 size;
    EXPECT_NE(STATUS_SUCCESS, ULLTOSTR(1, NULL, 100, 10, &size));
    EXPECT_NE(STATUS_SUCCESS, ULTOSTR(1, NULL, 100, 10, &size));
}

TEST_F(ItoEFunctionalityTest, InvalidInput_ZeroSizeInput) {
    UINT32 size;
    PCHAR pStr = (PCHAR) 1;
    EXPECT_NE(STATUS_SUCCESS, ULLTOSTR(1, pStr, 0, 10, &size));
    EXPECT_NE(STATUS_SUCCESS, ULTOSTR(1, pStr, 0, 10, &size));

    EXPECT_NE(STATUS_SUCCESS, ULLTOSTR(1, pStr, 1, 10, &size));
    EXPECT_NE(STATUS_SUCCESS, ULTOSTR(1, pStr, 1, 10, &size));
}


TEST_F(ItoEFunctionalityTest, InvalidInput_InvalidBase) {
    UINT32 size;
    PCHAR pStr = (PCHAR) 1;
    EXPECT_NE(STATUS_SUCCESS, ULLTOSTR(1, pStr, 100, 0, &size));
    EXPECT_NE(STATUS_SUCCESS, ULTOSTR(1, pStr, 100, 0, &size));

    EXPECT_NE(STATUS_SUCCESS, ULLTOSTR(1, pStr, 100, 1, &size));
    EXPECT_NE(STATUS_SUCCESS, ULTOSTR(1, pStr, 100, 1, &size));

    EXPECT_NE(STATUS_SUCCESS, ULLTOSTR(1, pStr, 100, 37, &size));
    EXPECT_NE(STATUS_SUCCESS, ULTOSTR(1, pStr, 100, 37, &size));
}

TEST_F(ItoEFunctionalityTest, InvalidInput_BufferTooSmall) {
    UINT32 size;
    CHAR temp[256];
    EXPECT_NE(STATUS_SUCCESS, ULLTOSTR(100, temp, 3, 10, &size));
    EXPECT_NE(STATUS_SUCCESS, ULTOSTR(100, temp, 3, 10, &size));
}

TEST_F(ItoEFunctionalityTest, ValidInput_SizeNotSpecified) {
    UINT32 size;
    CHAR temp[256];

    temp[0] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, ULLTOSTR(100, temp, 4, 10, &size));
    EXPECT_TRUE(STRCMP(temp, "100") == 0);
    EXPECT_EQ(3, size);

    temp[0] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, ULTOSTR(100, temp, 4, 10, &size));
    EXPECT_TRUE(STRCMP(temp, "100") == 0);
    EXPECT_EQ(3, size);

    temp[0] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, ULLTOSTR(100, temp, 4, 10, NULL));
    EXPECT_TRUE(STRCMP(temp, "100") == 0);

    temp[0] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, ULTOSTR(100, temp, 4, 10, NULL));
    EXPECT_TRUE(STRCMP(temp, "100") == 0);
}

TEST_F(ItoEFunctionalityTest, ValidInput_MinMaxValues) {
    CHAR temp[256];

    temp[0] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, ULLTOSTR(0xffffffffffffffff, temp, 256, 16, NULL));
    EXPECT_TRUE(STRCMP(temp, "ffffffffffffffff") == 0);

    temp[0] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, ULTOSTR(0xffffffff, temp, 256, 16, NULL));
    EXPECT_TRUE(STRCMP(temp, "ffffffff") == 0);

    temp[0] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, ULLTOSTR(0, temp, 2, 10, NULL));
    EXPECT_TRUE(STRCMP(temp, "0") == 0);

    temp[0] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, ULTOSTR(0, temp, 2, 10, NULL));
    EXPECT_TRUE(STRCMP(temp, "0") == 0);
}

TEST_F(ItoEFunctionalityTest, ValidInput_VariousBases) {
    CHAR temp[256];
    UINT32 value = 255;

    temp[0] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, ULLTOSTR(value, temp, 256, 2, NULL));
    EXPECT_TRUE(STRCMP(temp, "11111111") == 0);

    temp[0] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, ULTOSTR(value, temp, 256, 2, NULL));
    EXPECT_TRUE(STRCMP(temp, "11111111") == 0);

    temp[0] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, ULLTOSTR(value, temp, 256, 8, NULL));
    EXPECT_TRUE(STRCMP(temp, "377") == 0);

    temp[0] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, ULTOSTR(value, temp, 256, 8, NULL));
    EXPECT_TRUE(STRCMP(temp, "377") == 0);

    temp[0] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, ULLTOSTR(value, temp, 256, 10, NULL));
    EXPECT_TRUE(STRCMP(temp, "255") == 0);

    temp[0] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, ULTOSTR(value, temp, 256, 10, NULL));
    EXPECT_TRUE(STRCMP(temp, "255") == 0);

    temp[0] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, ULLTOSTR(value, temp, 256, 16, NULL));
    EXPECT_TRUE(STRCMP(temp, "ff") == 0);

    temp[0] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, ULTOSTR(value, temp, 256, 16, NULL));
    EXPECT_TRUE(STRCMP(temp, "ff") == 0);

    temp[0] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, ULLTOSTR(value, temp, 256, 36, NULL));
    EXPECT_TRUE(STRCMP(temp, "73") == 0);

    temp[0] = '\0';
    EXPECT_EQ(STATUS_SUCCESS, ULTOSTR(value, temp, 256, 36, NULL));
    EXPECT_TRUE(STRCMP(temp, "73") == 0);
}
