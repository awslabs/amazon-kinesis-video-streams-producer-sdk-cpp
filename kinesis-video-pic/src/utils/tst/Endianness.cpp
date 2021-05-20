#include "UtilTestFixture.h"

class EndiannessFunctionalityTest : public UtilTestBase {
};

TEST_F(EndiannessFunctionalityTest, InitializeEndianness)
{
    // Call multiple times
    initializeEndianness();
    initializeEndianness();
    initializeEndianness();
    initializeEndianness();
    initializeEndianness();

    // Validate endianness
    BOOL bigEndian = isBigEndian();

#if defined(__BIG_ENDIAN__)
    EXPECT_TRUE(bigEndian);
#elif defined(__LITTLE_ENDIAN__)
    EXPECT_FALSE(bigEndian);
#endif
}

TEST_F(EndiannessFunctionalityTest, UnalignedGet)
{
    BYTE data[9] = {0, 1, 2, 3, 4, 5, 6, 7, 8};

    // Initialize the endianness first
    initializeEndianness();

    UINT8 data8 = GET_UNALIGNED((PUINT8) data);
    UINT16 data16 = GET_UNALIGNED((PUINT16) data);
    UINT32 data32 = GET_UNALIGNED((PUINT32) data);
    UINT64 data64 = GET_UNALIGNED((PUINT64) data);

    UINT8 unaligned8 = GET_UNALIGNED((PUINT8) &data[1]);
    UINT16 unaligned16 = GET_UNALIGNED((PUINT16) &data[1]);
    UINT32 unaligned32 = GET_UNALIGNED((PUINT32) &data[1]);
    UINT64 unaligned64 = GET_UNALIGNED((PUINT64) &data[1]);

    if (isBigEndian()) {
        EXPECT_EQ(0x00, data8);
        EXPECT_EQ(0x0001, data16);
        EXPECT_EQ(0x00010203, data32);
        EXPECT_EQ(0x0001020304050607, data64);

        EXPECT_EQ(0x01, unaligned8);
        EXPECT_EQ(0x0102, unaligned16);
        EXPECT_EQ(0x01020304, unaligned32);
        EXPECT_EQ(0x0102030405060708, unaligned64);
    } else {
        EXPECT_EQ(0x00, data8);
        EXPECT_EQ(0x0100, data16);
        EXPECT_EQ(0x03020100, data32);
        EXPECT_EQ(0x0706050403020100, data64);

        EXPECT_EQ(0x01, unaligned8);
        EXPECT_EQ(0x0201, unaligned16);
        EXPECT_EQ(0x04030201, unaligned32);
        EXPECT_EQ(0x0807060504030201, unaligned64);
    }
}

TEST_F(EndiannessFunctionalityTest, UnalignedPut)
{
    BYTE data[9];
    PINT8 pInt8 = (PINT8) data;
    PINT16 pInt16 = (PINT16) data;
    PINT32 pInt32 = (PINT32) data;
    PINT64 pInt64 = (PINT64) data;

    // Initialize the endianness first
    initializeEndianness();

    MEMSET(data, 0x00, SIZEOF(data));
    PUT_UNALIGNED(pInt8, 1);
    EXPECT_EQ(data[0], 1);

    MEMSET(data, 0x00, SIZEOF(data));
    PUT_UNALIGNED(pInt16, 0x0102);
    if (isBigEndian()) {
        EXPECT_EQ(data[0], 0x01);
        EXPECT_EQ(data[1], 0x02);
    }else {
        EXPECT_EQ(data[0], 0x02);
        EXPECT_EQ(data[1], 0x01);
    }

    MEMSET(data, 0x00, SIZEOF(data));
    PUT_UNALIGNED(pInt32, 0x01020304);
    if (isBigEndian()) {
        EXPECT_EQ(data[0], 0x01);
        EXPECT_EQ(data[1], 0x02);
        EXPECT_EQ(data[2], 0x03);
        EXPECT_EQ(data[3], 0x04);
    }else {
        EXPECT_EQ(data[0], 0x04);
        EXPECT_EQ(data[1], 0x03);
        EXPECT_EQ(data[2], 0x02);
        EXPECT_EQ(data[3], 0x01);
    }

    MEMSET(data, 0x00, SIZEOF(data));
    PUT_UNALIGNED(pInt64, 0x0102030405060708);
    if (isBigEndian()) {
        EXPECT_EQ(data[0], 0x01);
        EXPECT_EQ(data[1], 0x02);
        EXPECT_EQ(data[2], 0x03);
        EXPECT_EQ(data[3], 0x04);
        EXPECT_EQ(data[4], 0x05);
        EXPECT_EQ(data[5], 0x06);
        EXPECT_EQ(data[6], 0x07);
        EXPECT_EQ(data[7], 0x08);
    }else {
        EXPECT_EQ(data[0], 0x08);
        EXPECT_EQ(data[1], 0x07);
        EXPECT_EQ(data[2], 0x06);
        EXPECT_EQ(data[3], 0x05);
        EXPECT_EQ(data[4], 0x04);
        EXPECT_EQ(data[5], 0x03);
        EXPECT_EQ(data[6], 0x02);
        EXPECT_EQ(data[7], 0x01);
    }

    //
    // Unaligned write access
    //

    pInt8 = (PINT8) &data[1];
    pInt16 = (PINT16) &data[1];
    pInt32 = (PINT32) &data[1];
    pInt64 = (PINT64) &data[1];

    MEMSET(data, 0x00, SIZEOF(data));
    PUT_UNALIGNED(pInt8, 1);
    EXPECT_EQ(data[0], 0);
    EXPECT_EQ(data[1], 1);

    MEMSET(data, 0x00, SIZEOF(data));
    PUT_UNALIGNED(pInt16, 0x0102);
    EXPECT_EQ(data[0], 0x00);
    if (isBigEndian()) {
        EXPECT_EQ(data[1], 0x01);
        EXPECT_EQ(data[2], 0x02);
    }else {
        EXPECT_EQ(data[1], 0x02);
        EXPECT_EQ(data[2], 0x01);
    }

    MEMSET(data, 0x00, SIZEOF(data));
    PUT_UNALIGNED(pInt32, 0x01020304);
    EXPECT_EQ(data[0], 0x00);
    if (isBigEndian()) {
        EXPECT_EQ(data[1], 0x01);
        EXPECT_EQ(data[2], 0x02);
        EXPECT_EQ(data[3], 0x03);
        EXPECT_EQ(data[4], 0x04);
    }else {
        EXPECT_EQ(data[1], 0x04);
        EXPECT_EQ(data[2], 0x03);
        EXPECT_EQ(data[3], 0x02);
        EXPECT_EQ(data[4], 0x01);
    }

    MEMSET(data, 0x00, SIZEOF(data));
    PUT_UNALIGNED(pInt64, 0x0102030405060708);
    EXPECT_EQ(data[0], 0x00);
    if (isBigEndian()) {
        EXPECT_EQ(data[1], 0x01);
        EXPECT_EQ(data[2], 0x02);
        EXPECT_EQ(data[3], 0x03);
        EXPECT_EQ(data[4], 0x04);
        EXPECT_EQ(data[5], 0x05);
        EXPECT_EQ(data[6], 0x06);
        EXPECT_EQ(data[7], 0x07);
        EXPECT_EQ(data[8], 0x08);
    }else {
        EXPECT_EQ(data[1], 0x08);
        EXPECT_EQ(data[2], 0x07);
        EXPECT_EQ(data[3], 0x06);
        EXPECT_EQ(data[4], 0x05);
        EXPECT_EQ(data[5], 0x04);
        EXPECT_EQ(data[6], 0x03);
        EXPECT_EQ(data[7], 0x02);
        EXPECT_EQ(data[8], 0x01);
    }
}
