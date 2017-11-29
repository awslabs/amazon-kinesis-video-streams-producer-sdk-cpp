#include "MkvgenTestFixture.h"

class AnnexBCpdNalAdapterTest : public MkvgenTestBase {
};

TEST_F(AnnexBCpdNalAdapterTest, nalAdapter_InvalidInput)
{
    PBYTE pCpd = (PBYTE) 100;
    UINT32 cpdSize = 10;
    PBYTE pAdaptedCpd = (PBYTE) 110;
    UINT32 adaptedCpdSize = 100;

    EXPECT_NE(STATUS_SUCCESS, adaptCpdNalsFromAnnexBToAvcc(NULL, cpdSize, pAdaptedCpd, &adaptedCpdSize));
    EXPECT_NE(STATUS_SUCCESS, adaptCpdNalsFromAnnexBToAvcc(pCpd, cpdSize, NULL, NULL));
    EXPECT_NE(STATUS_SUCCESS, adaptCpdNalsFromAnnexBToAvcc(pCpd, cpdSize, pAdaptedCpd, NULL));
    EXPECT_NE(STATUS_SUCCESS, adaptCpdNalsFromAnnexBToAvcc(pCpd, 0, pAdaptedCpd, &adaptedCpdSize));
    EXPECT_NE(STATUS_SUCCESS, adaptCpdNalsFromAnnexBToAvcc(pCpd, 1, pAdaptedCpd, &adaptedCpdSize));
    EXPECT_NE(STATUS_SUCCESS, adaptCpdNalsFromAnnexBToAvcc(pCpd, MIN_ANNEXB_CPD_SIZE - 1, pAdaptedCpd, &adaptedCpdSize));
}

TEST_F(AnnexBCpdNalAdapterTest, nalAdapter_InvalidNoStartCode)
{
    BYTE cpd[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    BYTE adaptedCpd[100];
    UINT32 cpdSize = SIZEOF(cpd);
    UINT32 adaptedSize;

    EXPECT_EQ(STATUS_SUCCESS, adaptCpdNalsFromAnnexBToAvcc(cpd, cpdSize, NULL, &adaptedSize));
    EXPECT_EQ(16, adaptedSize);

    adaptedSize = 100;
    EXPECT_NE(STATUS_SUCCESS, adaptCpdNalsFromAnnexBToAvcc(cpd, cpdSize, adaptedCpd, &adaptedSize));
    EXPECT_EQ(16, adaptedSize);
}

TEST_F(AnnexBCpdNalAdapterTest, nalAdapter_ValidZerosInStream)
{
    BYTE cpds[][21] = {
            {0, 0, 1, 1, 2, 3, 4, 10, 11, 12, 13, 14, 15, 0, 0, 1, 2, 3, 4, 5, 6}, // 0
            {0, 0, 0, 1, 1, 2, 3, 4, 10, 11, 12, 13, 14, 15, 0, 0, 0, 1, 2, 3, 4}, // 1
    };

    UINT32 adaptedSizes[] = {26, 26};
    UINT32 actualAdaptedSizes[] = {26, 24};

    BYTE adaptedCpds[][26] = {
            {1, 2, 3, 4, 0xff, 0xE1, 0, 10, 1, 2, 3, 4, 10, 11, 12, 13, 14, 15, 1, 0, 5, 2, 3, 4, 5, 6}, // 0
            {1, 2, 3, 4, 0xff, 0xE1, 0, 10, 1, 2, 3, 4, 10, 11, 12, 13, 14, 15, 1, 0, 3, 2, 3, 4, 0, 0}, // 1
    };

    UINT32 cpdSize;
    BYTE adaptedCpd[1000];
    UINT32 adaptedCpdSize = SIZEOF(adaptedCpd);

    for (UINT32 i = 0; i < SIZEOF(cpds) / SIZEOF(cpds[0]); i++) {
        cpdSize = SIZEOF(cpds[i]);
        EXPECT_EQ(STATUS_SUCCESS, adaptCpdNalsFromAnnexBToAvcc(cpds[i], cpdSize, NULL, &adaptedCpdSize));
        EXPECT_EQ(adaptedSizes[i], adaptedCpdSize);
        MEMSET(adaptedCpd, 0x00, SIZEOF(adaptedCpd));
        EXPECT_EQ(STATUS_SUCCESS, adaptCpdNalsFromAnnexBToAvcc(cpds[i], cpdSize, adaptedCpd, &adaptedCpdSize));
        EXPECT_EQ(actualAdaptedSizes[i], adaptedCpdSize);
        EXPECT_EQ(0, MEMCMP(adaptedCpds[i], adaptedCpd, adaptedCpdSize)) << "Failed comparison on index: " << i;
    }
}

TEST_F(AnnexBCpdNalAdapterTest, nalAdapter_ValidSpsPps)
{
    BYTE cpd[] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x1e,
                        0xa9, 0x50, 0x14, 0x07, 0xb4, 0x20, 0x00, 0x00,
                        0x7d, 0x00, 0x00, 0x1d, 0x4c, 0x00, 0x80, 0x00,
                        0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80};
    UINT32 cpdSize = SIZEOF(cpd);
    BYTE adaptedCpd[1000];
    UINT32 adaptedCpdSize = SIZEOF(adaptedCpd);

    EXPECT_EQ(STATUS_SUCCESS, adaptCpdNalsFromAnnexBToAvcc(cpd, cpdSize, NULL, &adaptedCpdSize));
    EXPECT_EQ(STATUS_SUCCESS, adaptCpdNalsFromAnnexBToAvcc(cpd, cpdSize, adaptedCpd, &adaptedCpdSize));
}