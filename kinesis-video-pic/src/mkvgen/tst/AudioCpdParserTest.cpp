#include "MkvgenTestFixture.h"

class AudioCpdParserTest : public MkvgenTestBase {
};

TEST_F(AudioCpdParserTest, audioCpdParser_InvalidInput)
{
    BYTE cpd[] = {0x00, 0x00};
    UINT32 cpdSize = 2;
    DOUBLE samplingFrequency;
    UINT16 channelConfig;

    EXPECT_NE(STATUS_SUCCESS, getSamplingFreqAndChannelFromAacCpd(NULL, cpdSize, &samplingFrequency, &channelConfig));
    EXPECT_NE(STATUS_SUCCESS, getSamplingFreqAndChannelFromAacCpd(cpd, 0, &samplingFrequency, &channelConfig));
    EXPECT_NE(STATUS_SUCCESS, getSamplingFreqAndChannelFromAacCpd(cpd, MIN_AAC_CPD_SIZE - 1, &samplingFrequency, &channelConfig));
    EXPECT_NE(STATUS_SUCCESS, getSamplingFreqAndChannelFromAacCpd(cpd, cpdSize, NULL, &channelConfig));
    EXPECT_NE(STATUS_SUCCESS, getSamplingFreqAndChannelFromAacCpd(cpd, cpdSize, &samplingFrequency, NULL));
    EXPECT_NE(STATUS_SUCCESS, getSamplingFreqAndChannelFromAacCpd(cpd, cpdSize, NULL, NULL));
}

TEST_F(AudioCpdParserTest, audioCpdParser_AacCpd)
{
    BYTE cpd[] = {0x12, 0x10};
    UINT32 cpdSize = SIZEOF(cpd);
    DOUBLE samplingFrequency;
    UINT16 channelConfig;

    EXPECT_EQ(STATUS_SUCCESS, getSamplingFreqAndChannelFromAacCpd(cpd, cpdSize, &samplingFrequency, &channelConfig));
    EXPECT_EQ((DOUBLE) 44100, samplingFrequency);
    EXPECT_EQ(2, channelConfig);
}

TEST_F(AudioCpdParserTest, audioCpdParser_InvalidCpd)
{
    BYTE cpd[] = {0x12, 0x10};
    UINT32 cpdSize = SIZEOF(cpd);
    DOUBLE samplingFrequency;
    UINT16 channelConfig;

    EXPECT_EQ(STATUS_MKV_INVALID_AAC_CPD, getSamplingFreqAndChannelFromAacCpd(NULL, 2, &samplingFrequency, &channelConfig));
    EXPECT_EQ(STATUS_MKV_INVALID_AAC_CPD, getSamplingFreqAndChannelFromAacCpd(NULL, 0, &samplingFrequency, &channelConfig));
    EXPECT_EQ(STATUS_MKV_INVALID_AAC_CPD, getSamplingFreqAndChannelFromAacCpd(cpd, 0, &samplingFrequency, &channelConfig));
    EXPECT_EQ(STATUS_NULL_ARG, getSamplingFreqAndChannelFromAacCpd(cpd, cpdSize, &samplingFrequency, NULL));
    EXPECT_EQ(STATUS_NULL_ARG, getSamplingFreqAndChannelFromAacCpd(cpd, cpdSize, NULL, &channelConfig));
    EXPECT_EQ(STATUS_NULL_ARG, getSamplingFreqAndChannelFromAacCpd(cpd, cpdSize, NULL, NULL));
}

TEST_F(AudioCpdParserTest, audioCpdParser_invalidSamplingFrequencyIndex)
{
    BYTE cpd[] = {0x07, 0x90}; // sampling frequency index is 15
    UINT32 cpdSize = SIZEOF(cpd);
    DOUBLE samplingFrequency;
    UINT16 channelConfig;

    EXPECT_EQ(STATUS_MKV_INVALID_AAC_CPD_SAMPLING_FREQUENCY_INDEX, getSamplingFreqAndChannelFromAacCpd(cpd, cpdSize, &samplingFrequency, &channelConfig));
}

TEST_F(AudioCpdParserTest, audioCpdParser_invalidChannelConfig)
{
    BYTE cpd[] = {0x07, 0xc8}; // channel config is 9
    UINT32 cpdSize = SIZEOF(cpd);
    DOUBLE samplingFrequency;
    UINT16 channelConfig;

    EXPECT_EQ(STATUS_MKV_INVALID_AAC_CPD_CHANNEL_CONFIG, getSamplingFreqAndChannelFromAacCpd(cpd, cpdSize, &samplingFrequency, &channelConfig));
}

TEST_F(AudioCpdParserTest, audioCpdParser_genreateAudioCpd)
{
    BYTE generatedCpd[2], actualCpd[] = {0x15, 0x90};
    UINT32 cpdSize = SIZEOF(generatedCpd);
    DOUBLE samplingFrequency = 8000;
    UINT16 channelConfig = 2;

    EXPECT_EQ(STATUS_SUCCESS, generateAacCpd(generatedCpd, &cpdSize, AAC_LC, samplingFrequency, channelConfig));
    EXPECT_EQ(0, MEMCMP(generatedCpd, actualCpd, cpdSize));
}

TEST_F(AudioCpdParserTest, audioCpdParser_genreateAudioCpdInvalidFrequency)
{
    BYTE generatedCpd[2];
    UINT32 cpdSize = SIZEOF(generatedCpd);
    DOUBLE samplingFrequency = 2222;
    UINT16 channelConfig = 2;

    EXPECT_EQ(STATUS_INVALID_ARG, generateAacCpd(generatedCpd, &cpdSize, AAC_LC, samplingFrequency, channelConfig));
}

TEST_F(AudioCpdParserTest, audioCpdParser_genreateAudioCpdInvalidChannelConfig)
{
    BYTE generatedCpd[2];
    UINT32 cpdSize = SIZEOF(generatedCpd);
    DOUBLE samplingFrequency = 8000;
    UINT16 channelConfig = 100;

    EXPECT_EQ(STATUS_INVALID_ARG, generateAacCpd(generatedCpd, &cpdSize, AAC_LC, samplingFrequency, channelConfig));
}

TEST_F(AudioCpdParserTest, audioCpdParser_genreateAudioCpdInvalidAudioObjectType)
{
    BYTE generatedCpd[2];
    UINT32 cpdSize = SIZEOF(generatedCpd);
    DOUBLE samplingFrequency = 8000;
    UINT16 channelConfig = 2;

    EXPECT_EQ(STATUS_INVALID_ARG, generateAacCpd(generatedCpd, &cpdSize, (MPEG4_AUDIO_OBJECT_TYPE) 120, samplingFrequency, channelConfig));
}

