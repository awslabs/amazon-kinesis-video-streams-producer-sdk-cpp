#include "CredentialProviderUtil.h"
#include <iterator>
#include <iomanip>

LOGGER_TAG("com.amazonaws.kinesis.video");

static const std::set<std::string> iot_param_set = {IOT_GET_CREDENTIAL_ENDPOINT,
                                                    CERTIFICATE_PATH,
                                                    PRIVATE_KEY_PATH,
                                                    CA_CERT_PATH,
                                                    ROLE_ALIASES};

static const time_t time_point = std::time(NULL);
static const long timezone_offset =
        static_cast<long> (std::mktime(std::gmtime(&time_point)) - std::mktime(std::localtime(&time_point)));

namespace com { namespace amazonaws { namespace kinesis { namespace video { namespace credential_provider_util {

bool parseTimeStr(std::string time_str, std::chrono::duration<uint64_t> &time_obj){
    bool res = true;
    std::tm timeinfo = std::tm();

#if defined(__GNUC__) && (__GNUC__ < 5) && !defined(__APPLE__)
    res = strptime(time_str.c_str(), "%Y-%m-%dT%H:%M:%SZ", &timeinfo) != NULL ? true : false;
#else
    std::istringstream iss(time_str);
    res = iss >> std::get_time(&timeinfo, "%Y-%m-%dT%H:%M:%SZ") ? true : false;
#endif

    if (res) {
        std::time_t tt = std::mktime(&timeinfo);
        // the expiration string from aws credential is in UTC, but get interpreted as local time when mktime.
        // Thus minus timezone to convert back to UTC.
        tt -= timezone_offset;
        std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t (tt);
        time_obj = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch());
        LOG_DEBUG("Credential expiration epoch: " << time_obj.count() << " seconds");
    }

    return res;
}
}
}
}
}
}