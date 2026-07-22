#ifndef __KVS_SINK_UTIL_H__
#define __KVS_SINK_UTIL_H__

#include <string>
#include <gst/gst.h>
#include <map>
#include <set>
#include <Logger.h>
#include <chrono>

#define IOT_GET_CREDENTIAL_ENDPOINT "endpoint"
#define CERTIFICATE_PATH "cert-path"
#define PRIVATE_KEY_PATH "key-path"
#define CA_CERT_PATH "ca-path"
#define ROLE_ALIASES "role-aliases"
#define IOT_THING_NAME "iot-thing-name"
#define IOT_CONNECTION_TIMEOUT "connection-timeout"
#define IOT_COMPLETION_TIMEOUT "completion-timeout"

#define KVSSINK_THROW_IF_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            throw std::runtime_error(std::string(INTERNAL_CHECK_PREFIX) + " " #ptr " is unexpectedly NULL!"); \
        } \
    } while (0)

namespace kvs_sink_util{

    gboolean gstructToMap(GstStructure *g_struct, std::map<std::string, std::string> *user_map);

    gboolean parseIotCredentialGstructure(GstStructure *g_struct,
                                             std::map<std::string, std::string> &iot_cert_params);

    // Resolves the effective AWS region. The lookup order is:
    //   1. AWS_DEFAULT_REGION environment variable (unless ignore_env is true).
    //   2. aws-region property.
    //   3. us-west-2 default (carried by the aws-region property, which defaults to DEFAULT_REGION).
    // When ignore_env is true, step 1 is skipped so the aws-region property always wins.
    //
    // The caller (kvssink) provides these values rather than this function reading them itself. This
    // keeps the function pure (no getenv / no dependency on the GstKvsSink struct), so the full
    // property/env/ignore matrix can be unit tested without mutating the process environment.
    std::string resolveRegion(const std::string &property_region, const char *env_region, bool ignore_env);

    // Returns whether a credential-related environment variable (e.g. AWS_ACCESS_KEY_ID) should be
    // consulted. Returns false when ignore_env is true so the corresponding GStreamer property always
    // wins, or when the environment variable is unset.
    //
    // As with resolveRegion, the caller (kvssink) provides these values rather than the function
    // reading them itself, keeping it pure and unit testable. kvssink passes kvssink->ignore_credentials_env
    // and a snapshot of getenv() for the relevant credential variable (nullptr if unset).
    bool shouldUseCredentialsEnv(bool ignore_env, const char *env_value);

}

#endif //__KVS_SINK_UTIL_H__