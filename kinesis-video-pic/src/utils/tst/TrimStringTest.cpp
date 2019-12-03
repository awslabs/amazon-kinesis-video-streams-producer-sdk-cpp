#include "UtilTestFixture.h"

class TrimStringFunctionalityTest : public UtilTestBase {
};

TEST_F(TrimStringFunctionalityTest, ltrim_Negative_Positive_Variations)
{
    PCHAR pRet = (PCHAR) 100;
    EXPECT_EQ(STATUS_NULL_ARG, LTRIMSTR(NULL, 0, &pRet));
    EXPECT_EQ((PCHAR) 100, pRet);

    EXPECT_EQ(STATUS_NULL_ARG, LTRIMSTR((PCHAR) "abc", 0, NULL));
    EXPECT_EQ(STATUS_NULL_ARG, LTRIMSTR(NULL, 0, NULL));
    EXPECT_EQ(STATUS_NULL_ARG, LTRIMSTR((PCHAR) "abc", 10, NULL));
    EXPECT_EQ(STATUS_NULL_ARG, LTRIMSTR(NULL, 10, NULL));

    EXPECT_EQ(STATUS_SUCCESS, LTRIMSTR(EMPTY_STRING, 0, &pRet));
    EXPECT_EQ('\0', *pRet);
    EXPECT_EQ(STATUS_SUCCESS, LTRIMSTR(EMPTY_STRING, 1, &pRet));
    EXPECT_EQ('\0', *pRet);
    EXPECT_EQ(STATUS_SUCCESS, LTRIMSTR(EMPTY_STRING, 100, &pRet));
    EXPECT_EQ('\0', *pRet);

    EXPECT_EQ(STATUS_SUCCESS, LTRIMSTR((PCHAR) "  \t   \r  \n  \v   \f  ", 0, &pRet));
    EXPECT_EQ('\0', *pRet);

    EXPECT_EQ(STATUS_SUCCESS, LTRIMSTR((PCHAR) " ", 0, &pRet));
    EXPECT_EQ('\0', *pRet);

    EXPECT_EQ(STATUS_SUCCESS, LTRIMSTR((PCHAR) "abc", 0, &pRet));
    EXPECT_EQ('a', *pRet);

    EXPECT_EQ(STATUS_SUCCESS, LTRIMSTR((PCHAR) "  \t   \r  \n  \v   \f  abc", 0, &pRet));
    EXPECT_EQ('a', *pRet);

    EXPECT_EQ(STATUS_SUCCESS, LTRIMSTR((PCHAR) "a", 0, &pRet));
    EXPECT_EQ('a', *pRet);

    EXPECT_EQ(STATUS_SUCCESS, LTRIMSTR((PCHAR) "  \t   \r  \n  \v   \f  a", 0, &pRet));
    EXPECT_EQ('a', *pRet);

    EXPECT_EQ(STATUS_SUCCESS, LTRIMSTR((PCHAR) "abc", 100, &pRet));
    EXPECT_EQ('a', *pRet);

    EXPECT_EQ(STATUS_SUCCESS, LTRIMSTR((PCHAR) "abc       ", 100, &pRet));
    EXPECT_EQ('a', *pRet);

    EXPECT_EQ(STATUS_SUCCESS, LTRIMSTR((PCHAR) "  \t   \r  \n  \v   \f  abc", 100, &pRet));
    EXPECT_EQ('a', *pRet);

    EXPECT_EQ(STATUS_SUCCESS, LTRIMSTR((PCHAR) "a", 100, &pRet));
    EXPECT_EQ('a', *pRet);

    EXPECT_EQ(STATUS_SUCCESS, LTRIMSTR((PCHAR) "a", 1, &pRet));
    EXPECT_EQ('a', *pRet);

    EXPECT_EQ(STATUS_SUCCESS, LTRIMSTR((PCHAR) "  \t   \r  \n  \v   \f  a", 100, &pRet));
    EXPECT_EQ('a', *pRet);

    EXPECT_EQ(STATUS_SUCCESS, LTRIMSTR((PCHAR) "  \t   \r  \n  \v   \f  abc", 3, &pRet));
    EXPECT_EQ(' ', *pRet);
    EXPECT_EQ('\t', *(pRet - 1));
}

TEST_F(TrimStringFunctionalityTest, rtrim_Negative_Positive_Variations)
{
    PCHAR pRet = (PCHAR) 100;
    EXPECT_EQ(STATUS_NULL_ARG, RTRIMSTR(NULL, 0, &pRet));
    EXPECT_EQ((PCHAR) 100, pRet);

    EXPECT_EQ(STATUS_NULL_ARG, RTRIMSTR((PCHAR) "abc", 0, NULL));
    EXPECT_EQ(STATUS_NULL_ARG, RTRIMSTR(NULL, 0, NULL));
    EXPECT_EQ(STATUS_NULL_ARG, RTRIMSTR((PCHAR) "abc", 10, NULL));
    EXPECT_EQ(STATUS_NULL_ARG, RTRIMSTR(NULL, 10, NULL));

    EXPECT_EQ(STATUS_SUCCESS, RTRIMSTR(EMPTY_STRING, 0, &pRet));
    EXPECT_EQ('\0', *pRet);
    EXPECT_EQ(STATUS_SUCCESS, RTRIMSTR(EMPTY_STRING, 1, &pRet));
    EXPECT_EQ('\0', *pRet);

    EXPECT_EQ(STATUS_SUCCESS, RTRIMSTR((PCHAR) "  \t   \r  \n  \v   \f  ", 0, &pRet));
    EXPECT_EQ(' ', *pRet);

    EXPECT_EQ(STATUS_SUCCESS, RTRIMSTR((PCHAR) " ", 0, &pRet));
    EXPECT_EQ(' ', *pRet);

    EXPECT_EQ(STATUS_SUCCESS, RTRIMSTR((PCHAR) "abc", 0, &pRet));
    EXPECT_EQ('\0', *pRet);

    EXPECT_EQ(STATUS_SUCCESS, RTRIMSTR((PCHAR) "abc  \t   \r  \n  \v   \f  ", 0, &pRet));
    EXPECT_EQ(' ', *pRet);
    EXPECT_EQ('c', *(pRet - 1));

    EXPECT_EQ(STATUS_SUCCESS, RTRIMSTR((PCHAR) "a", 0, &pRet));
    EXPECT_EQ('\0', *pRet);
    EXPECT_EQ('a', *(pRet - 1));

    EXPECT_EQ(STATUS_SUCCESS, RTRIMSTR((PCHAR) "a  \t   \r  \n  \v   \f  ", 0, &pRet));
    EXPECT_EQ(' ', *pRet);
    EXPECT_EQ('a', *(pRet - 1));

    EXPECT_EQ(STATUS_SUCCESS, RTRIMSTR((PCHAR) "abc", 3, &pRet));
    EXPECT_EQ('\0', *pRet);
    EXPECT_EQ('c', *(pRet - 1));

    EXPECT_EQ(STATUS_SUCCESS, RTRIMSTR((PCHAR) "          abc", 0, &pRet));
    EXPECT_EQ('\0', *pRet);
    EXPECT_EQ('c', *(pRet - 1));

    EXPECT_EQ(STATUS_SUCCESS, RTRIMSTR((PCHAR) "a", 1, &pRet));
    EXPECT_EQ('\0', *pRet);
    EXPECT_EQ('a', *(pRet - 1));

    EXPECT_EQ(STATUS_SUCCESS, RTRIMSTR((PCHAR) "a  \t   \r  \n  \v   \f  ", (UINT32) STRLEN((PCHAR) "a  \t   \r  \n  \v   \f  "), &pRet));
    EXPECT_EQ(' ', *pRet);
    EXPECT_EQ('a', *(pRet - 1));

    EXPECT_EQ(STATUS_SUCCESS, RTRIMSTR((PCHAR) "abc  \t   \r  \n  \v   \f  ", 3, &pRet));
    EXPECT_EQ(' ', *pRet);
    EXPECT_EQ('c', *(pRet - 1));

    EXPECT_EQ(STATUS_SUCCESS, RTRIMSTR((PCHAR) "abc  \t   \r  \n  \v   \f  ", 5, &pRet));
    EXPECT_EQ(' ', *pRet);
    EXPECT_EQ('c', *(pRet - 1));
}

TEST_F(TrimStringFunctionalityTest, trimall_Negative_Positive_Variations)
{
    PCHAR pStart = (PCHAR) 100;
    PCHAR pEnd = (PCHAR) 200;
    EXPECT_EQ(STATUS_NULL_ARG, TRIMSTRALL(NULL, 0, &pStart, &pEnd));
    EXPECT_EQ((PCHAR) 100, pStart);
    EXPECT_EQ((PCHAR) 200, pEnd);

    EXPECT_EQ(STATUS_NULL_ARG, TRIMSTRALL(NULL, 0, NULL, NULL));
    EXPECT_EQ(STATUS_NULL_ARG, TRIMSTRALL((PCHAR) "abc", 0, NULL, NULL));
    EXPECT_EQ(STATUS_NULL_ARG, TRIMSTRALL(NULL, 0, NULL, &pEnd));
    EXPECT_EQ(STATUS_NULL_ARG, TRIMSTRALL((PCHAR) "abc", 10, &pStart, NULL));
    EXPECT_EQ(STATUS_NULL_ARG, TRIMSTRALL((PCHAR) "abc", 10, NULL, &pEnd));
    EXPECT_EQ(STATUS_NULL_ARG, TRIMSTRALL(NULL, 10, NULL, NULL));

    EXPECT_EQ(STATUS_SUCCESS, TRIMSTRALL(EMPTY_STRING, 0, &pStart, &pEnd));
    EXPECT_EQ('\0', *pStart);
    EXPECT_EQ('\0', *pEnd);
    EXPECT_EQ(STATUS_SUCCESS, TRIMSTRALL(EMPTY_STRING, 1, &pStart, &pEnd));
    EXPECT_EQ('\0', *pStart);
    EXPECT_EQ('\0', *pEnd);
    EXPECT_EQ(STATUS_SUCCESS, TRIMSTRALL(EMPTY_STRING, 100, &pStart, &pEnd));
    EXPECT_EQ('\0', *pStart);
    EXPECT_EQ('\0', *pEnd);

    EXPECT_EQ(STATUS_SUCCESS, TRIMSTRALL((PCHAR) "  \t   \r  \n  \v   \f  ", 0, &pStart, &pEnd));
    EXPECT_EQ('\0', *pStart);
    EXPECT_EQ('\0', *pEnd);

    EXPECT_EQ(STATUS_SUCCESS, TRIMSTRALL((PCHAR) " ", 0, &pStart, &pEnd));
    EXPECT_EQ('\0', *pStart);
    EXPECT_EQ('\0', *pEnd);

    EXPECT_EQ(STATUS_SUCCESS, TRIMSTRALL((PCHAR) "abc", 0, &pStart, &pEnd));
    EXPECT_EQ('a', *pStart);
    EXPECT_EQ('\0', *pEnd);
    EXPECT_EQ('c', *(pEnd - 1));

    EXPECT_EQ(STATUS_SUCCESS, TRIMSTRALL((PCHAR) "  \t   \r  \n  \v   \f  abc", 0, &pStart, &pEnd));
    EXPECT_EQ('a', *pStart);
    EXPECT_EQ('\0', *pEnd);
    EXPECT_EQ('c', *(pEnd - 1));

    EXPECT_EQ(STATUS_SUCCESS, TRIMSTRALL((PCHAR) "  \t   \r  \n  \v   \f  abc  \t   \r  \n  \v   \f  ", 0, &pStart, &pEnd));
    EXPECT_EQ('a', *pStart);
    EXPECT_EQ(' ', *pEnd);
    EXPECT_EQ('c', *(pEnd - 1));

    EXPECT_EQ(STATUS_SUCCESS, TRIMSTRALL((PCHAR) "a", 0, &pStart, &pEnd));
    EXPECT_EQ('a', *pStart);
    EXPECT_EQ('\0', *pEnd);
    EXPECT_EQ('a', *(pEnd - 1));

    EXPECT_EQ(STATUS_SUCCESS, TRIMSTRALL((PCHAR) "  \t   \r  \n  \v   \f  a", 0, &pStart, &pEnd));
    EXPECT_EQ('a', *pStart);
    EXPECT_EQ('\0', *pEnd);
    EXPECT_EQ('a', *(pEnd - 1));

    EXPECT_EQ(STATUS_SUCCESS, TRIMSTRALL((PCHAR) "  \t   \r  \n  \v   \f  a  \t   \r  \n  \v   \f  ", 0, &pStart, &pEnd));
    EXPECT_EQ('a', *pStart);
    EXPECT_EQ(' ', *pEnd);
    EXPECT_EQ('a', *(pEnd - 1));

    EXPECT_EQ(STATUS_SUCCESS, TRIMSTRALL((PCHAR) "abc", 3, &pStart, &pEnd));
    EXPECT_EQ('a', *pStart);
    EXPECT_EQ('\0', *pEnd);
    EXPECT_EQ('c', *(pEnd - 1));

    EXPECT_EQ(STATUS_SUCCESS, TRIMSTRALL((PCHAR) "abc       ", (UINT32) STRLEN((PCHAR) "abc       "), &pStart, &pEnd));
    EXPECT_EQ('a', *pStart);
    EXPECT_EQ(' ', *pEnd);
    EXPECT_EQ('c', *(pEnd - 1));

    EXPECT_EQ(STATUS_SUCCESS, TRIMSTRALL((PCHAR) "  \t   \r  \n  \v   \f  abc", (UINT32) STRLEN((PCHAR) "  \t   \r  \n  \v   \f  abc"), &pStart, &pEnd));
    EXPECT_EQ('a', *pStart);
    EXPECT_EQ('\0', *pEnd);
    EXPECT_EQ('c', *(pEnd - 1));

    EXPECT_EQ(STATUS_SUCCESS, TRIMSTRALL((PCHAR) "a", 1, &pStart, &pEnd));
    EXPECT_EQ('a', *pStart);
    EXPECT_EQ('\0', *pEnd);
    EXPECT_EQ('a', *(pEnd - 1));

    EXPECT_EQ(STATUS_SUCCESS, TRIMSTRALL((PCHAR) "  \t   \r  \n  \v   \f  a", (UINT32) STRLEN((PCHAR) "  \t   \r  \n  \v   \f  a"), &pStart, &pEnd));
    EXPECT_EQ('a', *pStart);
    EXPECT_EQ('\0', *pEnd);
    EXPECT_EQ('a', *(pEnd - 1));

    EXPECT_EQ(STATUS_SUCCESS, TRIMSTRALL((PCHAR) "  \t   \r  \n  \v   \f  abc     ", 3, &pStart, &pEnd));
    EXPECT_EQ(' ', *pStart);
    EXPECT_EQ('\t', *(pStart - 1));
    EXPECT_EQ(' ', *(pStart + 1));
    EXPECT_EQ(' ', *pEnd);
    EXPECT_EQ('\t', *(pEnd - 1));

    EXPECT_EQ(STATUS_SUCCESS, TRIMSTRALL((PCHAR) "  \t   \r  \n  \v   \f  abc  \t   \r  \n  \v   \f  ", (UINT32) STRLEN((PCHAR) "  \t   \r  \n  \v   \f  abc  \t   \r  \n  \v   \f  "), &pStart, &pEnd));
    EXPECT_EQ('a', *pStart);
    EXPECT_EQ(' ', *pEnd);
    EXPECT_EQ('c', *(pEnd - 1));
}
