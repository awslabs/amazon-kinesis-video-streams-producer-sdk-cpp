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

class CachingEndpointOnlyCallbackProvider : public DefaultCallbackProvider {
public:
    explicit CachingEndpointOnlyCallbackProvider(
            std::unique_ptr <ClientCallbackProvider> client_callback_provider,
            std::unique_ptr <StreamCallbackProvider> stream_callback_provider,
            std::unique_ptr <CredentialProvider> credentials_provider,
            const std::string &region,
            const std::string &control_plane_uri,
            const std::string &user_agent_name,
            const std::string &custom_user_agent,
            const std::string &cert_path,
            uint64_t cache_update_period);

    explicit CachingEndpointOnlyCallbackProvider(
            std::unique_ptr <ClientCallbackProvider> client_callback_provider,
            std::unique_ptr <StreamCallbackProvider> stream_callback_provider,
            std::unique_ptr <CredentialProvider> credentials_provider =
            (std::unique_ptr<EmptyCredentialProvider>) new EmptyCredentialProvider(),
            const std::string &region = DEFAULT_AWS_REGION,
            const std::string &control_plane_uri = EMPTY_STRING,
            const std::string &user_agent_name = EMPTY_STRING,
            const std::string &custom_user_agent = EMPTY_STRING,
            const std::string &cert_path = EMPTY_STRING,
            std::chrono::duration<uint64_t> caching_update_period = std::chrono::seconds(DEFAULT_ENDPOINT_CACHE_UPDATE_PERIOD / HUNDREDS_OF_NANOS_IN_A_SECOND));

    virtual ~CachingEndpointOnlyCallbackProvider();
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
