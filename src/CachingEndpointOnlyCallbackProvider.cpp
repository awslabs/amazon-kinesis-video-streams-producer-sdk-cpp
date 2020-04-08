/** Copyright 2019 Amazon.com. All rights reserved. */

#include "CachingEndpointOnlyCallbackProvider.h"
#include "Logger.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

LOGGER_TAG("com.amazonaws.kinesis.video");

using std::move;
using std::unique_ptr;
using std::string;

CachingEndpointOnlyCallbackProvider::CachingEndpointOnlyCallbackProvider(
        unique_ptr <ClientCallbackProvider> client_callback_provider,
        unique_ptr <StreamCallbackProvider> stream_callback_provider,
        unique_ptr <CredentialProvider> credentials_provider,
        const string& region,
        const string& control_plane_uri,
        const std::string &user_agent_name,
        const std::string &custom_user_agent,
        const std::string &cert_path,
        uint64_t cache_update_period) :
            cache_update_period_(cache_update_period),
            DefaultCallbackProvider(
                move(client_callback_provider),
                move(stream_callback_provider),
                move(credentials_provider),
                region,
                control_plane_uri,
                user_agent_name,
                custom_user_agent,
                cert_path,
                true,
                cache_update_period) {
}

CachingEndpointOnlyCallbackProvider::~CachingEndpointOnlyCallbackProvider() {
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
