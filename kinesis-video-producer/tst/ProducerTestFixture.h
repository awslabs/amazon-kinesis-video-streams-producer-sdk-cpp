#if 0
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

#include "gtest/gtest.h"
#include "KinesisVideoProducer.h"
#include "DefaultDeviceInfoProvider.h"
#include "ClientCallbackProvider.h"
#include "StreamCallbackProvider.h"
#include "Auth.h"
#include "StreamDefinition.h"
#include "CachingEndpointOnlyCallbackProvider.h"
#include "Logger.h"

#include <atomic>
#include <map>

using namespace std;
using namespace std::chrono;

namespace com { namespace amazonaws { namespace kinesis { namespace video {

LOGGER_TAG("com.amazonaws.kinesis.video.TEST");

#define TEST_FRAME_DURATION                                 (40 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)
#define TEST_EXECUTION_DURATION                             (120 * HUNDREDS_OF_NANOS_IN_A_SECOND)
#define TEST_STREAM_COUNT                                   1
#define TEST_FRAME_SIZE                                     1000
#define TEST_STREAMING_TOKEN_DURATION_IN_SECONDS            160 // need to be longer than TEST_EXECUTION_DURATION
#define TEST_STORAGE_SIZE_IN_BYTES                          64 * 1024 * 1024ull
#define TEST_MAX_STREAM_LATENCY_IN_MILLIS                   60000
#define TEST_START_STOP_ITERATION_COUT                      200

#define TEST_FPS                                            20
#define TEST_MEDIA_DURATION_SECONDS                         60
#define TEST_TOTAL_FRAME_COUNT                              TEST_FPS * TEST_MEDIA_DURATION_SECONDS // 1 minutes of frames
#define TEST_TIMESTAMP_SENTINEL                             -1

#define TEST_MAGIC_NUMBER                                   0x1234abcd

// Forward declaration
class ProducerTestBase;

class TestClientCallbackProvider : public ClientCallbackProvider {
public:
    TestClientCallbackProvider(ProducerTestBase *producer_test_base) : producer_test_base_(producer_test_base) {}

    UINT64 getCallbackCustomData() override {
        return reinterpret_cast<UINT64> (this);
    }

    StorageOverflowPressureFunc getStorageOverflowPressureCallback() override {
        return storageOverflowPressure;
    }

    ProducerTestBase* getTestBase() {
        return producer_test_base_;
    }

    static STATUS storageOverflowPressure(UINT64 custom_handle, UINT64 remaining_bytes);
private:
    ProducerTestBase* producer_test_base_;
};

class TestStreamCallbackProvider : public StreamCallbackProvider {
public:
    TestStreamCallbackProvider(ProducerTestBase* producer_test_base) {
        producer_test_base_ = producer_test_base;
    }

    UINT64 getCallbackCustomData() override {
        return reinterpret_cast<UINT64> (this);
    }

    StreamConnectionStaleFunc getStreamConnectionStaleCallback() override {
        return streamConnectionStaleHandler;
    };

    StreamErrorReportFunc getStreamErrorReportCallback() override {
        return streamErrorReportHandler;
    };

    DroppedFrameReportFunc getDroppedFrameReportCallback() override {
        return droppedFrameReportHandler;
    };

    StreamClosedFunc getStreamClosedCallback() override {
        return streamClosedHandler;
    }

    StreamLatencyPressureFunc getStreamLatencyPressureCallback() override {
        return streamLatencyPressureHandler;
    }

    FragmentAckReceivedFunc getFragmentAckReceivedCallback() override {
        return fragmentAckReceivedHandler;
    }

    StreamDataAvailableFunc getStreamDataAvailableCallback() override {
        return streamDataAvailableHandler;
    }

    BufferDurationOverflowPressureFunc getBufferDurationOverflowPressureCallback() override {
        return bufferDurationOverflowPressureHandler;
    }

    ProducerTestBase* getTestBase() {
        return producer_test_base_;
    }

    UINT64 getTestMagicNumber() {
        // This function will return a pre-set 64 bit number which will be used in
        // the callback to validate the that we landed on the correct object.
        // If we didn't land on the correct object then it will likely result in
        // a memory access fault so this is a rather simple check.
        return TEST_MAGIC_NUMBER;
    }

private:
    static STATUS streamConnectionStaleHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 last_buffering_ack);
    static STATUS streamErrorReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UPLOAD_HANDLE upload_handle, UINT64 errored_timecode, STATUS status);
    static STATUS droppedFrameReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 dropped_frame_timecode);
    static STATUS streamClosedHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 stream_upload_handle);
    static STATUS streamLatencyPressureHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 duration);
    static STATUS fragmentAckReceivedHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UPLOAD_HANDLE upload_handle, PFragmentAck fragment_ack);
    static STATUS streamDataAvailableHandler(UINT64 custom_data,
                                             STREAM_HANDLE stream_handle,
                                             PCHAR stream_name,
                                             UPLOAD_HANDLE stream_upload_handle,
                                             UINT64 duration_available,
                                             UINT64 size_available);
    static STATUS bufferDurationOverflowPressureHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UINT64 remaining_duration);

    static STATUS validateCallback(UINT64 custom_data);

    ProducerTestBase* producer_test_base_;
};

class TestDeviceInfoProvider : public DefaultDeviceInfoProvider {
    uint64_t device_storage_size_;
public:
    TestDeviceInfoProvider(uint64_t device_storage_size): device_storage_size_(device_storage_size) {}

    device_info_t getDeviceInfo() override {
        auto device_info = DefaultDeviceInfoProvider::getDeviceInfo();
        device_info.storageInfo.storageSize = (UINT64) device_storage_size_;
        device_info.streamCount = TEST_STREAM_COUNT;
        return device_info;
    }
};

extern ProducerTestBase* gProducerApiTest;

class TestCredentialProvider : public StaticCredentialProvider {
    // Test rotation period is 40 second for the grace period.
    const std::chrono::duration<uint64_t> ROTATION_PERIOD = std::chrono::seconds(TEST_STREAMING_TOKEN_DURATION_IN_SECONDS);
public:
    TestCredentialProvider(const Credentials& credentials, uint32_t rotation_period_seconds) :
            StaticCredentialProvider(credentials), ROTATION_PERIOD(std::chrono::seconds(rotation_period_seconds)) {}

    void updateCredentials(Credentials& credentials) override {
        // Copy the stored creds forward
        credentials = credentials_;

        // Update only the expiration
        auto now_time = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch());
        auto expiration_seconds = now_time + ROTATION_PERIOD;
        credentials.setExpiration(std::chrono::seconds(expiration_seconds.count()));
        LOG_INFO("New credentials expiration is " << credentials.getExpiration().count());
    }
};

class ProducerTestBase : public ::testing::Test {
public:
    ProducerTestBase() : producer_thread_(0),
                         start_producer_(false),
                         stop_called_(false),
                         frame_dropped_(false),
                         stop_producer_(false),
                         access_key_set_(true),
                         buffer_duration_pressure_(false),
                         defaultRegion_(DEFAULT_AWS_REGION),
                         caCertPath_(""),
                         error_status_(STATUS_SUCCESS),
                         device_storage_size_(TEST_STORAGE_SIZE_IN_BYTES),
                         fps_(TEST_FPS),
                         total_frame_count_(TEST_TOTAL_FRAME_COUNT),
                         current_pressure_state_(OK),
                         buffering_ack_in_sequence_(true),
                         key_frame_interval_(TEST_FPS),
                         token_rotation_seconds_(TEST_STREAMING_TOKEN_DURATION_IN_SECONDS) {

        // Set the global to this object so we won't need to allocate structures in the heap
        gProducerApiTest = this;
        frame_duration_ = 1000LLU * HUNDREDS_OF_NANOS_IN_A_MILLISECOND / fps_;
    }

    PVOID basicProducerRoutine(KinesisVideoStream*, STREAMING_TYPE streaming_type = STREAMING_TYPE_REALTIME);

    void freeStreams() {
        stop_producer_ = true;

        // It's also easy to call kinesis_video_producer_->freeStreams();
        // to free all streams instead of iterating over each one and freeing it.

        for (uint32_t i = 0; i < TEST_STREAM_COUNT; i++) {
            LOG_DEBUG("Freeing stream " << streams_[i]->getStreamName());

            // Freeing the stream
            kinesis_video_producer_->freeStream(move(streams_[i]));
            streams_[i] = nullptr;
        }
    }

    void setErrorStatus(STATUS error_status) {
        error_status_ = (uint32_t) error_status;
    }

    STATUS getErrorStatus() {
        return (STATUS) error_status_.load();
    }

    uint64_t getFragmentDurationMs() {
        return key_frame_interval_ * frame_duration_ / HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    }

    atomic_bool stop_called_;
    atomic_bool frame_dropped_;
    atomic_bool buffer_duration_pressure_;
    atomic_bool storage_overflow_;
    atomic_bool buffering_ack_in_sequence_;
    atomic_uint error_status_;
    map<UPLOAD_HANDLE, uint64_t> previous_buffering_ack_timestamp_;

protected:

    void handlePressure(atomic_bool &pressure_flag, uint32_t grace_period_seconds) {
        // first time pressure is detected
        if (pressure_flag && current_pressure_state_ == OK) {
            current_pressure_state_ = PRESSURING; // update state
            pressure_flag = false;  // turn off pressure flag as it is handled

            // whether to give some extra time for the pressure to relieve. For example, storageOverflow takes longer
            // to recover as it needs to wait for persisted acks.
            if (grace_period_seconds != 0){
                THREAD_SLEEP(grace_period_seconds * HUNDREDS_OF_NANOS_IN_A_SECOND);
            }

            // record the time until which we will remain in pressured state.
            pressure_cooldown_time_ = (std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()) + std::chrono::seconds(1)).count();
        } else if (current_pressure_state_ == PRESSURING) { // if we are already in pressured state
            int64_t now_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            // it's time to check whether the pressure has been relieved
            if (now_time >= pressure_cooldown_time_) {
                if (pressure_flag) { // still getting the pressure signal, remain in pressured state.
                    pressure_cooldown_time_ = (std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()) + std::chrono::seconds(1)).count();
                    pressure_flag = false;
                } else { // pressure is relieved, back to OK state.
                    current_pressure_state_ = OK;
                }
            } else { // else reset the signal variable and keep waiting
                pressure_flag = false;
            }
        }
    }

    void CreateCredentialProvider() {
        // Read the credentials from the environmental variables if defined. Use defaults if not.
        char const *accessKey;
        char const *secretKey;
        char const *sessionToken;
        char const *defaultRegion;
        char const *caCertPath;
        string sessionTokenStr;
        if (nullptr == (accessKey = getenv(ACCESS_KEY_ENV_VAR))) {
            accessKey = "AccessKey";
            access_key_set_ = false;
        }

        if (nullptr == (secretKey = getenv(SECRET_KEY_ENV_VAR))) {
            secretKey = "SecretKey";
        }

        if (nullptr == (sessionToken = getenv(SESSION_TOKEN_ENV_VAR))) {
            sessionTokenStr = "";
        } else {
            sessionTokenStr = string(sessionToken);
        }

        if (nullptr != (defaultRegion = getenv(DEFAULT_REGION_ENV_VAR))) {
            defaultRegion_ = string(defaultRegion);
        }

        if (nullptr != (caCertPath = getenv(CACERT_PATH_ENV_VAR))) {
            caCertPath_ = string(caCertPath);
        }

        credentials_.reset(new Credentials(string(accessKey),
                string(secretKey),
                sessionTokenStr,
                std::chrono::seconds(TEST_STREAMING_TOKEN_DURATION_IN_SECONDS)));

        credential_provider_.reset(new TestCredentialProvider(*credentials_.get(), token_rotation_seconds_));
    }


    void CreateProducer(bool cachingEndpoingProvider = false) {
        // Create the producer client
        CreateCredentialProvider();
        device_provider_.reset(new TestDeviceInfoProvider(device_storage_size_));
        client_callback_provider_.reset(new TestClientCallbackProvider(this));
        stream_callback_provider_.reset(new TestStreamCallbackProvider(this));

        try {
            unique_ptr<DefaultCallbackProvider> defaultCallbackProvider;
            if (cachingEndpoingProvider) {
                defaultCallbackProvider.reset(new CachingEndpointOnlyCallbackProvider(
                        move(client_callback_provider_),
                        move(stream_callback_provider_),
                        move(credential_provider_),
                        defaultRegion_,
                        "",
                        "",
                        "",
                        caCertPath_,
                        DEFAULT_CACHE_UPDATE_PERIOD_IN_SECONDS));
            } else {
                defaultCallbackProvider.reset(new DefaultCallbackProvider(
                        move(client_callback_provider_),
                        move(stream_callback_provider_),
                        move(credential_provider_),
                        defaultRegion_,
                        "",
                        "",
                        "",
                        caCertPath_));
            }

            // testDefaultCallbackProvider = reinterpret_cast<TestDefaultCallbackProvider *>(defaultCallbackProvider.get());
            kinesis_video_producer_ = KinesisVideoProducer::createSync(move(device_provider_),
                                                                       move(defaultCallbackProvider));
        } catch (std::runtime_error) {
            ASSERT_TRUE(false) << "Failed creating kinesis video producer";
        }
    };

    shared_ptr<KinesisVideoStream> CreateTestStream(int index,
                                                    STREAMING_TYPE streaming_type = STREAMING_TYPE_REALTIME,
                                                    uint32_t max_stream_latency_ms = TEST_MAX_STREAM_LATENCY_IN_MILLIS,
                                                    int buffer_duration_seconds = 120) {
        char stream_name[MAX_STREAM_NAME_LEN];
        sprintf(stream_name, "ScaryTestStream_%d", index);
        map<string, string> tags;
        char tag_name[MAX_TAG_NAME_LEN];
        char tag_val[MAX_TAG_VALUE_LEN];
        for (int i = 0; i < 5; i++) {
            sprintf(tag_name, "testTag_%d_%d", index, i);
            sprintf(tag_val, "testTag_%d_%d_Value", index, i);

            tags.emplace(std::make_pair(tag_name, tag_val));
        }

        unique_ptr<StreamDefinition> stream_definition(new StreamDefinition(stream_name,
                hours(2),
                &tags,
                "",
                streaming_type,
                "video/h264",
                milliseconds(max_stream_latency_ms),
                seconds(2),
                milliseconds(1),
                true,
                true,
                true,
                true,
                true,
                true,
                0,
                25,
                4 * 1024 * 1024,
                seconds(buffer_duration_seconds),
                seconds(buffer_duration_seconds),
                seconds(50)));
        return kinesis_video_producer_->createStreamSync(move(stream_definition));
    };

    virtual void SetUp() {
        LOG_INFO("Setting up test: " << GetTestName());
    };

    virtual void TearDown() {
        auto producer_ptr = kinesis_video_producer_.release();
        delete producer_ptr;
        previous_buffering_ack_timestamp_.clear();
        buffering_ack_in_sequence_ = true;
        frame_dropped_ = false;
        LOG_INFO("Tearing down test: " << GetTestName());
    };

    PCHAR GetTestName() {
        return (PCHAR) ::testing::UnitTest::GetInstance()->current_test_info()->test_case_name();
    };

    void setFps(uint64_t fps) {
        fps_ = fps;
        frame_duration_ = 1000LLU * HUNDREDS_OF_NANOS_IN_A_MILLISECOND / fps_;
    }

    unique_ptr<KinesisVideoProducer> kinesis_video_producer_;
    unique_ptr<DeviceInfoProvider> device_provider_;
    unique_ptr<ClientCallbackProvider> client_callback_provider_;
    unique_ptr<StreamCallbackProvider> stream_callback_provider_;
    unique_ptr<Credentials> credentials_;
    unique_ptr<CredentialProvider> credential_provider_;

    string defaultRegion_;
    string caCertPath_;

    bool access_key_set_;

    TID producer_thread_;
    volatile bool start_producer_;
    volatile bool stop_producer_;
    volatile bool producer_stopped_;

    uint32_t token_rotation_seconds_;
    uint64_t device_storage_size_;
    uint64_t fps_;
    uint64_t total_frame_count_;
    uint64_t frame_duration_; // in hundreds of nanos
    uint64_t key_frame_interval_;
    enum PressureState {OK, PRESSURING};
    PressureState current_pressure_state_;
    int64_t pressure_cooldown_time_;

    BYTE frameBuffer_[TEST_FRAME_SIZE];
    shared_ptr<KinesisVideoStream> streams_[TEST_STREAM_COUNT];
};

}  // namespace video
}  // namespace kinesis
}  // namespace amazonaws
}  // namespace com;
