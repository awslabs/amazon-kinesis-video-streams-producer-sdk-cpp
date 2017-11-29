#include "MkvgenTestFixture.h"

class AnnexBNalAdapterTest : public MkvgenTestBase {
};

TEST_F(AnnexBNalAdapterTest, nalAdapter_InvalidInput)
{
    PBYTE pFrameData = (PBYTE) 100;
    UINT32 frameDataSize = 10;
    PBYTE pAdaptedFrameData = (PBYTE) 110;
    UINT32 adaptedFrameDataSize = 100;

    EXPECT_NE(STATUS_SUCCESS,
              adaptFrameNalsFromAnnexBToAvcc(NULL, frameDataSize, pAdaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(pFrameData, 0, pAdaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(0, adaptedFrameDataSize);
    adaptedFrameDataSize = frameDataSize - 1;
    EXPECT_NE(STATUS_SUCCESS,
              adaptFrameNalsFromAnnexBToAvcc(pFrameData, frameDataSize, pAdaptedFrameData, &adaptedFrameDataSize));
    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(pFrameData, frameDataSize, pAdaptedFrameData, NULL));
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_ValidOneByteNonZero)
{
    BYTE frameData[] = {1};
    UINT32 frameDataSize = SIZEOF(frameData);
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
    EXPECT_EQ(STATUS_SUCCESS,
              adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_ValidOneByteZero)
{
    BYTE frameData[] = {0};
    UINT32 frameDataSize = SIZEOF(frameData);
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
    EXPECT_EQ(STATUS_SUCCESS,
              adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_ValidTwoBytesNonZero)
{
    BYTE frameData[] = {1, 1};
    UINT32 frameDataSize = SIZEOF(frameData);
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
    EXPECT_EQ(STATUS_SUCCESS,
              adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_ValidTwoBytesZero)
{
    BYTE frameData[] = {0, 0};
    UINT32 frameDataSize = SIZEOF(frameData);
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
    EXPECT_EQ(STATUS_SUCCESS,
              adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_ValidThreeBytesNonZero)
{
    BYTE frameData[] = {1, 1, 1};
    UINT32 frameDataSize = SIZEOF(frameData);
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
    EXPECT_EQ(STATUS_SUCCESS,
              adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_ValidThreeBytesZero)
{
    BYTE frameData[] = {0, 0, 0};
    UINT32 frameDataSize = SIZEOF(frameData);
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
    EXPECT_EQ(STATUS_SUCCESS,
              adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_ValidFourBytesNonZero)
{
    BYTE frameData[] = {1, 2, 3, 4};
    UINT32 frameDataSize = SIZEOF(frameData);
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, NULL, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
    EXPECT_EQ(STATUS_SUCCESS,
              adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, adaptedFrameData, &adaptedFrameDataSize));
    EXPECT_EQ(SIZEOF(frameData), adaptedFrameDataSize);
}

TEST_F(AnnexBNalAdapterTest, nalAdapter_InvalidFourBytesZero)
{
    BYTE frameData[] = {0, 0, 0, 0};
    UINT32 frameDataSize = SIZEOF(frameData);
    BYTE adaptedFrameData[1000];
    UINT32 adaptedFrameDataSize = SIZEOF(adaptedFrameData);

    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, NULL, &adaptedFrameDataSize));
    EXPECT_NE(STATUS_SUCCESS,
              adaptFrameNalsFromAnnexBToAvcc(frameData, frameDataSize, adaptedFrameData, &adaptedFrameDataSize));
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
        EXPECT_EQ(STATUS_SUCCESS,
                  adaptFrameNalsFromAnnexBToAvcc(frameDatas[i], frameDataSize, NULL, &adaptedFrameDataSize));
        EXPECT_EQ(adaptedSizes[i], adaptedFrameDataSize);
        MEMSET(adaptedFrameData, 0xff, SIZEOF(adaptedFrameData));
        EXPECT_EQ(STATUS_SUCCESS,
                  adaptFrameNalsFromAnnexBToAvcc(frameDatas[i], frameDataSize, adaptedFrameData, &adaptedFrameDataSize));
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

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData1, SIZEOF(frameData1), NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData1, SIZEOF(frameData1), adaptedFrameData, &adaptedFrameDataSize));

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData2, SIZEOF(frameData2), NULL, &adaptedFrameDataSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAnnexBToAvcc(frameData2, SIZEOF(frameData2), adaptedFrameData, &adaptedFrameDataSize));
}