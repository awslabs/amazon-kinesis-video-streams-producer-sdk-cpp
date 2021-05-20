#include "ClientTestFixture.h"

class StreamDeviceTagsTest : public ClientTestBase {
};

TEST_F(StreamDeviceTagsTest, createDeviceTagsValid)
{
    UINT32 i;
    CLIENT_HANDLE clientHandle = INVALID_CLIENT_HANDLE_VALUE;
    UINT32 tagCount = MAX_TAG_COUNT;
    PTag tags = (PTag) MEMALLOC(SIZEOF(Tag) * tagCount);
    for(i = 0; i < tagCount; i++) {
        tags[i].version = TAG_CURRENT_VERSION;
        tags[i].name = (PCHAR) MEMALLOC(MAX_TAG_NAME_LEN);
        STRCPY(tags[i].name, "TagName");
        tags[i].value = (PCHAR) MEMALLOC(MAX_TAG_VALUE_LEN);
        STRCPY(tags[i].value, "TagValue");
    }

    mDeviceInfo.tags = tags;
    mDeviceInfo.tagCount = tagCount;

    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, createDeviceResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_DEVICE_ARN));
    EXPECT_EQ(STATUS_SUCCESS, tagResourceResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK));

    // Fixme: The count should be equal when tagging devices is enabled
    EXPECT_EQ(0, mTagCount);

    // Free the tags
    for(i = 0; i < tagCount; i++) {
        MEMFREE(tags[i].name);
        MEMFREE(tags[i].value);
    }

    MEMFREE(tags);

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
}

TEST_F(StreamDeviceTagsTest, createDeviceTagsLongNameInvalid)
{
    UINT32 i;
    CLIENT_HANDLE clientHandle = INVALID_CLIENT_HANDLE_VALUE;
    UINT32 tagCount = MAX_TAG_COUNT;
    PTag tags = (PTag) MEMALLOC(SIZEOF(Tag) * tagCount);

    for(i = 0; i < tagCount; i++) {
        tags[i].version = TAG_CURRENT_VERSION;
        tags[i].name = (PCHAR) MEMALLOC(MAX_TAG_NAME_LEN + 2);
        MEMSET(tags[i].name, 'A', MAX_TAG_NAME_LEN + 1);
        tags[i].name[MAX_TAG_NAME_LEN + 1] = '\0';
        tags[i].value = (PCHAR) MEMALLOC(MAX_TAG_VALUE_LEN + 1);
        MEMSET(tags[i].value, 'B', MAX_TAG_VALUE_LEN);
        tags[i].value[MAX_TAG_VALUE_LEN] = '\0';
    }

    mDeviceInfo.tags = tags;
    mDeviceInfo.tagCount = tagCount;

    EXPECT_NE(STATUS_SUCCESS, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    EXPECT_EQ(0, mTagCount);

    // Free the tags
    for(i = 0; i < tagCount; i++) {
        MEMFREE(tags[i].name);
        MEMFREE(tags[i].value);
    }

    MEMFREE(tags);

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
}

TEST_F(StreamDeviceTagsTest, createDeviceTagsLongValueInvalid)
{
    UINT32 i;
    CLIENT_HANDLE clientHandle = INVALID_CLIENT_HANDLE_VALUE;
    UINT32 tagCount = MAX_TAG_COUNT;
    PTag tags = (PTag) MEMALLOC(SIZEOF(Tag) * tagCount);
    for(i = 0; i < tagCount; i++) {
        tags[i].version = TAG_CURRENT_VERSION;
        tags[i].name = (PCHAR) MEMALLOC(MAX_TAG_NAME_LEN + 1);
        MEMSET(tags[i].name, 'A', MAX_TAG_NAME_LEN);
        tags[i].name[MAX_TAG_NAME_LEN] = '\0';
        tags[i].value = (PCHAR) MEMALLOC(MAX_TAG_VALUE_LEN + 2);
        MEMSET(tags[i].value, 'B', MAX_TAG_VALUE_LEN + 1);
        tags[i].value[MAX_TAG_VALUE_LEN + 1] = '\0';
    }

    mDeviceInfo.tags = tags;
    mDeviceInfo.tagCount = tagCount;

    EXPECT_NE(STATUS_SUCCESS, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    EXPECT_EQ(0, mTagCount);

    // Free the tags
    for(i = 0; i < tagCount; i++) {
        MEMFREE(tags[i].name);
        MEMFREE(tags[i].value);
    }

    MEMFREE(tags);

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
}

TEST_F(StreamDeviceTagsTest, createStreamTagsValid)
{
    UINT32 i;
    UINT32 tagCount = MAX_TAG_COUNT;
    CLIENT_HANDLE clientHandle = INVALID_CLIENT_HANDLE_VALUE;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;

    PTag tags = (PTag) MEMALLOC(SIZEOF(Tag) * tagCount);
    for(i = 0; i < tagCount; i++) {
        tags[i].version = TAG_CURRENT_VERSION;
        tags[i].name = (PCHAR) MEMALLOC(MAX_TAG_NAME_LEN);
        STRCPY(tags[i].name, "TagName");
        tags[i].value = (PCHAR) MEMALLOC(MAX_TAG_VALUE_LEN);
        STRCPY(tags[i].value, "TagValue");
    }

    mStreamInfo.tags = tags;
    mStreamInfo.tagCount = tagCount;

    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, createDeviceResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_DEVICE_ARN));
    EXPECT_EQ(0, mTagCount);
    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoStream(clientHandle, &mStreamInfo, &streamHandle));
    EXPECT_EQ(0, mTagCount);

    // Move the stream to the ready state
    EXPECT_EQ(STATUS_SUCCESS, describeStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESOURCE_NOT_FOUND, NULL));
    EXPECT_EQ(STATUS_SUCCESS, createStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAM_ARN));
    EXPECT_EQ(STATUS_SUCCESS, tagResourceResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK));
    EXPECT_EQ(STATUS_SUCCESS, getStreamingEndpointResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_ENDPOINT));
    EXPECT_EQ(STATUS_SUCCESS, getStreamingTokenResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, (PBYTE) TEST_STREAMING_TOKEN, SIZEOF(TEST_STREAMING_TOKEN), TEST_AUTH_EXPIRATION));

    // Free the tags
    for(i = 0; i < tagCount; i++) {
        MEMFREE(tags[i].name);
        MEMFREE(tags[i].value);
    }

    MEMFREE(tags);

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
}

TEST_F(StreamDeviceTagsTest, createStreamTagsLongNameInvalid)
{
    UINT32 i;
    UINT32 tagCount = MAX_TAG_COUNT;
    CLIENT_HANDLE clientHandle = INVALID_CLIENT_HANDLE_VALUE;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;

    PTag tags = (PTag) MEMALLOC(SIZEOF(Tag) * tagCount);
    for(i = 0; i < tagCount; i++) {
        tags[i].version = TAG_CURRENT_VERSION;
        tags[i].name = (PCHAR) MEMALLOC(MAX_TAG_NAME_LEN + 2);
        MEMSET(tags[i].name, 'A', MAX_TAG_NAME_LEN + 1);
        tags[i].name[MAX_TAG_NAME_LEN + 1] = '\0';
        tags[i].value = (PCHAR) MEMALLOC(MAX_TAG_VALUE_LEN + 1);
        MEMSET(tags[i].value, 'B', MAX_TAG_VALUE_LEN);
        tags[i].value[MAX_TAG_VALUE_LEN] = '\0';
    }

    mStreamInfo.tags = tags;
    mStreamInfo.tagCount = tagCount;

    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, createDeviceResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_DEVICE_ARN));
    EXPECT_EQ(0, mTagCount);
    EXPECT_NE(STATUS_SUCCESS, createKinesisVideoStream(clientHandle, &mStreamInfo, &streamHandle));
    EXPECT_EQ(0, mTagCount);

    // Free the tags
    for(i = 0; i < tagCount; i++) {
        MEMFREE(tags[i].name);
        MEMFREE(tags[i].value);
    }

    MEMFREE(tags);

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
}

TEST_F(StreamDeviceTagsTest, createStreamTagsLongValueInvalid)
{
    UINT32 i;
    UINT32 tagCount = MAX_TAG_COUNT;
    CLIENT_HANDLE clientHandle = INVALID_CLIENT_HANDLE_VALUE;
    STREAM_HANDLE streamHandle = INVALID_STREAM_HANDLE_VALUE;

    PTag tags = (PTag) MEMALLOC(SIZEOF(Tag) * tagCount);
    for(i = 0; i < tagCount; i++) {
        tags[i].version = TAG_CURRENT_VERSION;
        tags[i].name = (PCHAR) MEMALLOC(MAX_TAG_NAME_LEN + 1);
        MEMSET(tags[i].name, 'A', MAX_TAG_NAME_LEN);
        tags[i].name[MAX_TAG_NAME_LEN] = '\0';
        tags[i].value = (PCHAR) MEMALLOC(MAX_TAG_VALUE_LEN + 2);
        MEMSET(tags[i].value, 'B', MAX_TAG_VALUE_LEN + 1);
        tags[i].value[MAX_TAG_VALUE_LEN + 1] = '\0';
    }

    mStreamInfo.tags = tags;
    mStreamInfo.tagCount = tagCount;

    EXPECT_EQ(STATUS_SUCCESS, createKinesisVideoClient(&mDeviceInfo, &mClientCallbacks, &clientHandle));
    EXPECT_EQ(STATUS_SUCCESS, createDeviceResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_DEVICE_ARN));
    EXPECT_EQ(0, mTagCount);
    EXPECT_NE(STATUS_SUCCESS, createKinesisVideoStream(clientHandle, &mStreamInfo, &streamHandle));
    EXPECT_EQ(0, mTagCount);

    // Free the tags
    for(i = 0; i < tagCount; i++) {
        MEMFREE(tags[i].name);
        MEMFREE(tags[i].value);
    }

    MEMFREE(tags);

    EXPECT_EQ(STATUS_SUCCESS, freeKinesisVideoClient(&clientHandle));
}
