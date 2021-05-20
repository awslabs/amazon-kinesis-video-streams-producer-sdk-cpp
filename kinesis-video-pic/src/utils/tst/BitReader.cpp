#include "UtilTestFixture.h"

class BitReaderFunctionalityTest : public UtilTestBase {
};

TEST_F(BitReaderFunctionalityTest, NegativeInvalidInput_BitReaderReset)
{
    BitReader bitReader;
    BYTE tempBuffer[1000];
    UINT32 size = SIZEOF(tempBuffer);
    EXPECT_NE(STATUS_SUCCESS, bitReaderReset(NULL, tempBuffer, size));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReset(&bitReader, NULL, size));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReset(NULL, NULL, size));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 0));
}

TEST_F(BitReaderFunctionalityTest, Variations_BitReaderSet)
{
    BitReader bitReader;
    BYTE tempBuffer[1000];
    UINT32 size = SIZEOF(tempBuffer);

    // Reset the buffer
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, size));

    // Set permutations
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
    EXPECT_NE(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, size));
    EXPECT_NE(STATUS_SUCCESS, bitReaderSetCurrent(NULL, size));
    EXPECT_NE(STATUS_SUCCESS, bitReaderSetCurrent(NULL, 0));

    // Reset to 0 len
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 0));
    EXPECT_NE(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
}

TEST_F(BitReaderFunctionalityTest, BitReaderReadBit)
{
    BitReader bitReader;
    BYTE tempBuffer[1000];
    UINT32 size = SIZEOF(tempBuffer);
    UINT32 readVal;

    tempBuffer[0] = 0x80;

    // Reset the buffer
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, size));

    // Negative permutations
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadBit(NULL, NULL));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadBit(&bitReader, NULL));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadBit(NULL, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadBit(&bitReader, &readVal));

    // Reset to 0 len
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 0));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadBit(&bitReader, &readVal));

    // Reset to 1 bit
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 1));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadBit(&bitReader, &readVal));
    EXPECT_EQ(1, readVal);
    // Fails on the next one
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadBit(&bitReader, &readVal));

    // Set to 0 - re-read
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadBit(&bitReader, &readVal));
    EXPECT_EQ(1, readVal);

    // Reset to 2 bit
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 2));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadBit(&bitReader, &readVal));
    EXPECT_EQ(1, readVal);
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadBit(&bitReader, &readVal));
    EXPECT_EQ(0, readVal);
    // Fails on the next one
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadBit(&bitReader, &readVal));
}

TEST_F(BitReaderFunctionalityTest, BitReaderReadBits)
{
    BitReader bitReader;
    BYTE tempBuffer[1000];
    UINT32 size = SIZEOF(tempBuffer);
    UINT32 readVal;

    tempBuffer[0] = 0xAA;
    tempBuffer[1] = 0xAA;

    // Reset the buffer
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, size));

    // Negative permutations
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadBits(NULL, 0, NULL));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadBits(&bitReader, 0, NULL));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadBits(NULL, 0, &readVal));
    EXPECT_EQ(0, readVal);
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadBits(NULL, 2, &readVal));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadBits(&bitReader, 33, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadBits(&bitReader, 1, &readVal));

    // Size variations
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 1));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadBits(&bitReader, 2, &readVal));

    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 31));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadBits(&bitReader, 32, &readVal));

    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 0));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadBits(&bitReader, 1, &readVal));

    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 10));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadBits(&bitReader, 1, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadBits(&bitReader, 2, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadBits(&bitReader, 3, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadBits(&bitReader, 4, &readVal));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadBits(&bitReader, 1, &readVal));

    // Read verify
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 32));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadBits(&bitReader, 2, &readVal));
    EXPECT_EQ(2, readVal);
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadBits(&bitReader, 1, &readVal));
    EXPECT_EQ(1, readVal);
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadBits(&bitReader, 1, &readVal));
    EXPECT_EQ(0, readVal);

    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 3));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadBits(&bitReader, 8, &readVal));
    EXPECT_EQ(0x55, readVal);

    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 31));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadBits(&bitReader, 8, &readVal));

    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 25));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadBits(&bitReader, 8, &readVal));

    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 3));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 2));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadBits(&bitReader, 2, &readVal));
}

TEST_F(BitReaderFunctionalityTest, BitReaderReadExpGolomb)
{
    BitReader bitReader;
    BYTE tempBuffer[1000];
    UINT32 size = SIZEOF(tempBuffer);
    UINT32 readVal;
    INT32 readValSign;

    // Reset the buffer
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, size));

    // Negative permutations
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(NULL, NULL));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, NULL));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(NULL, &readVal));

    // Negative permutations - signed
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolombSe(NULL, NULL));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolombSe(&bitReader, NULL));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolombSe(NULL, &readValSign));

    // Valid cases - 1
    tempBuffer[0] = 0x80;
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 1));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(0, readVal);
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolombSe(&bitReader, &readValSign));
    EXPECT_EQ(0, readValSign);

    // Valid cases - 010
    tempBuffer[0] = 0x40;
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 3));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(1, readVal);
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolombSe(&bitReader, &readValSign));
    EXPECT_EQ(1, readValSign);

    // Invalid cases - 01
    tempBuffer[0] = 0x40;
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 2));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolombSe(&bitReader, &readValSign));

    // Valid cases - 011
    tempBuffer[0] = 0x60;
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 3));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(2, readVal);
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolombSe(&bitReader, &readValSign));
    EXPECT_EQ(-1, readValSign);

    // Valid cases - 00100
    tempBuffer[0] = 0x20;
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 5));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(3, readVal);
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolombSe(&bitReader, &readValSign));
    EXPECT_EQ(2, readValSign);

    // Invalid cases - 0010
    tempBuffer[0] = 0x20;
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 4));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolombSe(&bitReader, &readValSign));

    // Valid cases - 00101
    tempBuffer[0] = 0x28;
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 5));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(4, readVal);
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolombSe(&bitReader, &readValSign));
    EXPECT_EQ(-2, readValSign);

    // Valid cases - 00110
    tempBuffer[0] = 0x30;
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 5));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(5, readVal);
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolombSe(&bitReader, &readValSign));
    EXPECT_EQ(3, readValSign);

    // Valid cases - 00111
    tempBuffer[0] = 0x38;
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 5));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(6, readVal);
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolombSe(&bitReader, &readValSign));
    EXPECT_EQ(-3, readValSign);

    // Valid cases - 0001000
    tempBuffer[0] = 0x10;
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 7));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(7, readVal);
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolombSe(&bitReader, &readValSign));
    EXPECT_EQ(4, readValSign);

    // Invalid cases - 000100
    tempBuffer[0] = 0x10;
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 6));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolombSe(&bitReader, &readValSign));

    // Valid cases - 0001001
    tempBuffer[0] = 0x12;
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 7));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(8, readVal);
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolombSe(&bitReader, &readValSign));
    EXPECT_EQ(-4, readValSign);

    // Valid cases - 0001010
    tempBuffer[0] = 0x14;
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 7));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(9, readVal);
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolombSe(&bitReader, &readValSign));
    EXPECT_EQ(5, readValSign);

    // Valid cases - 0001011
    tempBuffer[0] = 0x16;
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 7));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(10, readVal);
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolombSe(&bitReader, &readValSign));
    EXPECT_EQ(-5, readValSign);

    // Valid cases - 0001100
    tempBuffer[0] = 0x18;
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 7));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(11, readVal);
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolombSe(&bitReader, &readValSign));
    EXPECT_EQ(6, readValSign);

    // Valid case with more than 32 0s
    MEMSET(tempBuffer, 0x00, SIZEOF(tempBuffer));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 64));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(0xFFFFFFFF, readVal);
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReadExpGolombSe(&bitReader, &readValSign));
    EXPECT_EQ(0, readValSign);

    // Invalid - overflow with 32 0s
    EXPECT_EQ(STATUS_SUCCESS, bitReaderReset(&bitReader, tempBuffer, 63));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolomb(&bitReader, &readVal));
    EXPECT_EQ(STATUS_SUCCESS, bitReaderSetCurrent(&bitReader, 0));
    EXPECT_NE(STATUS_SUCCESS, bitReaderReadExpGolombSe(&bitReader, &readValSign));
}
