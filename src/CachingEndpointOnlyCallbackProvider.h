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
            (std::unique_ptr<EmptyCredentialProvider>) new EmptyCredentialProvider(),
            const std::string &region = DEFAULT_AWS_REGION,
            const std::string &control_plane_uri = "",
            const std::string &user_agent_name = "",
            const std::string &custom_user_agent = "",
            const std::string &cert_path = "",
            uint64_t cache_update_period = DEFAULT_CACHE_UPDATE_PERIOD_IN_SECONDS);

    virtual ~CachingEndpointOnlyCallbackProvider();

protected:

    // Cache update period in seconds
    uint64_t cache_update_period_;
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
