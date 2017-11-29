#include <gtest/gtest.h>
#include "src/duration/src/Duration.h"

TEST(InvalidInput, NullInput) {
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(NULL, NULL));
}

TEST(InvalidInput, EmptyInput) {
    PCHAR emptyStr = (PCHAR) "";
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(emptyStr, NULL));
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(emptyStr, emptyStr));
}

TEST(InvalidInput, NonPeriodInput) {
    PCHAR nonPeriodStr = (PCHAR) "23DT23H";
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(nonPeriodStr, NULL));
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(nonPeriodStr, nonPeriodStr + STRLEN(nonPeriodStr)));
}

TEST(InvalidInput, TwoTimeInput) {
    PCHAR twoTimeStr = (PCHAR) "P23DT23HT23H";
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(twoTimeStr, NULL));
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(twoTimeStr, twoTimeStr + STRLEN(twoTimeStr)));
}

TEST(InvalidInput, InvalidCharInput) {
    PCHAR invalidCharStr = (PCHAR) "P23DT23HX";
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(invalidCharStr, NULL));
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(invalidCharStr, invalidCharStr + STRLEN(invalidCharStr)));
}

TEST(InvalidInput, TwoYearInput) {
    PCHAR twoYearStr = (PCHAR) "P1Y23DT23H1Y";
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(twoYearStr, NULL));
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(twoYearStr, twoYearStr + STRLEN(twoYearStr)));
}

TEST(InvalidInput, TwoDayInput) {
    PCHAR twoDayStr = (PCHAR) "P1Y23DT23H2D";
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(twoDayStr, NULL));
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(twoDayStr, twoDayStr + STRLEN(twoDayStr)));
}

TEST(InvalidInput, InvalidFractionInput1) {
    PCHAR invalidFractionStr = (PCHAR) "P1Y23.5.5DT23H";
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(invalidFractionStr, NULL));
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(invalidFractionStr, invalidFractionStr + STRLEN(invalidFractionStr)));
}

TEST(InvalidInput, InvalidFractionInput2) {
    PCHAR invalidFractionStr = (PCHAR) "P1Y34.SDT23H";
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(invalidFractionStr, NULL));
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(invalidFractionStr, invalidFractionStr + STRLEN(invalidFractionStr)));
}

TEST(InvalidInput, EarlyEndInput) {
    PCHAR earlyEndStr = (PCHAR) "P1Y23.5DT23";
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(earlyEndStr, NULL));
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(earlyEndStr, earlyEndStr + STRLEN(earlyEndStr)));
}

TEST(InvalidInput, OverflowInput) {
    PCHAR overflowStr = (PCHAR) "P355000Y";
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(overflowStr, NULL));
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(overflowStr, overflowStr + STRLEN(overflowStr)));
}

TEST(InvalidInput, SmallCaseInput) {
    PCHAR smallCaseInput = (PCHAR) "pT23.5H3M";
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(smallCaseInput, NULL));
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(smallCaseInput, smallCaseInput + STRLEN(smallCaseInput)));
    smallCaseInput = (PCHAR) "Pt23.5H3M";
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(smallCaseInput, NULL));
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(smallCaseInput, smallCaseInput + STRLEN(smallCaseInput)));
    smallCaseInput = (PCHAR) "PT23.5h3M";
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(smallCaseInput, NULL));
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(smallCaseInput, smallCaseInput + STRLEN(smallCaseInput)));
}

TEST(InvalidInput, InputWithInvalidChar) {
    PCHAR inputWithSpace = (PCHAR) "PT23.5 H3M";
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(inputWithSpace, NULL));
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(inputWithSpace, inputWithSpace + STRLEN(inputWithSpace)));
    inputWithSpace = (PCHAR) "PT23.5;H3M";
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(inputWithSpace, NULL));
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(inputWithSpace, inputWithSpace + STRLEN(inputWithSpace)));
    inputWithSpace = (PCHAR) "PT23.5Hq3M";
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(inputWithSpace, NULL));
    EXPECT_EQ(INVALID_DURATION_VALUE, parseDuration(inputWithSpace, inputWithSpace + STRLEN(inputWithSpace)));
}

TEST(ValidInput, CompleteInput) {
    PCHAR completeStr = (PCHAR) "P1.2Y2.6M3.5W23.5DT23.2H12.5M6.7S";
    unsigned long long expectedValue = (unsigned long long)(((1.2 * 365 + 2.6 * 31 + 3.5 * 7 + 23.5 ) * 24 * 3600
	+ 23.2 * 3600 + 12.5 * 60 + 6.7) * 10000000);
    EXPECT_EQ(expectedValue, parseDuration(completeStr, NULL));
    EXPECT_EQ(expectedValue, parseDuration(completeStr, completeStr + STRLEN(completeStr)));
}

TEST(ValidInput, InputWithZeros) {
    PCHAR withZerosStr = (PCHAR) "P1Y0M0.0W23DT23H";
    unsigned long long expectedValue = (unsigned long long)(((1.0 * 365 + 23) * 24 * 3600 + 23 * 3600) * 10000000);
    EXPECT_EQ(expectedValue, parseDuration(withZerosStr, NULL));
    EXPECT_EQ(expectedValue, parseDuration(withZerosStr, withZerosStr + STRLEN(withZerosStr)));
}

TEST(ValidInput, InputWithoutTime) {
    PCHAR withoutTimeStr = (PCHAR) "P1Y23D";
    unsigned long long expectedValue = (unsigned long long)(((1.0 * 365 + 23) * 24 * 3600) * 10000000);
    EXPECT_EQ(expectedValue, parseDuration(withoutTimeStr, NULL));
    EXPECT_EQ(expectedValue, parseDuration(withoutTimeStr, withoutTimeStr + STRLEN(withoutTimeStr)));
}

TEST(ValidInput, TimeOnlyInput) {
    PCHAR timeOnlyStr = (PCHAR) "PT23.5H3M";
    unsigned long long expectedValue = (unsigned long long)((23.5 * 3600 + 3 * 60) * 10000000);
    EXPECT_EQ(expectedValue, parseDuration(timeOnlyStr, NULL));
    EXPECT_EQ(expectedValue, parseDuration(timeOnlyStr, timeOnlyStr + STRLEN(timeOnlyStr)));
}

TEST(ValidInput, LargeIntegerInput) {
    PCHAR largeIntegerStr = (PCHAR) "P35500Y";
    unsigned long long expectedValue = (unsigned long long) 35500 * 365 * 24 * 3600 * 10000000;
    EXPECT_EQ(expectedValue, parseDuration(largeIntegerStr, NULL));
    EXPECT_EQ(expectedValue, parseDuration(largeIntegerStr, largeIntegerStr + STRLEN(largeIntegerStr)));
}

TEST(ValidInput, PreciseFractionalInput) {
    PCHAR preciseFractionStr = (PCHAR) "P1.2Y2.6M3.5W23.5DT23.2311H12.567122M6.712343S";
    unsigned long long expectedValue = (unsigned long long)(((1.2 * 365 + 2.6 * 31 + 3.5 * 7 + 23.5 ) * 24 * 3600
                + 23.2311 * 3600 + 12.567122 * 60 + 6.712343) * 10000000);
    EXPECT_EQ(expectedValue, parseDuration(preciseFractionStr, NULL));
    EXPECT_EQ(expectedValue, parseDuration(preciseFractionStr, preciseFractionStr + STRLEN(preciseFractionStr)));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
