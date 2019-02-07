#include "FileOutputCallbackProvider.h"
#include <chrono>
#include <thread>
#include <Logger.h>

#define TEST_TOKEN_BITS                 "Test token bits"
#define TEST_AUTH_EXPIRATION            (UINT64)(-1LL)
#define TEST_DEVICE_ARN                 "TestDeviceARN"
#define TEST_STREAMING_TOKEN            "TestStreamingToken"
#define TEST_STREAMING_ENDPOINT         "http://test.com/test_endpoint"
#define TEST_DEVICE_NAME                "Test device name"
#define TEST_STREAM_NAME                "Test stream name"
#define TEST_CONTENT_TYPE               "TestType"
#define TEST_CODEC_ID                   "TestCodec"
#define TEST_TRACK_NAME                 "TestTrack"
#define TEST_STREAM_ARN                 "TestStreamARN"
#define TEST_UPDATE_VERSION             "TestUpdateVer"
#define DEFAULT_BUFFER_SIZE             32 * 1024
#define DEFAULT_DUMP_FILENAME           "debug_file.mkv"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

LOGGER_TAG("com.amazonaws.kinesis.video");
using namespace std;
using std::string;

FileOutputCallbackProvider::FileOutputCallbackProvider() {
    buffer_ = new uint8_t[DEFAULT_BUFFER_SIZE];
    debug_dump_file_stream_.open(DEFAULT_DUMP_FILENAME, ios::out | ios::trunc | ios::binary);
}

FileOutputCallbackProvider::~FileOutputCallbackProvider() {
    delete [] buffer_;
    debug_dump_file_stream_.close();
}

void FileOutputCallbackProvider::shutdownStream(STREAM_HANDLE stream_handle) {
    // no op
}

STATUS FileOutputCallbackProvider::clientReadyHandler(UINT64 custom_data, CLIENT_HANDLE client_handle) {
    return STATUS_SUCCESS;
}

STATUS FileOutputCallbackProvider::storageOverflowPressureHandler(UINT64 custom_data, UINT64 bytes_remaining) {
    return STATUS_SUCCESS;
}

STATUS FileOutputCallbackProvider::streamUnderflowReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle) {
    return STATUS_SUCCESS;
}

STATUS FileOutputCallbackProvider::streamLatencyPressureHandler(UINT64 custom_data,
                                    STREAM_HANDLE stream_handle,
                                    UINT64 buffer_duration) {
    return STATUS_SUCCESS;
}

STATUS FileOutputCallbackProvider::droppedFrameReportHandler(UINT64 custom_data,
                                 STREAM_HANDLE stream_handle,
                                 UINT64 timecode) {
    return STATUS_SUCCESS;
}

STATUS FileOutputCallbackProvider::droppedFragmentReportHandler(UINT64 custom_data,
                                    STREAM_HANDLE stream_handle,
                                    UINT64 timecode) {
    return STATUS_SUCCESS;
}

STATUS FileOutputCallbackProvider::streamConnectionStaleHandler(UINT64 custom_data,
                                    STREAM_HANDLE stream_handle,
                                    UINT64 last_ack_duration) {
    return STATUS_SUCCESS;
}

STATUS FileOutputCallbackProvider::streamReadyHandler(UINT64 custom_data, STREAM_HANDLE stream_handle) {
    return STATUS_SUCCESS;
}

STATUS FileOutputCallbackProvider::fragmentAckReceivedHandler(UINT64 custom_data,
                                  STREAM_HANDLE stream_handle,
                                  UPLOAD_HANDLE upload_handle,
                                  PFragmentAck fragment_ack) {
    return STATUS_SUCCESS;
}

STATUS FileOutputCallbackProvider::getSecurityTokenHandler(UINT64 custom_data, PBYTE *buffer, PUINT32 size, PUINT64 expiration) {
    *buffer = (PBYTE) TEST_TOKEN_BITS;
    *size = SIZEOF(TEST_TOKEN_BITS);
    *expiration = TEST_AUTH_EXPIRATION;

    return STATUS_SUCCESS;
}

STATUS FileOutputCallbackProvider::createStreamHandler(UINT64 custom_data,
                           PCHAR device_name,
                           PCHAR stream_name,
                           PCHAR content_type,
                           PCHAR kms_arn,
                           UINT64 retention_period,
                           PServiceCallContext service_call_ctx) {
    auto async_call = [](UINT64 custom_data) -> auto {
        this_thread::sleep_for(chrono::seconds(2));
        createStreamResultEvent(custom_data,
                                SERVICE_CALL_RESULT_OK,
                                TEST_STREAM_ARN);
    };
    thread worker(async_call, service_call_ctx->customData);
    worker.detach();
    return STATUS_SUCCESS;
}

STATUS FileOutputCallbackProvider::describeStreamHandler(UINT64 custom_data, PCHAR stream_name, PServiceCallContext service_call_ctx) {
    auto async_call = [](UINT64 custom_data) -> auto {
        this_thread::sleep_for(chrono::seconds(2));
        StreamDescription stream_description;
        stream_description.version = STREAM_DESCRIPTION_CURRENT_VERSION;
        STRCPY(stream_description.deviceName, TEST_DEVICE_NAME);
        STRCPY(stream_description.streamName, TEST_STREAM_NAME);
        STRCPY(stream_description.contentType, TEST_CONTENT_TYPE);
        STRCPY(stream_description.updateVersion, TEST_UPDATE_VERSION);
        STRCPY(stream_description.streamArn, TEST_STREAM_ARN);
        stream_description.streamStatus = STREAM_STATUS_ACTIVE;
        stream_description.creationTime = GETTIME();

        describeStreamResultEvent(custom_data,
                                  SERVICE_CALL_RESULT_OK,
                                  &stream_description);
    };
    thread worker(async_call, service_call_ctx->customData);
    worker.detach();
    return STATUS_SUCCESS;
}

STATUS FileOutputCallbackProvider::streamingEndpointHandler(UINT64 custom_data,
                                PCHAR stream_name,
                                PCHAR api_name,
                                PServiceCallContext service_call_ctx) {
    auto async_call = [](UINT64 custom_data) -> auto {
        this_thread::sleep_for(chrono::seconds(2));
        getStreamingEndpointResultEvent(custom_data,
                                        SERVICE_CALL_RESULT_OK,
                                        TEST_STREAMING_ENDPOINT);
    };
    thread worker(async_call, service_call_ctx->customData);
    worker.detach();
    return STATUS_SUCCESS;
}

STATUS FileOutputCallbackProvider::streamingTokenHandler(UINT64 custom_data,
                             PCHAR stream_name,
                             STREAM_ACCESS_MODE access_mode,
                             PServiceCallContext service_call_ctx) {
    auto async_call = [](UINT64 custom_data) -> auto {
        this_thread::sleep_for(chrono::seconds(2));
        getStreamingTokenResultEvent(
                custom_data, SERVICE_CALL_RESULT_OK,
                (PBYTE) TEST_STREAMING_TOKEN,
                SIZEOF(TEST_STREAMING_TOKEN),
                TEST_AUTH_EXPIRATION);
    };
    thread worker(async_call, service_call_ctx->customData);
    worker.detach();
    return STATUS_SUCCESS;
}

STATUS
FileOutputCallbackProvider::putStreamHandler(UINT64 custom_data,
                 PCHAR stream_name,
                 PCHAR container_type,
                 UINT64 start_timestamp,
                 BOOL absolute_fragment_timestamp,
                 BOOL do_ack,
                 PCHAR streaming_endpoint,
                 PServiceCallContext service_call_ctx) {
    auto this_obj = reinterpret_cast<FileOutputCallbackProvider *>(custom_data);
    UPLOAD_HANDLE upload_handle = this_obj->getUploadHandle();

    auto async_call = [](UPLOAD_HANDLE upload_handle, UINT64 custom_data) -> auto {
        putStreamResultEvent(custom_data, SERVICE_CALL_RESULT_OK, upload_handle);
    };

    thread worker(async_call, upload_handle, service_call_ctx->customData);
    worker.detach();

    return STATUS_SUCCESS;
}

STATUS FileOutputCallbackProvider::tagResourceHandler(UINT64 custom_data,
                          PCHAR stream_arn,
                          UINT32 num_tags,
                          PTag tags,
                          PServiceCallContext service_call_ctx) {
    auto async_call = [](UINT64 custom_data) -> auto {
        this_thread::sleep_for(chrono::seconds(2));
        tagResourceResultEvent(custom_data, SERVICE_CALL_RESULT_OK);
    };
    thread worker(async_call, service_call_ctx->customData);
    worker.detach();

    return STATUS_SUCCESS;
}

STATUS
FileOutputCallbackProvider::createDeviceHandler(UINT64 custom_data, PCHAR device_name, PServiceCallContext service_call_ctx) {
    auto async_call = [](UINT64 custom_data) -> auto {
        this_thread::sleep_for(chrono::seconds(2));
        createDeviceResultEvent(custom_data, SERVICE_CALL_RESULT_OK,
                                TEST_DEVICE_ARN);
    };
    thread worker(async_call, service_call_ctx->customData);
    worker.detach();

    return STATUS_SUCCESS;
}

STATUS
FileOutputCallbackProvider::streamErrorHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UPLOAD_HANDLE upload_handle, UINT64 fragment_timecode, STATUS status) {
    return STATUS_SUCCESS;
}

STATUS FileOutputCallbackProvider::streamDataAvailableHandler(UINT64 custom_data,
                                                              STREAM_HANDLE stream_handle,
                                                              PCHAR stream_name,
                                                              UPLOAD_HANDLE stream_upload_handle,
                                                              UINT64 duration_available,
                                                              UINT64 size_available) {
    STATUS ret_status;
    UINT64 client_stream_handle = 0;
    UINT32 buffer_size = DEFAULT_BUFFER_SIZE, retrieved_size;
    auto this_obj = reinterpret_cast<FileOutputCallbackProvider *>(custom_data);
    do {
        ret_status = getKinesisVideoStreamData(
                stream_handle,
                stream_upload_handle,
                reinterpret_cast<PBYTE>(this_obj->buffer_),
                (UINT32)buffer_size,
                &retrieved_size);
        LOG_DEBUG("write " << retrieved_size << " to " << DEFAULT_DUMP_FILENAME);
        this_obj->debug_dump_file_stream_.write(reinterpret_cast<char *>(this_obj->buffer_), (size_t) retrieved_size);

    } while (ret_status != STATUS_NO_MORE_DATA_AVAILABLE);

    return STATUS_SUCCESS;
}

STATUS FileOutputCallbackProvider::streamClosedHandler(UINT64 custom_data,
                           STREAM_HANDLE stream_handle,
                           UPLOAD_HANDLE stream_upload_handle) {
    return STATUS_SUCCESS;
}

GetCurrentTimeFunc FileOutputCallbackProvider::getCurrentTimeCallback() {
    return nullptr;
}

DroppedFragmentReportFunc FileOutputCallbackProvider::getDroppedFragmentReportCallback() {
    return droppedFragmentReportHandler;
}

StreamReadyFunc FileOutputCallbackProvider::getStreamReadyCallback() {
    return streamReadyHandler;
}

StreamClosedFunc FileOutputCallbackProvider::getStreamClosedCallback() {
    return streamClosedHandler;
}

FragmentAckReceivedFunc FileOutputCallbackProvider::getFragmentAckReceivedCallback() {
    return fragmentAckReceivedHandler;
}

CreateStreamFunc FileOutputCallbackProvider::getCreateStreamCallback() {
    return createStreamHandler;
}

DescribeStreamFunc FileOutputCallbackProvider::getDescribeStreamCallback() {
    return describeStreamHandler;
}

GetStreamingEndpointFunc FileOutputCallbackProvider::getStreamingEndpointCallback() {
    return streamingEndpointHandler;
}

GetStreamingTokenFunc FileOutputCallbackProvider::getStreamingTokenCallback() {
    return streamingTokenHandler;
}

PutStreamFunc FileOutputCallbackProvider::getPutStreamCallback() {
    return putStreamHandler;
}

TagResourceFunc FileOutputCallbackProvider::getTagResourceCallback() {
    return tagResourceHandler;
}

GetSecurityTokenFunc FileOutputCallbackProvider::getSecurityTokenCallback() {
    return getSecurityTokenHandler;
}

StreamUnderflowReportFunc FileOutputCallbackProvider::getStreamUnderflowReportCallback() {
    return streamUnderflowReportHandler;
}

StorageOverflowPressureFunc FileOutputCallbackProvider::getStorageOverflowPressureCallback() {
    return storageOverflowPressureHandler;
}

StreamLatencyPressureFunc FileOutputCallbackProvider::getStreamLatencyPressureCallback() {
    return streamLatencyPressureHandler;
}

DroppedFrameReportFunc FileOutputCallbackProvider::getDroppedFrameReportCallback() {
    return droppedFrameReportHandler;
}

StreamErrorReportFunc FileOutputCallbackProvider::getStreamErrorReportCallback() {
    return streamErrorHandler;
}

GetDeviceCertificateFunc FileOutputCallbackProvider::getDeviceCertificateCallback() {
    // we are using a security token, so this callback should be null.
    return nullptr;
}

GetDeviceFingerprintFunc FileOutputCallbackProvider::getDeviceFingerprintCallback() {
    // we are using a security token, so this callback should be null.
    return nullptr;
}

ClientReadyFunc FileOutputCallbackProvider::getClientReadyCallback() {
    return clientReadyHandler;
}

CreateDeviceFunc FileOutputCallbackProvider::getCreateDeviceCallback() {
    return createDeviceHandler;
}

DeviceCertToTokenFunc FileOutputCallbackProvider::getDeviceCertToTokenCallback() {
    // We are using a security token, so this callback should be null.
    return nullptr;
}

StreamDataAvailableFunc FileOutputCallbackProvider::getStreamDataAvailableCallback() {
    return streamDataAvailableHandler;
}

StreamConnectionStaleFunc FileOutputCallbackProvider::getStreamConnectionStaleCallback() {
    return streamConnectionStaleHandler;
}

}
}
}
}
