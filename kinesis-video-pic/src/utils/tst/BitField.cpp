#include "UtilTestFixture.h"

class BitfieldFunctionalityTest : public UtilTestBase {
};

TEST_F(BitfieldFunctionalityTest, NegativeInvalidInput_BitFieldCreate)
{
    PBitField pBitField;
    EXPECT_NE(STATUS_SUCCESS, bitFieldCreate(0, &pBitField));
    EXPECT_NE(STATUS_SUCCESS, bitFieldCreate(10, NULL));
    EXPECT_NE(STATUS_SUCCESS, bitFieldCreate(0, NULL));
}

TEST_F(BitfieldFunctionalityTest, PositiveIdempotentInvalidInput_BitFieldFree)
{
    EXPECT_EQ(STATUS_SUCCESS, bitFieldFree(NULL));
}

TEST_F(BitfieldFunctionalityTest, NegativeInvalidInput_BitFieldGetCount)
{
    PBitField pBitField = (PBitField) 1;
    UINT32 count;
    EXPECT_NE(STATUS_SUCCESS, bitFieldGetCount(NULL, &count));
    EXPECT_NE(STATUS_SUCCESS, bitFieldGetCount(pBitField, NULL));
    EXPECT_NE(STATUS_SUCCESS, bitFieldGetCount(NULL, NULL));
}

TEST_F(BitfieldFunctionalityTest, NegativeInvalidInput_BitFieldReset)
{
    EXPECT_NE(STATUS_SUCCESS, bitFieldReset(NULL, TRUE));
}

TEST_F(BitfieldFunctionalityTest, NegativeInvalidInput_BitFieldSet)
{
    PBitField pBitField;
    EXPECT_EQ(STATUS_SUCCESS, bitFieldCreate(100, &pBitField));

    EXPECT_NE(STATUS_SUCCESS, bitFieldSet(NULL, 10, TRUE));
    EXPECT_NE(STATUS_SUCCESS, bitFieldSet(pBitField, 100, TRUE));
    EXPECT_NE(STATUS_SUCCESS, bitFieldSet(NULL, 100, TRUE));

    EXPECT_EQ(STATUS_SUCCESS, bitFieldFree(pBitField));
}

TEST_F(BitfieldFunctionalityTest, NegativeInvalidInput_BitFieldGet)
{
    PBitField pBitField;
    BOOL isSet;
    EXPECT_EQ(STATUS_SUCCESS, bitFieldCreate(100, &pBitField));

    EXPECT_NE(STATUS_SUCCESS, bitFieldGet(NULL, 10, &isSet));
    EXPECT_NE(STATUS_SUCCESS, bitFieldGet(pBitField, 100, &isSet));
    EXPECT_NE(STATUS_SUCCESS, bitFieldGet(pBitField, 10, NULL));
    EXPECT_NE(STATUS_SUCCESS, bitFieldGet(pBitField, 100, &isSet));
    EXPECT_NE(STATUS_SUCCESS, bitFieldGet(NULL, 100, &isSet));
    EXPECT_NE(STATUS_SUCCESS, bitFieldGet(NULL, 100, NULL));

    EXPECT_EQ(STATUS_SUCCESS, bitFieldFree(pBitField));
}

TEST_F(BitfieldFunctionalityTest, GetPutResetSingleItemBitField)
{
    PBitField pBitField;
    BOOL isSet;
    UINT32 count;

    // create the bit field
    EXPECT_EQ(STATUS_SUCCESS, bitFieldCreate(1, &pBitField));

    // Get/put/reset
    EXPECT_EQ(STATUS_SUCCESS, bitFieldGet(pBitField, 0, &isSet));
    EXPECT_FALSE(isSet);

    EXPECT_EQ(STATUS_SUCCESS, bitFieldSet(pBitField, 0, TRUE));

    EXPECT_EQ(STATUS_SUCCESS, bitFieldGet(pBitField, 0, &isSet));
    EXPECT_TRUE(isSet);

    EXPECT_EQ(STATUS_SUCCESS, bitFieldReset(pBitField, FALSE));

    EXPECT_EQ(STATUS_SUCCESS, bitFieldGet(pBitField, 0, &isSet));
    EXPECT_FALSE(isSet);

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, bitFieldGetCount(pBitField, &count));
    EXPECT_EQ(1, count);

    // Free the bit field
    EXPECT_EQ(STATUS_SUCCESS, bitFieldFree(pBitField));
}

TEST_F(BitfieldFunctionalityTest, GetPutResetSubByteItemBitField)
{
    PBitField pBitField;
    BOOL isSet;
    UINT32 count;

    // create the bit field
    EXPECT_EQ(STATUS_SUCCESS, bitFieldCreate(8, &pBitField));

    // Get/put/reset
    EXPECT_EQ(STATUS_SUCCESS, bitFieldGet(pBitField, 0, &isSet));
    EXPECT_FALSE(isSet);

    EXPECT_EQ(STATUS_SUCCESS, bitFieldSet(pBitField, 0, TRUE));

    EXPECT_EQ(STATUS_SUCCESS, bitFieldGet(pBitField, 0, &isSet));
    EXPECT_TRUE(isSet);

    EXPECT_EQ(STATUS_SUCCESS, bitFieldReset(pBitField, FALSE));

    EXPECT_EQ(STATUS_SUCCESS, bitFieldGet(pBitField, 0, &isSet));
    EXPECT_FALSE(isSet);


    EXPECT_EQ(STATUS_SUCCESS, bitFieldGet(pBitField, 7, &isSet));
    EXPECT_FALSE(isSet);

    EXPECT_EQ(STATUS_SUCCESS, bitFieldSet(pBitField, 7, TRUE));

    EXPECT_EQ(STATUS_SUCCESS, bitFieldGet(pBitField, 7, &isSet));
    EXPECT_TRUE(isSet);

    EXPECT_EQ(STATUS_SUCCESS, bitFieldReset(pBitField, FALSE));

    EXPECT_EQ(STATUS_SUCCESS, bitFieldGet(pBitField, 7, &isSet));
    EXPECT_FALSE(isSet);

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, bitFieldGetCount(pBitField, &count));
    EXPECT_EQ(8, count);

    // Free the bit field
    EXPECT_EQ(STATUS_SUCCESS, bitFieldFree(pBitField));
}

TEST_F(BitfieldFunctionalityTest, GetPutResetVariousByteCountBitField)
{
    PBitField pBitField;
    BOOL isSet;
    UINT32 count;
    UINT32 itemCount = 10000 * 8;
    UINT32 i;

    // create the bit field with exact byte multiple size
    EXPECT_EQ(STATUS_SUCCESS, bitFieldCreate(itemCount, &pBitField));

    for (i = 0; i < itemCount; i++) {
        // Get/put/reset
        EXPECT_EQ(STATUS_SUCCESS, bitFieldGet(pBitField, i, &isSet));
        EXPECT_FALSE(isSet);

        EXPECT_EQ(STATUS_SUCCESS, bitFieldSet(pBitField, i, TRUE));

        EXPECT_EQ(STATUS_SUCCESS, bitFieldGet(pBitField, i, &isSet));
        EXPECT_TRUE(isSet);

        EXPECT_EQ(STATUS_SUCCESS, bitFieldSet(pBitField, i, FALSE));

        EXPECT_EQ(STATUS_SUCCESS, bitFieldGet(pBitField, i, &isSet));
        EXPECT_FALSE(isSet);
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, bitFieldGetCount(pBitField, &count));
    EXPECT_EQ(itemCount, count);

    // Free the bit field
    EXPECT_EQ(STATUS_SUCCESS, bitFieldFree(pBitField));

    // create the bit field with not exact byte multiple size
    itemCount += 1;
    EXPECT_EQ(STATUS_SUCCESS, bitFieldCreate(itemCount, &pBitField));

    for (i = 0; i < itemCount; i++) {
        // Get/put/reset
        EXPECT_EQ(STATUS_SUCCESS, bitFieldGet(pBitField, i, &isSet));
        EXPECT_FALSE(isSet);

        EXPECT_EQ(STATUS_SUCCESS, bitFieldSet(pBitField, i, TRUE));

        EXPECT_EQ(STATUS_SUCCESS, bitFieldGet(pBitField, i, &isSet));
        EXPECT_TRUE(isSet);

        EXPECT_EQ(STATUS_SUCCESS, bitFieldSet(pBitField, i, FALSE));

        EXPECT_EQ(STATUS_SUCCESS, bitFieldGet(pBitField, i, &isSet));
        EXPECT_FALSE(isSet);
    }

    // Validate the count
    EXPECT_EQ(STATUS_SUCCESS, bitFieldGetCount(pBitField, &count));
    EXPECT_EQ(itemCount, count);

    // Free the bit field
    EXPECT_EQ(STATUS_SUCCESS, bitFieldFree(pBitField));
}
