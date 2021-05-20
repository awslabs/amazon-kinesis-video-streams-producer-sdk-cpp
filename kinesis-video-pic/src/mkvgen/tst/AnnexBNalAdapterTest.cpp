#include "MkvgenTestFixture.h"

class AnnexBNalAdapterTest : public MkvgenTestBase {
};

TEST_F(AnnexBNalAdapterTest, nalAdapter_InvalidInput)
{
    PBYTE pFrameData = (PBYTE) 100;
    UINT32 frameDataSize = 10;
    PBYTE pAdaptedFrameData = (PBYTE) 110;
    UINT32 adaptedFrameDataSize = 100;

    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(NULL, frameDataSize, FALSE, pAdaptedFrameData, &adaptedFrameDataSize));
    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(NULL, frameDataSize, TRUE, pAdaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(pFrameData, 0, FALSE, pAdaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(pFrameData, 0, TRUE, pAdaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(0, adaptedFrameDataSize);
    adaptedFrameDataSize = frameDataSize - 1;
    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(pFrameData, frameDataSize, TRUE, pAdaptedFrameData, &adaptedFrameDataSize));
    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(pFrameData, frameDataSize, FALSE, pAdaptedFrameData, NULL));
    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(pFrameData, frameDataSize, TRUE, pAdaptedFrameData, NULL));
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_ValidOneByteNonZero)
{
    BYTE frameData[] = {1};
    UINT32 frameDataSize = SIZEOF(frameData);
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, TRUE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, FALSE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, TRUE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, FALSE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_ValidOneByteZero)
{
    BYTE frameData[] = {0};
    UINT32 frameDataSize = SIZEOF(frameData);
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, TRUE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, FALSE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, TRUE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, FALSE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_ValidTwoBytesNonZero)
{
    BYTE frameData[] = {1, 1};
    UINT32 frameDataSize = SIZEOF(frameData);
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, TRUE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, FALSE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, TRUE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, FALSE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_ValidTwoBytesZero)
{
    BYTE frameData[] = {0, 0};
    UINT32 frameDataSize = SIZEOF(frameData);
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, TRUE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, FALSE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, TRUE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, FALSE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_ValidThreeBytesNonZero)
{
    BYTE frameData[] = {1, 1, 1};
    UINT32 frameDataSize = SIZEOF(frameData);
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, TRUE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, FALSE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, TRUE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, FALSE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_ValidThreeBytesZero)
{
    BYTE frameData[] = {0, 0, 0};
    UINT32 frameDataSize = SIZEOF(frameData);
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, TRUE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, FALSE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, TRUE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, FALSE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_ValidFourBytesNonZero)
{
    BYTE frameData[] = {1, 2, 3, 4};
    UINT32 frameDataSize = SIZEOF(frameData);
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, TRUE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, FALSE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, TRUE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, FALSE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_ValidTrailingZeros)
{
    // All zeroes
    BYTE frameData[] = {0, 0, 0, 0};
    UINT32 frameDataSize = SIZEOF(frameData);
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, TRUE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, FALSE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, TRUE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(frameDataSize, adaptedFrameDataSize);
    EXPECT_EQ(0, MEMCMP(adaptedFrameData, frameData, frameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, FALSE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(frameDataSize, adaptedFrameDataSize);
    EXPECT_EQ(0, MEMCMP(adaptedFrameData, frameData, frameDataSize));

    BYTE frameData2[] = {0, 0, 0, 0, 0};
    UINT32 frameData2Size = SIZEOF(frameData2);
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData2, frameData2Size, TRUE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData2, frameData2Size, FALSE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData2, frameData2Size, TRUE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(frameData2Size, adaptedFrameDataSize);
    EXPECT_EQ(0, MEMCMP(adaptedFrameData, frameData2, frameData2Size));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData2, frameData2Size, FALSE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(frameData2Size, adaptedFrameDataSize);
    EXPECT_EQ(0, MEMCMP(adaptedFrameData, frameData2, frameData2Size));

    BYTE frameData3[] = {1, 0, 0, 0, 0};
    UINT32 frameData3Size = SIZEOF(frameData3);
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData3, frameData3Size, TRUE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData3, frameData3Size, FALSE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData3, frameData3Size, TRUE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(frameData3Size, adaptedFrameDataSize);
    EXPECT_EQ(0, MEMCMP(adaptedFrameData, frameData3, frameData3Size));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData3, frameData3Size, FALSE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(frameData3Size, adaptedFrameDataSize);
    EXPECT_EQ(0, MEMCMP(adaptedFrameData, frameData3, frameData3Size));

    BYTE frameData4[] = {0, 0, 0, 1};
    UINT32 frameData4Size = SIZEOF(frameData4);
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData4, frameData4Size, TRUE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData4, frameData4Size, FALSE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData4, frameData4Size, TRUE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(frameData4Size, adaptedFrameDataSize);
    EXPECT_EQ(0, MEMCMP(adaptedFrameData, frameData, frameData4Size));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData4, frameData4Size, FALSE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(frameData4Size, adaptedFrameDataSize);
    EXPECT_EQ(0, MEMCMP(adaptedFrameData, frameData, frameData4Size));

    BYTE frameData5[] = {0, 0, 0, 0, 1};
    UINT32 frameData5Size = SIZEOF(frameData5);
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData5, frameData5Size, TRUE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData5, frameData5Size, FALSE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData5, frameData5Size, FALSE, adaptedFrameData, &adaptedFrameDataSize));
    // Should set the size larger due to extra 0 removal and checking for at least the same size for EPB
    adaptedFrameDataSize += 1;
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData5, frameData5Size, TRUE, adaptedFrameData, &adaptedFrameDataSize));


    BYTE frameData6[] = {0, 0, 0, 1, 0};
    UINT32 frameData6Size = SIZEOF(frameData6);
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData6, frameData6Size, TRUE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData6, frameData6Size, FALSE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData6, frameData6Size, TRUE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(frameData6Size, adaptedFrameDataSize);
    EXPECT_EQ(0, MEMCMP(adaptedFrameData, frameData6, frameData6Size));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData6, frameData6Size, FALSE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(frameData6Size, adaptedFrameDataSize);
    EXPECT_EQ(0, MEMCMP(adaptedFrameData, frameData6, frameData6Size));

    BYTE frameData7[] = {0, 0, 1, 0};
    UINT32 frameData7Size = SIZEOF(frameData7);
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData7, frameData7Size, TRUE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData7, frameData7Size, FALSE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData7, frameData7Size, TRUE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(frameData6Size, adaptedFrameDataSize);
    EXPECT_EQ(0, MEMCMP(adaptedFrameData, frameData6, frameData6Size));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData7, frameData7Size, FALSE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(frameData6Size, adaptedFrameDataSize);
    EXPECT_EQ(0, MEMCMP(adaptedFrameData, frameData6, frameData6Size));

    BYTE frameData8[] = {0, 0, 0, 1, 0, 0, 0, 0};
    BYTE adaptedData8[] = {0, 0, 0, 4, 0, 0, 0, 0};
    UINT32 frameData8Size = SIZEOF(frameData8);
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData8, frameData8Size, TRUE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData8, frameData8Size, FALSE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData8, frameData8Size, TRUE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(frameData8Size, adaptedFrameDataSize);
    EXPECT_EQ(0, MEMCMP(adaptedFrameData, adaptedData8, frameData8Size));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData8, frameData8Size, FALSE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(frameData8Size, adaptedFrameDataSize);
    EXPECT_EQ(0, MEMCMP(adaptedFrameData, adaptedData8, frameData8Size));
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_ValidZerosInStream)
{
    BYTE frameDatas[][10] = {
            {1, 0, 0, 0, 2, 2, 2, 2, 2, 2}, // 0
            {0, 1, 0, 0, 2, 2, 2, 2, 2, 2}, // 1
            {0, 0, 1, 0, 2, 2, 2, 2, 2, 2}, // 2
            {0, 0, 0, 2, 2, 2, 2, 2, 2, 2}, // 3
            {0, 0, 0, 1, 2, 2, 2, 2, 2, 2}, // 4
            {1, 0, 0, 0, 1, 2, 2, 2, 2, 2}, // 5
            {1, 0, 0, 1, 0, 1, 2, 2, 2, 2}, // 6
            {1, 0, 0, 0, 1, 1, 2, 2, 0, 0}, // 7
            {1, 0, 0, 0, 1, 0, 0, 0, 1, 0}, // 8
            {1, 0, 0, 1, 1, 0, 0, 0, 1, 0}, // 9
            {1, 0, 0, 1, 1, 0, 0, 1, 1, 0}, // 10
            {0, 0, 1, 2, 2, 2, 2, 2, 2, 2}, // 11
            {1, 2, 3, 4, 5, 6, 7, 0, 0, 1}, // 12
            {1, 0, 0, 1, 5, 6, 7, 0, 0, 1}, // 13
            {0, 0, 1, 2, 5, 6, 7, 0, 0, 1}, // 14
    };

    UINT32 adaptedSizes[] = {10, 10, 11, 10, 10, 10, 11, 10, 10, 11, 12, 11, 11, 12, 12};

    BYTE adaptedFrameDatas[][12] = {
            {1, 0, 0, 0, 2, 2, 2, 2, 2, 2, 0, 0}, // 0
            {0, 1, 0, 0, 2, 2, 2, 2, 2, 2, 0, 0}, // 1
            {0, 0, 0, 7, 0, 2, 2, 2, 2, 2, 2, 0}, // 2
            {0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 0, 0}, // 3
            {0, 0, 0, 6, 2, 2, 2, 2, 2, 2, 0, 0}, // 4
            {1, 0, 0, 0, 5, 2, 2, 2, 2, 2, 0, 0}, // 5
            {1, 0, 0, 0, 6, 0, 1, 2, 2, 2, 2, 0}, // 6
            {1, 0, 0, 0, 5, 1, 2, 2, 0, 0, 0, 0}, // 7
            {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}, // 8
            {1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0}, // 9
            {1, 0, 0, 0, 1, 1, 0, 0, 0, 2, 1, 0}, // 10
            {0, 0, 0, 7, 2, 2, 2, 2, 2, 2, 2, 0}, // 11
            {1, 2, 3, 4, 5, 6, 7, 0, 0, 0, 0, 0}, // 12
            {1, 0, 0, 0, 3, 5, 6, 7, 0, 0, 0, 0}, // 13
            {0, 0, 0, 4, 2, 5, 6, 7, 0, 0, 0, 0}, // 14
    };

    UINT32 frameDataSize;
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    for (UINT32 i = 0; i < SIZEOF(frameDatas) / SIZEOF(frameDatas[0]); i++) {
        frameDataSize = SIZEOF(frameDatas[i]);
        EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameDatas[i], frameDataSize, TRUE, NULL, &adaptedFrameDataSize));
        EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameDatas[i], frameDataSize, FALSE, NULL, &adaptedFrameDataSize));
        EXPECT_EQ(adaptedSizes[i], adaptedFrameDataSize);
        MEMSET(adaptedFrameData, 0xff, SIZEOF(adaptedFrameData));
        EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameDatas[i], frameDataSize, TRUE, adaptedFrameData, &adaptedFrameDataSize));
        EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameDatas[i], frameDataSize, FALSE, adaptedFrameData, &adaptedFrameDataSize));
        EXPECT_EQ(adaptedSizes[i], adaptedFrameDataSize);
        EXPECT_EQ(0, MEMCMP(adaptedFrameDatas[i], adaptedFrameData, adaptedFrameDataSize)) << "Failed comparison on index: " << i;
    }
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_ValidSpsPpsOnly)
{
    BYTE frameData1[] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x40, 0x1f,
                        0x96, 0x54, 0x02, 0x80, 0x2d, 0xc8, 0x00, 0x00,
                        0x00, 0x01, 0x68, 0xce, 0x38, 0x80};

    BYTE frameData2[] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x1e,
                         0xa9, 0x50, 0x14, 0x07, 0xb4, 0x20, 0x00, 0x00,
                         0x7d, 0x00, 0x00, 0x1d, 0x4c, 0x00, 0x80, 0x00,
                         0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80};
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData1, SIZEOF(frameData1), TRUE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData1, SIZEOF(frameData1), FALSE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData1, SIZEOF(frameData1), TRUE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData1, SIZEOF(frameData1), FALSE, adaptedFrameData, &adaptedFrameDataSize));

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData2, SIZEOF(frameData2), TRUE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData2, SIZEOF(frameData2), FALSE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData2, SIZEOF(frameData2), TRUE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData2, SIZEOF(frameData2), FALSE, adaptedFrameData, &adaptedFrameDataSize));
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_ValidEPB)
{
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize;

    // These are series of codec private data bits extracted from an
    // Android device for H265 for various resolutions
    BYTE frames[8][1000] = {
            {0x00, 0x00, 0x00, 0x01, 0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00,
                    0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x3c, 0xac, 0x59, 0x00, 0x00, 0x00, 0x01, 0x42,
                    0x01, 0x01, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00, 0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00,
                    0x3c, 0xa0, 0x18, 0x20, 0x28, 0x71, 0x31, 0x39, 0x6b, 0xb9, 0x32, 0x4b, 0xb9, 0x48, 0x28, 0x10,
                    0x10, 0x17, 0x68, 0x50, 0x94, 0x00, 0x00, 0x00, 0x01, 0x44, 0x01, 0xc0, 0xf1, 0x80, 0x04, 0x20},

            {0x00, 0x00, 0x00, 0x01, 0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00,
                    0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x3c, 0xac, 0x59, 0x00, 0x00, 0x00, 0x01, 0x42,
                    0x01, 0x01, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00, 0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00,
                    0x3c, 0xa0, 0x0a, 0x08, 0x04, 0x07, 0xc4, 0xe5, 0xae, 0xe4, 0xc9, 0x2e, 0xe5, 0x20, 0xa0, 0x40,
                    0x40, 0x5d, 0xa1, 0x42, 0x50, 0x00, 0x00, 0x00, 0x01, 0x44, 0x01, 0xc0, 0xf1, 0x80, 0x04, 0x20},

            {0x00, 0x00, 0x00, 0x01, 0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00,
                    0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x5a, 0xac, 0x59, 0x00, 0x00, 0x00, 0x01, 0x42,
                    0x01, 0x01, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00, 0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00,
                    0x5a, 0xa0, 0x05, 0x02, 0x01, 0xe1, 0x65, 0xae, 0xe4, 0xc9, 0x2e, 0xe5, 0x20, 0xa0, 0x40, 0x40},

            {0x00, 0x00, 0x00, 0x01, 0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00,
                    0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x5a, 0xac, 0x59, 0x00, 0x00, 0x00, 0x01, 0x42,
                    0x01, 0x01, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00, 0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00,
                    0x5a, 0xa0, 0x06, 0x02, 0x01, 0xc1, 0xf1, 0x39, 0x6b, 0xb9, 0x32, 0x4b, 0xb9, 0x48, 0x28, 0x10,
                    0x10, 0x17, 0x68, 0x50, 0x94, 0x00, 0x00, 0x00, 0x01, 0x44, 0x01, 0xc0, 0xf1, 0x80, 0x04, 0x20},

            {0x00, 0x00, 0x00, 0x01, 0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00,
                    0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x5a, 0xac, 0x59, 0x00, 0x00, 0x00, 0x01, 0x42,
                    0x01, 0x01, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00, 0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00,
                    0x5a, 0xa0, 0x05, 0xc2, 0x01, 0xe1, 0xc4, 0xf9, 0x6b, 0xb9, 0x32, 0x4b, 0xb9, 0x48, 0x28, 0x10,
                    0x10, 0x17, 0x68, 0x50, 0x94, 0x00, 0x00, 0x00, 0x01, 0x44, 0x01, 0xc0, 0xf1, 0x80, 0x04, 0x20},

            {0x00, 0x00, 0x00, 0x01, 0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00,
                    0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x5d, 0xac, 0x59, 0x00, 0x00, 0x00, 0x01, 0x42,
                    0x01, 0x01, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00, 0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00,
                    0x5d, 0xa0, 0x07, 0x82, 0x00, 0xb8, 0x7c, 0x4e, 0x5a, 0xee, 0x4c, 0x92, 0xee, 0x52, 0x0a, 0x04,
                    0x04, 0x05, 0xda, 0x14, 0x25, 0x00, 0x00, 0x00, 0x01, 0x44, 0x01, 0xc0, 0xf1, 0x80, 0x04, 0x20},

            {0x00, 0x00, 0x00, 0x01, 0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00,
                    0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x5d, 0xac, 0x59, 0x00, 0x00, 0x00, 0x01, 0x42,
                    0x01, 0x01, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00, 0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00,
                    0x5d, 0xa0, 0x02, 0x80, 0x80, 0x2e, 0x1f, 0x13, 0x96, 0xbb, 0x93, 0x24, 0xbb, 0x94, 0x82, 0x81,
                    0x01, 0x01, 0x76, 0x85, 0x09, 0x40, 0x00, 0x00, 0x00, 0x01, 0x44, 0x01, 0xc0, 0xf1, 0x80, 0x04},

            {0x00, 0x00, 0x00, 0x01, 0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00,
                    0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x5d, 0xac, 0x59, 0x00, 0x00, 0x00, 0x01, 0x42,
                    0x01, 0x01, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00, 0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00,
                    0x5d, 0xa0, 0x02, 0x80, 0x80, 0x3c, 0x16, 0x5a, 0xee, 0x4c, 0x92, 0xee, 0x52, 0x0a, 0x04, 0x04},
    };

    UINT32 frameSizes[8] = {80, 80, 64, 80, 80, 80, 80, 64};

    UINT16 i;

    for (i = 0; i < ARRAY_SIZE(frameSizes); i++) {
        adaptedFrameDataSize = 0;
        EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frames[i], frameSizes[i], TRUE, NULL, &adaptedFrameDataSize)) << "Failed on iteration " << i;
        EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frames[i], frameSizes[i], TRUE, adaptedFrameData, &adaptedFrameDataSize)) << "Failed on iteration " << i;

        adaptedFrameDataSize = 0;
        EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frames[i], frameSizes[i], FALSE, NULL, &adaptedFrameDataSize)) << "Failed on iteration " << i;
        EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frames[i], frameSizes[i], FALSE, adaptedFrameData, &adaptedFrameDataSize)) << "Failed on iteration " << i;
    }
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_badRealLifeEncoderSampleWithFix)
{
    // I-frame from a real-life encoder output which is actually invalid Annex-B format (shortened after a few bytes of the actual frame)
    BYTE frameData[] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10, 0x00, 0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x1E, 0xAC,
                        0x1B, 0x1A, 0x80, 0xB0, 0x3D, 0xFF, 0xFF, 0x00, 0x28, 0x00, 0x21, 0x6E, 0x0C, 0x0C, 0x0C, 0x80,
                        0x00, 0x01, 0xF4, 0x00, 0x00, 0x75, 0x30, 0x74, 0x30, 0x07, 0xD0, 0x00, 0x01, 0x31, 0x2D, 0x5D,
                        0xE5, 0xC6, 0x86, 0x00, 0xFA, 0x00, 0x00, 0x26, 0x25, 0xAB, 0xBC, 0xB8, 0x50, 0x00, 0x00, 0x00,
                        0x00, 0x01, 0x68, 0xEE, 0x38, 0x30, 0x00, 0x00, 0x00, 0x00, 0x01, 0x06, 0x00, 0x0D, 0xBC, 0xFF,
                        0x87, 0x49, 0xB5, 0x16, 0x3C, 0xFF, 0x87, 0x49, 0xB5, 0x16, 0x40, 0x01, 0x04, 0x00, 0x78, 0x08,
                        0x10, 0x06, 0x01, 0xC4, 0x80, 0x00, 0x00, 0x00, 0x00, 0x01, 0x65, 0xB8, 0x00, 0x02, 0x00, 0x00,
                        0x03, 0x02, 0x7F, 0xEC, 0x0E, 0xD0, 0xE1, 0xA7, 0x9D, 0xA3, 0x7C, 0x49, 0x42, 0xC2, 0x23, 0x59,};

    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, SIZEOF(frameData), TRUE, NULL, &adaptedFrameDataSize));

    // Need to account for the removal of zeroes and the fact that with EPB we are requiring at least the same size as frame data
    adaptedFrameDataSize += 4;
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, SIZEOF(frameData), TRUE, adaptedFrameData, &adaptedFrameDataSize));

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, SIZEOF(frameData), FALSE, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, SIZEOF(frameData), FALSE, adaptedFrameData, &adaptedFrameDataSize));

    // 4 larger than normal Annex-B start codes as with no-EPB we are returning the actual size required
    EXPECT_EQ(SIZEOF(frameData) - 4, adaptedFrameDataSize);

    // We have 5 NALUs here with run sizes of 2, 50, 4, 26, 22
    UINT32 expectedRunSizes[] = {2, 50, 4, 26, 22};

    PUINT32 pRunSize = (PUINT32) adaptedFrameData;
    for (UINT32 i = 0; i < ARRAY_SIZE(expectedRunSizes); i++) {
        UINT32 runSize = GET_UNALIGNED_BIG_ENDIAN(pRunSize);
        EXPECT_EQ(expectedRunSizes[i], runSize);

        // Increment the run pointer which is the current incremented by 1 (4 bytes of run) and the byte cast pointer size of the run
        pRunSize = (PUINT32) ((PBYTE) (pRunSize + 1) + runSize);
    }

    // Extract the SPS and PPS
    PBYTE pSps, pPps;
    UINT32 spsSize, ppsSize;
    EXPECT_EQ(STATUS_SUCCESS,
              getH264SpsPpsNalusFromAvccNalus(adaptedFrameData, adaptedFrameDataSize, &pSps, &spsSize, &pPps, &ppsSize));

    EXPECT_TRUE(pSps != NULL);
    EXPECT_EQ(50, spsSize);

    EXPECT_TRUE(pPps != NULL);
    EXPECT_EQ(4, ppsSize);
}
