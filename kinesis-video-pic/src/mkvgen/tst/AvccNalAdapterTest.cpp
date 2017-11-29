#include "MkvgenTestFixture.h"

class AvccNalAdapterTest : public MkvgenTestBase {
};

TEST_F(AvccNalAdapterTest, nalAdapter_InvalidInput)
{
    PBYTE pFrameData = (PBYTE) 100;
    UINT32 frameDataSize = 10;

    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAvccToAnnexB(NULL, frameDataSize));
    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAvccToAnnexB(NULL, 0));
    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAvccToAnnexB(NULL, 1));
    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAvccToAnnexB(NULL, 2));
    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAvccToAnnexB(NULL, 3));
    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAvccToAnnexB(pFrameData, 0));
    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAvccToAnnexB(pFrameData, 1));
    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAvccToAnnexB(pFrameData, 2));
    EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAvccToAnnexB(pFrameData, 3));
}

TEST_F(AvccNalAdapterTest, nalAdapter_ValidEmpty)
{
    BYTE frameData[] = {0, 0, 0, 0};
    UINT32 frameDataSize = SIZEOF(frameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAvccToAnnexB(frameData, frameDataSize));
    EXPECT_EQ(0, frameData[0]);
    EXPECT_EQ(0, frameData[1]);
    EXPECT_EQ(0, frameData[2]);
    EXPECT_EQ(1, frameData[3]);
}

TEST_F(AvccNalAdapterTest, nalAdapter_ValidOneByte)
{
    BYTE frameData[] = {0, 0, 0, 1, 2};
    UINT32 frameDataSize = SIZEOF(frameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAvccToAnnexB(frameData, frameDataSize));
    EXPECT_EQ(0, frameData[0]);
    EXPECT_EQ(0, frameData[1]);
    EXPECT_EQ(0, frameData[2]);
    EXPECT_EQ(1, frameData[3]);
    EXPECT_EQ(2, frameData[4]);
}

TEST_F(AvccNalAdapterTest, nalAdapter_ValidTwoBytes)
{
    BYTE frameData[] = {0, 0, 0, 2, 2, 3};
    UINT32 frameDataSize = SIZEOF(frameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAvccToAnnexB(frameData, frameDataSize));
    EXPECT_EQ(0, frameData[0]);
    EXPECT_EQ(0, frameData[1]);
    EXPECT_EQ(0, frameData[2]);
    EXPECT_EQ(1, frameData[3]);
    EXPECT_EQ(2, frameData[4]);
    EXPECT_EQ(3, frameData[5]);
}

TEST_F(AvccNalAdapterTest, nalAdapter_ValidThreeBytes)
{
    BYTE frameData[] = {0, 0, 0, 3, 2, 3, 4};
    UINT32 frameDataSize = SIZEOF(frameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAvccToAnnexB(frameData, frameDataSize));
    EXPECT_EQ(0, frameData[0]);
    EXPECT_EQ(0, frameData[1]);
    EXPECT_EQ(0, frameData[2]);
    EXPECT_EQ(1, frameData[3]);
    EXPECT_EQ(2, frameData[4]);
    EXPECT_EQ(3, frameData[5]);
    EXPECT_EQ(4, frameData[6]);
}

TEST_F(AvccNalAdapterTest, nalAdapter_ValidFourBytes)
{
    BYTE frameData[] = {0, 0, 0, 4, 2, 3, 4, 5};
    UINT32 frameDataSize = SIZEOF(frameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAvccToAnnexB(frameData, frameDataSize));
    EXPECT_EQ(0, frameData[0]);
    EXPECT_EQ(0, frameData[1]);
    EXPECT_EQ(0, frameData[2]);
    EXPECT_EQ(1, frameData[3]);
    EXPECT_EQ(2, frameData[4]);
    EXPECT_EQ(3, frameData[5]);
    EXPECT_EQ(4, frameData[6]);
    EXPECT_EQ(5, frameData[7]);
}

TEST_F(AvccNalAdapterTest, nalAdapter_MultiNal)
{
    BYTE frameData[] = {0, 0, 0, 4, 2, 3, 4, 5, 0, 0, 0, 0, 0, 0, 0, 1, 6, 0, 0, 0, 0};
    BYTE adaptedFrameData[] = {0, 0, 0, 1, 2, 3, 4, 5, 0, 0, 0, 1, 0, 0, 0, 1, 6, 0, 0, 0, 1};
    UINT32 frameDataSize = SIZEOF(frameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAvccToAnnexB(frameData, frameDataSize));
    EXPECT_EQ(0, MEMCMP(adaptedFrameData, frameData, frameDataSize));
}

TEST_F(AvccNalAdapterTest, nalAdapter_MultiNalSkipToEnd)
{
    BYTE frameData[] = {0, 0, 0, 4, 2, 3, 4, 5, 0, 0, 0, 9, 0, 0, 0, 1, 6, 0, 0, 0, 0};
    BYTE adaptedFrameData[] = {0, 0, 0, 1, 2, 3, 4, 5, 0, 0, 0, 1, 0, 0, 0, 1, 6, 0, 0, 0, 0};
    UINT32 frameDataSize = SIZEOF(frameData);

    EXPECT_EQ(STATUS_SUCCESS, adaptFrameNalsFromAvccToAnnexB(frameData, frameDataSize));
    EXPECT_EQ(0, MEMCMP(adaptedFrameData, frameData, frameDataSize));
}

TEST_F(AvccNalAdapterTest, nalAdapter_OutOfBounds)
{
    UINT32 frameDataSize;
    BYTE frameDatas[][21] = {
            {0, 0, 0, 4, 2, 3, 4, 5, 0, 0, 0, 10, 0, 0, 0, 1, 6, 0, 0, 0, 0}, // 0
            {0, 0, 0, 4, 2, 3, 4, 5, 0, 0, 0, 8, 0, 0, 0, 1, 6, 0, 0, 0, 0}, // 1
            {0, 0, 0, 4, 2, 3, 4, 5, 0, 0, 0, 7, 0, 0, 0, 1, 6, 0, 0, 0, 0}, // 1
            {0, 0, 0, 4, 2, 3, 4, 5, 0, 0, 0, 6, 0, 0, 0, 1, 6, 0, 0, 0, 0}, // 1
    };

    for (UINT32 i = 0; i < SIZEOF(frameDatas) / SIZEOF(frameDatas[0]); i++) {
        frameDataSize = SIZEOF(frameDatas[i]);
        EXPECT_NE(STATUS_SUCCESS, adaptFrameNalsFromAvccToAnnexB(frameDatas[i], frameDataSize));
    }
}
