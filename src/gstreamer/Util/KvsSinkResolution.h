#ifndef __KVS_SINK_RESOLUTION_H__
#define __KVS_SINK_RESOLUTION_H__

#include <string>

// Pure region/credential precedence helpers, intentionally kept in their own dependency-free
// translation unit (only <string>; no GStreamer, no logging, no file-scope statics). This lets the
// unit tests compile this single .cpp into the test binary without pulling in KvsSinkUtil.cpp's
// static initializers (e.g. the log4cplus LOGGER_TAG), which otherwise get constructed twice - once
// in the test executable and once in the dlopened kvssink plugin - and crash at teardown.
namespace kvs_sink_util {

    // Returns whether an AWS environment variable should be consulted. Returns false when ignore_env
    // is true so the corresponding GStreamer property always wins, or when the environment variable is
    // unset. Used for both the region (AWS_DEFAULT_REGION) and credential (e.g. AWS_ACCESS_KEY_ID)
    // env vars.
    //
    // The caller (kvssink) provides these values rather than the function reading them itself, keeping
    // it pure and unit testable. kvssink passes the relevant ignore flag and a snapshot of getenv() for
    // the variable (nullptr if unset).
    bool shouldUseEnvVar(bool ignore_env, const char *env_value);

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

}

#endif //__KVS_SINK_RESOLUTION_H__
