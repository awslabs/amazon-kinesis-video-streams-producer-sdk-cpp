#include "UtilTestFixture.h"

class FindStringFunctionalityTest : public UtilTestBase {
};

TEST_F(FindStringFunctionalityTest, Negative_Positive_Variations)
{
    PCHAR pRet;
    CHAR notNullTerminatedStr[] = {'a', 'b', 'c'};

    EXPECT_EQ(NULL, STRNCHR(NULL, 0, 'a'));
    EXPECT_EQ(NULL, STRNCHR(NULL, 10, 'a'));
    EXPECT_EQ(NULL, STRNCHR((PCHAR) "abc", 0, 'a'));
    EXPECT_EQ(NULL, STRNCHR(EMPTY_STRING, 0, 'a'));
    EXPECT_EQ(NULL, STRNCHR(EMPTY_STRING, 1, 'a'));
    EXPECT_EQ(NULL, STRNCHR(EMPTY_STRING, 10, 'a'));
    EXPECT_EQ(NULL, STRNCHR((PCHAR) "abc", 0, 'a'));

    pRet = STRNCHR((PCHAR) "abc", (UINT32) STRLEN((PCHAR) "abc"), 'a');
    EXPECT_EQ('a', *pRet);

    pRet = STRNCHR((PCHAR) "abc", 100, 'a');
    EXPECT_EQ('a', *pRet);

    EXPECT_EQ(NULL, STRNCHR((PCHAR) "abc", (UINT32) STRLEN((PCHAR) "abc"), 'd'));
    EXPECT_EQ(NULL, STRNCHR((PCHAR) "abc", 100, 'd'));
    EXPECT_EQ(NULL, STRNCHR((PCHAR) notNullTerminatedStr, 3, 'd'));

    pRet = STRNCHR((PCHAR) "abc", 1, 'a');
    EXPECT_EQ('a', *pRet);

    pRet = STRNCHR((PCHAR) "a", 1, 'a');
    EXPECT_EQ('a', *pRet);

    pRet = STRNCHR((PCHAR) "abc", 10, 'a');
    EXPECT_EQ('a', *pRet);

    pRet = STRNCHR((PCHAR) "abc", 10, 'c');
    EXPECT_EQ('c', *pRet);
}
