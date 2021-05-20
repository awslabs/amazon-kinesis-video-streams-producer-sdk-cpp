#include "UtilTestFixture.h"

class StringSearchFunctionalityTest : public UtilTestBase {
};

TEST_F(StringSearchFunctionalityTest, NegativeInvalidInput) {
    CHAR str[] = "this is a test";
    EXPECT_EQ(NULL, defaultStrnstr(NULL, NULL, 0));
    EXPECT_EQ(str, defaultStrnstr(str, NULL, 0));
    EXPECT_EQ(NULL, defaultStrnstr(NULL, str, 0));
    EXPECT_EQ(NULL, defaultStrnstr(str, str, 1));
    EXPECT_EQ(str, defaultStrnstr(str, (PCHAR) "", ARRAY_SIZE(str) - 1));
    EXPECT_EQ(NULL, defaultStrnstr((PCHAR) "", str, 1));
    EXPECT_EQ(NULL, defaultStrnstr(str, (PCHAR) "this is a test with extra length", ARRAY_SIZE(str) - 1));
}

TEST_F(StringSearchFunctionalityTest, ExactMatch_WithoutLen) {
    CHAR str[] = "this is a test";
    EXPECT_EQ(str, defaultStrnstr(str, str, ARRAY_SIZE(str) - 1));
}

TEST_F(StringSearchFunctionalityTest, ExactMatch_WithLen) {
    CHAR str1[] = "this is a test";
    CHAR str2[] = "this is";
    EXPECT_EQ(str1, defaultStrnstr(str1, str2, ARRAY_SIZE(str2) - 1));
}

auto randStr = [](SIZE_T n) -> std::string {
    std::string s;

    // not ideal, but it's important to always randomize rand
    SRAND(GETTIME());
    for (int i = 0; i < n; i++) {
        // 8 bits contain 255 characters + 1 null character
        s += (CHAR)(RAND() % 255) + 1;
    }

    return s;
};

TEST_F(StringSearchFunctionalityTest, BigString_BigHaystack) {
    std::string str1;
    std::string str2;

    str1 += randStr(10000);
    str2 = "this is the needle";
    str1 += str2;
    str1 += randStr(10000);

    EXPECT_EQ(STRSTR(str1.c_str(), str2.c_str()), defaultStrnstr((PCHAR) str1.c_str(), (PCHAR) str2.c_str(), str1.size()));
}

TEST_F(StringSearchFunctionalityTest, BigString_BigNeedle) {
    std::string str1;
    std::string str2;

    str1 += randStr(10000);
    str2 = randStr(10000);
    str1 += str2;
    str1 += randStr(10000);

    EXPECT_EQ(STRSTR(str1.c_str(), str2.c_str()), defaultStrnstr((PCHAR) str1.c_str(), (PCHAR) str2.c_str(), str1.size()));
}
