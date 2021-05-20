#include "UtilTestFixture.h"

class Crc32Test : public UtilTestBase {
};

TEST_F(Crc32Test, crc32FunctionalityTest)
{
    PCHAR testString = (PCHAR) "testString";
    PCHAR testStringNum = (PCHAR) "12345";
    PCHAR testStringSingleChar = (PCHAR) "a";
    PCHAR testStringLong = (PCHAR) "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

    EXPECT_EQ((UINT32) 0, COMPUTE_CRC32((PBYTE) "", 0));
    EXPECT_EQ((UINT32) 0, COMPUTE_CRC32(NULL, 0));
    EXPECT_EQ((UINT32) 0, COMPUTE_CRC32(NULL, 2));
    EXPECT_EQ((UINT32) 0xcbf53a1c, COMPUTE_CRC32((PBYTE) testStringNum, (UINT32) STRLEN(testStringNum)));
    EXPECT_EQ((UINT32) 0x18f4fd08, COMPUTE_CRC32((PBYTE) testString, (UINT32) STRLEN(testString)));
    EXPECT_EQ((UINT32) 0xe8b7be43, COMPUTE_CRC32((PBYTE) testStringSingleChar, (UINT32) STRLEN(testStringSingleChar)));
    EXPECT_EQ((UINT32) 0x46619d26, COMPUTE_CRC32((PBYTE) testStringLong, (UINT32) STRLEN(testStringLong)));
}
