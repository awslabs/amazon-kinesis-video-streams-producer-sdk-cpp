#include "KvsSinkResolution.h"

namespace kvs_sink_util {

bool shouldUseEnvVar(bool ignore_env, const char *env_value) {
    return !ignore_env && nullptr != env_value;
}

std::string resolveRegion(const std::string &property_region, const char *env_region, bool ignore_env) {
    // By default, the AWS_DEFAULT_REGION env var overrides the aws-region property. When ignore_env is
    // set, the env var is ignored and the aws-region property is always used.
    // The aws-region property has a default value of DEFAULT_REGION.
    if (shouldUseEnvVar(ignore_env, env_region)) {
        return std::string(env_region);
    }
    return property_region;
}

}
