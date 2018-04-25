/** Copyright 2017 Amazon.com. All rights reserved. */

#pragma once

#include "KinesisVideoStream.h"
#include "CallbackProvider.h"
#include "ClientCallbackProvider.h"
#include "StreamCallbackProvider.h"
#include "DefaultCallbackProvider.h"
#include "DefaultDeviceInfoProvider.h"
#include "DeviceInfoProvider.h"
#include "StreamDefinition.h"
#include "Logger.h"
#include "Auth.h"

#include <cstring>

#include <memory>
#include <mutex>
#include <iostream>

#include "com/amazonaws/kinesis/video/client/Include.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

/**
 * Client ready timeout duration.
 **/
#define CLIENT_READY_TIMEOUT_DURATION_IN_SECONDS 15

/**
 * Stream ready timeout duration.
 **/
#define STREAM_READY_TIMEOUT_DURATION_IN_SECONDS 30

/**
* Kinesis Video client interface for real time streaming. The structure of this class is that each instance of type <T,U>
* is a singleton where T is the implementation of the DeviceInfoProvider interface and U is the implementation of the
* CallbackProvider interface. The reason for using the singleton is that Kinesis Video client has logic that benefits
* throughput and network congestion avoidance by sharing state across multiple streams. We balance this benefit with
* flexibility in the implementation of the application logic that provide the device information and Kinesis Video
* user-implemented application callbacks. The division by type allows for easy migrations to new callback
* implementations, such as different network thread implementations or different handling of latency pressure by the
* application without perturbing the existing system.
*
* Example Usage:
* @code:
* auto client(KinesisVideoClient<DeviceInfoProviderImpl, CallbackProviderImpl>::getInstance());
* @endcode
*
*/
class KinesisVideoStream;
class KinesisVideoProducer {
public:
    static std::unique_ptr<KinesisVideoProducer> create(
            std::unique_ptr<DeviceInfoProvider> device_info_provider,
            std::unique_ptr<ClientCallbackProvider> client_callback_provider,
            std::unique_ptr<StreamCallbackProvider> stream_callback_provider,
            std::unique_ptr<CredentialProvider> credential_provider,
            const std::string &region = DEFAULT_AWS_REGION,
            const std::string &control_plane_uri = "");

    static std::unique_ptr<KinesisVideoProducer> create(
            std::unique_ptr<DeviceInfoProvider> device_info_provider,
            std::unique_ptr<CallbackProvider> callback_provider);

    static std::unique_ptr<KinesisVideoProducer> createSync(
            std::unique_ptr<DeviceInfoProvider> device_info_provider,
            std::unique_ptr<ClientCallbackProvider> client_callback_provider,
            std::unique_ptr<StreamCallbackProvider> stream_callback_provider,
            std::unique_ptr<CredentialProvider> credential_provider,
            const std::string &region = DEFAULT_AWS_REGION,
            const std::string &control_plane_uri = "");

    static std::unique_ptr<KinesisVideoProducer> createSync(
            std::unique_ptr<DeviceInfoProvider> device_info_provider,
            std::unique_ptr<CallbackProvider> callback_provider);

    virtual ~KinesisVideoProducer();

    /**
     * Factory method for creating streams to Kinesis Video PIC. The full stream configuration is passed through the
     * stream_definition object which is then used to initialize an KinesisVideoStream instance and return it to the caller.
     *
     * @param stream_definition A shared pointer to the StreamDefinition which describes the
     *                          stream to be created.
     * @return An KinesisVideoStream instance which is ready to start streaming.
     */
    std::shared_ptr<KinesisVideoStream> createStream(std::unique_ptr<StreamDefinition> stream_definition);

    /**
     * Synchronous version of the createStream
     * @param stream_definition A unique pointer to the StreamDefinition which describes the
     *                          stream to be created.
     * @return An KinesisVideoStream instance which is ready to start streaming.
     */
    std::shared_ptr<KinesisVideoStream> createStreamSync(std::unique_ptr<StreamDefinition> stream_definition);

    /**
     * Frees the stream and removes it from the producer stream list.
     *
     * NOTE: This is a prompt operation and will stop the stream immediately without
     * emptying the buffer.
     *
     * @param KinesisVideo_stream A unique pointer to the KinesisVideoStream to free and remove.
     */
    void freeStream(std::shared_ptr<KinesisVideoStream> kinesis_video_stream);

    /**
     * Gets the available storage size
     * @return Available storage size in bytes
     */
    uint64_t getAvailableStorageSize() const;

    /**
     * Returns the raw client handle
     */
    CLIENT_HANDLE getClientHandle() const {
        return client_handle_;
    }

protected:

    /**
     * Frees the resources in the underlying Kinesis Video client.
     */
    void freeKinesisVideoClient() {
        if (nullptr != callback_provider_) {
            callback_provider_->shutdown();
        }

        std::call_once(free_kinesis_video_client_flag_, ::freeKinesisVideoClient, &client_handle_);
    }

    /**
     * Initializes an empty class. The real initialization happens through the static functions.
     */
    KinesisVideoProducer() : client_handle_(INVALID_CLIENT_HANDLE_VALUE),
                             client_ready_(false) {
    }

    /**
     * pointer to the initialized client, stored as a integer value.
     */
    CLIENT_HANDLE client_handle_;

    /**
     * Flag used to ensure idempotency of freeKinesisVideoClient().
     */
    std::once_flag free_kinesis_video_client_flag_;

    /**
     * We keep the reference to callback_provider_ as a field variable
     * to ensure its lifetime is the lifetime of the client because the API
     * is a C only library and doesn't know about managed pointers.
     */
    std::unique_ptr<CallbackProvider> callback_provider_;

    /**
     * Stored callbacks from the client
     */
    ClientCallbacks stored_callbacks_;

    /**
     * Mutex needed for the condition variable for client ready locking.
     */
    std::mutex client_ready_mutex_;

    /**
     * Condition variable used to signal the client being ready.
     */
    std::condition_variable client_ready_var_;

    /**
     * Indicating that the client is ready
     */
    volatile bool client_ready_;

    /**
     * Map of the handle to stream object
     */
    ThreadSafeMap<STREAM_HANDLE, shared_ptr<KinesisVideoStream>> active_streams_;

    /**
     * Callback overrides
     */
    static MUTEX createMutexFunc(UINT64,
                                 BOOL);
    static VOID lockMutexFunc(UINT64,
                              MUTEX);
    static VOID unlockMutexFunc(UINT64,
                                MUTEX);
    static VOID tryLockMutexFunc(UINT64,
                                 MUTEX);
    static VOID freeMutexFunc(UINT64,
                              MUTEX);
    static UINT64 getCurrentTimeFunc(UINT64);
    static UINT32 getRandomNumberFunc(UINT64);
    static VOID logPrintFunc(UINT32, PCHAR, PCHAR, ...);
    static STATUS getSecurityTokenFunc(UINT64,
                                       PBYTE*,
                                       PUINT32,
                                       PUINT64);
    static STATUS getDeviceCertificateFunc(UINT64,
                                           PBYTE*,
                                           PUINT32,
                                           PUINT64);
    static STATUS getDeviceFingerprintFunc(UINT64,
                                           PCHAR*);
    static STATUS streamUnderflowReportFunc(UINT64,
                                            STREAM_HANDLE);
    static STATUS storageOverflowPressureFunc(UINT64,
                                              UINT64);
    static STATUS streamLatencyPressureFunc(UINT64,
                                            STREAM_HANDLE,
                                            UINT64);
    static STATUS droppedFrameReportFunc(UINT64,
                                         STREAM_HANDLE,
                                         UINT64);
    static STATUS droppedFragmentReportFunc(UINT64,
                                            STREAM_HANDLE,
                                            UINT64);
    static STATUS streamErrorReportFunc(UINT64,
                                        STREAM_HANDLE,
                                        UINT64,
                                        STATUS);
    static STATUS streamReadyFunc(UINT64,
                                  STREAM_HANDLE);
    static STATUS streamClosedFunc(UINT64,
                                   STREAM_HANDLE,
                                   UINT64);
    static STATUS createStreamFunc(UINT64,
                                   PCHAR,
                                   PCHAR,
                                   PCHAR,
                                   PCHAR,
                                   UINT64,
                                   PServiceCallContext);
    static STATUS describeStreamFunc(UINT64,
                                     PCHAR,
                                     PServiceCallContext);
    static STATUS getStreamingEndpointFunc(UINT64,
                                           PCHAR,
                                           PCHAR,
                                           PServiceCallContext);
    static STATUS getStreamingTokenFunc(UINT64,
                                        PCHAR,
                                        STREAM_ACCESS_MODE,
                                        PServiceCallContext);
    static STATUS putStreamFunc(UINT64,
                                PCHAR,
                                PCHAR,
                                UINT64,
                                BOOL,
                                BOOL,
                                PCHAR,
                                PServiceCallContext);
    static STATUS tagResourceFunc(UINT64,
                                  PCHAR,
                                  UINT32,
                                  PTag,
                                  PServiceCallContext);
    static STATUS clientReadyFunc(UINT64,
                                  CLIENT_HANDLE);
    static STATUS createDeviceFunc(UINT64,
                                   PCHAR,
                                   PServiceCallContext);
    static STATUS deviceCertToTokenFunc(UINT64,
                                        PCHAR,
                                        PServiceCallContext);
    static STATUS streamDataAvailableFunc(UINT64,
                                          STREAM_HANDLE,
                                          PCHAR,
                                          UINT64,
                                          UINT64,
                                          UINT64);
    static STATUS streamConnectionStaleFunc(UINT64,
                                            STREAM_HANDLE,
                                            UINT64);
    static STATUS fragmentAckReceivedFunc(UINT64,
                                          STREAM_HANDLE,
                                          PFragmentAck);
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
