/** Copyright 2019 Amazon.com. All rights reserved. */

#pragma once

#include "com/amazonaws/kinesis/video/client/Include.h"
#include "CallbackProvider.h"
#include "ClientCallbackProvider.h"
#include "StreamCallbackProvider.h"
#include "GetTime.h"
#include "DefaultCallbackProvider.h"

#include <algorithm>
#include <memory>
#include <thread>
#include <future>
#include <list>
#include <cstdint>
#include <string>
#include <mutex>
#include <chrono>

namespace com { namespace amazonaws { namespace kinesis { namespace video {

const uint64_t DEFAULT_CACHE_UPDATE_PERIOD_IN_SECONDS = 2400;

class CachingEndpointOnlyCallbackProvider : public DefaultCallbackProvider {
public:
    explicit CachingEndpointOnlyCallbackProvider(
            std::unique_ptr <ClientCallbackProvider> client_callback_provider,
            std::unique_ptr <StreamCallbackProvider> stream_callback_provider,
            std::unique_ptr <CredentialProvider> credentials_provider =
                std::make_unique<EmptyCredentialProvider>(),
            const std::string &region = DEFAULT_AWS_REGION,
            const std::string &control_plane_uri = "",
            const std::string &user_agent_name = "",
            const std::string &custom_user_agent = "",
            const std::string &cert_path = "",
            uint64_t cache_update_period = DEFAULT_CACHE_UPDATE_PERIOD_IN_SECONDS);

    virtual ~CachingEndpointOnlyCallbackProvider();

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getCreateStreamCallback()
     */
    CreateStreamFunc getCreateStreamCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getDescribeStreamCallback()
     */
    DescribeStreamFunc getDescribeStreamCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getStreamingEndpointCallback()
     */
    GetStreamingEndpointFunc getStreamingEndpointCallback() override;

    /**
     * @copydoc com::amazonaws::kinesis::video::CallbackProvider::getTagResourceCallback()
     */
    TagResourceFunc getTagResourceCallback() override;

    /**
     * Invoked when the Kinesis Video SDK determines that the stream, defined by the stream_name,
     * does not exist.
     *
     * The handler spawns an async task and makes a network call on that thread of execution to the createStream
     * API. If the HTTP status code returned is not 200, the response body and the HTTP status code is logged
     * and a std::runtime_exception is thrown from the async thread execution context.
     * On successful completion of the createStream call, the createStreamResultEvent() callback is invoked to
     * drive the Kinesis Video SDK state machine into its next state.
     *
     * @param custom_data Custom handle passed by the caller (this class)
     * @param device_name The name of this device (compute ID)
     * @param stream_name The name of this string (<sensor ID>.camera_<stream_tag>)
     * @param kms_arn Optional kms key ARN to be used in encrypting the data at rest.
     * @param retention_period Archival retention period in 100ns. A value of 0 means no retention.
     * @param content_type MIME type (e.g. video/x-matroska)
     * @param service_call_ctx service call context passed from Kinesis Video PIC
     * @return Status of the callback
     */
    static STATUS createStreamHandler(UINT64 custom_data,
                                      PCHAR device_name,
                                      PCHAR stream_name,
                                      PCHAR content_type,
                                      PCHAR kms_arn,
                                      UINT64 retention_period,
                                      PServiceCallContext service_call_ctx);

    /**
     * Invoked when the Kinesis Video SDK to check if the stream, defined by stream_name, exists.
     *
     * The handler spawns an async task and makes a network call on that thread of execution to the
     * describeStream API.
     * On successful completion of the describeStream call, the describeStreamResultEvent() callback is invoked to
     * drive the Kinesis Video SDK state machine into its next state.
     *
     * @param custom_data Custom handle passed by the caller (this class)
     * @param stream_name The name of this string (<sensor ID>.camera_<stream_tag>)
     * @param service_call_ctx service call context passed from Kinesis Video PIC
     * @param service_call_ctx
     * @return Status of the callback
     */
    static STATUS describeStreamHandler(UINT64 custom_data, PCHAR stream_name, PServiceCallContext service_call_ctx);

    /**
     * Gets the streaming endpoint.
     * getStreamingEndpointResultEvent() callback to drive the Kinesis Video SDK state machine into its next state.
     *
     * @param custom_data Custom handle passed by the caller (this class)
     * @param stream_name The name of this string (<sensor ID>.camera_<stream_tag>)
     * @param api_name API name to call (currently PUT_MEDIA)
     * @param service_call_ctx service call context passed from Kinesis Video PIC
     * @return Status of the callback
     */
    static STATUS streamingEndpointHandler(UINT64 custom_data,
                                           PCHAR stream_name,
                                           PCHAR api_name,
                                           PServiceCallContext service_call_ctx);

    /**
     * Tags a stream with a tag.
     * tagResourceResultEvent() callback to drive the Kinesis Video SDK state machine into its next state.
     *
     * @param custom_data Custom handle passed by the caller (this class)
     * @param stream_arn The stream arn
     * @param num_tags Number of tags that follow
     * @param tags The tags array
     * @param service_call_ctx service call context passed from Kinesis Video PIC
     * @return Status of the callback
     */
    static STATUS tagResourceHandler(UINT64 custom_data,
                                     PCHAR stream_arn,
                                     UINT32 num_tags,
                                     PTag tags,
                                     PServiceCallContext service_call_ctx);

protected:

    // Cache update period in seconds
    uint64_t cache_update_period_;

    // Cached value of the endpoint
    std::string streaming_endpoint_;

    // Cache last update time
    std::chrono::system_clock::time_point last_update_time_;

};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
