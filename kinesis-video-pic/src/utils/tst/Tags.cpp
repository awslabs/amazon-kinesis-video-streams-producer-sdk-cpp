#include "UtilTestFixture.h"

class TagsFunctionalityTest : public UtilTestBase {
};

TEST_F(TagsFunctionalityTest, basisTagsFunctionality)
{
    Tag tags[10];
    UINT32 i, tagsCount = ARRAY_SIZE(tags), retSize;
    PTag pDstTags = (PTag) MEMALLOC(tagsCount * TAG_FULL_LENGTH);

    for (i = 0; i < tagsCount; i++) {
        tags[i].version = TAG_CURRENT_VERSION;
        tags[i].name = (PCHAR) MEMALLOC((MAX_TAG_NAME_LEN + 2) * SIZEOF(CHAR));
        tags[i].value = (PCHAR) MEMALLOC((MAX_TAG_VALUE_LEN + 2) * SIZEOF(CHAR));

        SPRINTF(tags[i].name, "Tag Name %u", i);
        SPRINTF(tags[i].value, "Tag Value %u", i);
    }

    EXPECT_EQ(STATUS_SUCCESS, validateTags(0, NULL));
    EXPECT_EQ(STATUS_SUCCESS, validateTags(0, tags));
    EXPECT_NE(STATUS_SUCCESS, validateTags(tagsCount, NULL));

    // wrong version
    tags[5].version = TAG_CURRENT_VERSION + 1;
    EXPECT_NE(STATUS_SUCCESS, validateTags(tagsCount, tags));
    tags[5].version = TAG_CURRENT_VERSION;

    // Long name
    MEMSET(tags[5].name, 'A', (MAX_TAG_NAME_LEN + 1) * SIZEOF(CHAR));
    tags[5].name[MAX_TAG_NAME_LEN + 1] = '\0';
    EXPECT_NE(STATUS_SUCCESS, validateTags(tagsCount, tags));
    STRCPY(tags[5].name, "Tag Name 5");

    // Long value
    MEMSET(tags[5].value, 'A', (MAX_TAG_VALUE_LEN + 1) * SIZEOF(CHAR));
    tags[5].value[MAX_TAG_VALUE_LEN + 1] = '\0';
    EXPECT_NE(STATUS_SUCCESS, validateTags(tagsCount, tags));
    STRCPY(tags[5].value, "Tag Value 5");

    EXPECT_NE(STATUS_SUCCESS, packageTags(1, NULL, 0, NULL, &retSize));
    EXPECT_EQ(STATUS_SUCCESS, packageTags(0, tags, 0, NULL, &retSize));
    EXPECT_EQ(0, retSize);

    EXPECT_EQ(STATUS_SUCCESS, packageTags(tagsCount, tags, 0, NULL, &retSize));
    EXPECT_EQ(tagsCount * TAG_FULL_LENGTH, retSize);

    EXPECT_EQ(STATUS_SUCCESS, packageTags(tagsCount, tags, retSize, pDstTags, &retSize));

    // The actual storage should be less as our tag name/values are shorter than max
    EXPECT_TRUE(tagsCount * TAG_FULL_LENGTH > retSize);

    // Less than required size might succeed due to alignment.
    // In this case, it will succeed as we have more storage.
    retSize -= 1;
    EXPECT_EQ(STATUS_SUCCESS, packageTags(tagsCount, tags, retSize, pDstTags, &retSize));

    // Now, it should fail as we are 8 bytes less than the storage size returned earlier.
    retSize -= 7;
    EXPECT_NE(STATUS_SUCCESS, packageTags(tagsCount, tags, retSize, pDstTags, &retSize));

    for (i = 0; i < tagsCount; i++) {
        MEMFREE(tags[i].name);
        MEMFREE(tags[i].value);
    }

    MEMFREE(pDstTags);
}
