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
    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(pFrameData, frameDataSize, FALSE, pAdaptedFrameData, &adaptedFrameDataSize));
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

TEST_F(AnnexBNalAdapterTest, nalAdapter_InvalidFourBytesZero)
{
    BYTE frameData[] = {0, 0, 0, 0};
    UINT32 frameDataSize = SIZEOF(frameData);
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, TRUE, NULL, &adaptedFrameDataSize));
    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, FALSE, NULL, &adaptedFrameDataSize));
    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, TRUE, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, FALSE, adaptedFrameData, &adaptedFrameDataSize));
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

    for (i = 0; i < 8; i++) {
        adaptedFrameDataSize = 0;
        EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frames[i], frameSizes[i], TRUE, NULL, &adaptedFrameDataSize)) << "Failed on iteration " << i;
        EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frames[i], frameSizes[i], TRUE, adaptedFrameData, &adaptedFrameDataSize)) << "Failed on iteration " << i;

        adaptedFrameDataSize = 0;
        EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frames[i], frameSizes[i], FALSE, NULL, &adaptedFrameDataSize)) << "Failed on iteration " << i;
        EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frames[i], frameSizes[i], FALSE, adaptedFrameData, &adaptedFrameDataSize)) << "Failed on iteration " << i;
    }
}