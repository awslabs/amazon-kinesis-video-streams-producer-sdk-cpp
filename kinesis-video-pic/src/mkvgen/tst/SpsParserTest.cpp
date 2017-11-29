#include "MkvgenTestFixture.h"

class SpsParserTest : public MkvgenTestBase {
};

TEST_F(SpsParserTest, spsParser_InvalidInput)
{
    PBYTE cpd = (PBYTE) 100;
    UINT16 width, height;

    EXPECT_NE(STATUS_SUCCESS, getVideoWidthAndHeightFromSps(NULL, 100, &width, &height));
    EXPECT_NE(STATUS_SUCCESS, getVideoWidthAndHeightFromSps(cpd, 0, &width, &height));
    EXPECT_NE(STATUS_SUCCESS, getVideoWidthAndHeightFromSps(cpd, MIN_H264_H265_CPD_SIZE - 1, &width, &height));
    EXPECT_NE(STATUS_SUCCESS, getVideoWidthAndHeightFromSps(cpd, 100, NULL, &height));
    EXPECT_NE(STATUS_SUCCESS, getVideoWidthAndHeightFromSps(cpd, 100, &width, NULL));
    EXPECT_NE(STATUS_SUCCESS, getVideoWidthAndHeightFromSps(NULL, 0, NULL, NULL));

    EXPECT_NE(STATUS_SUCCESS, parseSpsGetResolution(NULL, 100, &width, &height));
    EXPECT_NE(STATUS_SUCCESS, parseSpsGetResolution(cpd, 0, &width, &height));
    EXPECT_NE(STATUS_SUCCESS, parseSpsGetResolution(cpd, 100, NULL, &height));
    EXPECT_NE(STATUS_SUCCESS, parseSpsGetResolution(cpd, 100, &width, NULL));
    EXPECT_NE(STATUS_SUCCESS, parseSpsGetResolution(cpd, 100, NULL, NULL));
}

TEST_F(SpsParserTest, spsParser_Raw_Sps)
{
    BYTE cpd[] = {0x67, 0x42, 0x40, 0x1f, 0x96, 0x54, 0x02, 0x80,
                  0x2d, 0xc8, 0x68, 0xce, 0x38, 0x80};
    UINT32 cpdSize = SIZEOF(cpd);
    UINT16 width, height;

    EXPECT_EQ(STATUS_SUCCESS, getVideoWidthAndHeightFromSps(cpd, cpdSize, &width, &height));
    EXPECT_EQ(1280, width);
    EXPECT_EQ(720, height);
}

TEST_F(SpsParserTest, spsParser_AnnexB)
{
    BYTE cpd1[] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x40, 0x1f,
                   0x96, 0x54, 0x02, 0x80, 0x2d, 0xc8, 0x00, 0x00,
                   0x00, 0x01, 0x68, 0xce, 0x38, 0x80};
    BYTE cpd2[] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x1e,
                   0xa9, 0x50, 0x14, 0x07, 0xb4, 0x20, 0x00, 0x00,
                   0x7d, 0x00, 0x00, 0x1d, 0x4c, 0x00, 0x80, 0x00,
                   0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80};

    UINT16 width, height;

    EXPECT_EQ(STATUS_SUCCESS, getVideoWidthAndHeightFromSps(cpd1, SIZEOF(cpd1), &width, &height));
    EXPECT_EQ(1280, width);
    EXPECT_EQ(720, height);

    EXPECT_EQ(STATUS_SUCCESS, getVideoWidthAndHeightFromSps(cpd2, SIZEOF(cpd2), &width, &height));
    EXPECT_EQ(640, width);
    EXPECT_EQ(480, height);
}

TEST_F(SpsParserTest, spsParser_Avcc) {
    BYTE cpd1[] = {0x01, 0x64, 0x00, 0x28, 0xff, 0xe1, 0x00, 0x2e,
                  0x67, 0x64, 0x00, 0x28, 0xac, 0x2c, 0xa4, 0x01,
                  0xe0, 0x08, 0x9f, 0x97, 0xff, 0x00, 0x01, 0x00,
                  0x01, 0x52, 0x02, 0x02, 0x02, 0x80, 0x00, 0x01,
                  0xf4, 0x80, 0x00, 0x75, 0x30, 0x70, 0x10, 0x00,
                  0x16, 0xe3, 0x60, 0x00, 0x08, 0x95, 0x45, 0xf8,
                  0xc7, 0x07, 0x68, 0x58, 0xb4, 0x48, 0x01, 0x00,
                  0x05, 0x68, 0xeb, 0x73, 0x52, 0x50, 0xfd, 0xf8,
                  0xf8, 0x00};
    BYTE cpd2[] = {0x01, 0x42, 0x40, 0x15, 0xFF, 0xE1, 0x00, 0x0a,
                   0x67, 0x42, 0x40, 0x33, 0x95, 0xa0, 0x1e, 0x00,
                   0xb5, 0x90, 0x01, 0x00, 0x04, 0x68, 0xce, 0x3c,
                   0x80};

    UINT16 width, height;

    EXPECT_EQ(STATUS_SUCCESS, getVideoWidthAndHeightFromSps(cpd1, SIZEOF(cpd1), &width, &height));
    EXPECT_EQ(1920, width);
    EXPECT_EQ(1080, height);

    EXPECT_EQ(STATUS_SUCCESS, getVideoWidthAndHeightFromSps(cpd2, SIZEOF(cpd2), &width, &height));
    EXPECT_EQ(1920, width);
    EXPECT_EQ(1440, height);
}

TEST_F(SpsParserTest, spsParser_Mjpg) {
    BYTE cpd[] = {0x28, 0x00, 0x00, 0x00, 0x80, 0x02, 0x00, 0x00,
                  0xe0, 0x01, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00,
                  0x4d, 0x4a, 0x50, 0x47, 0x00, 0x10, 0x0e, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    UINT16 width, height;

    EXPECT_EQ(STATUS_SUCCESS, getVideoWidthAndHeightFromBih(cpd, SIZEOF(cpd), &width, &height));
    EXPECT_EQ(640, width);
    EXPECT_EQ(480, height);

    EXPECT_EQ(STATUS_MKV_INVALID_BIH_CPD, getVideoWidthAndHeightFromBih(cpd, SIZEOF(cpd) - 1, &width, &height));
}