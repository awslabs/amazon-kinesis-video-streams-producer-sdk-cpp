#ifndef __CREDENTIAL_PROVIDER_UTIL_H__
#define __CREDENTIAL_PROVIDER_UTIL_H__

#include <string>
#include <map>
#include <set>
#include <Logger.h>
#include <chrono>

#define IOT_GET_CREDENTIAL_ENDPOINT "endpoint"
#define CERTIFICATE_PATH "cert-path"
#define PRIVATE_KEY_PATH "key-path"
#define CA_CERT_PATH "ca-path"
#define ROLE_ALIASES "role-aliases"

namespace com { namespace amazonaws { namespace kinesis { namespace video { namespace credential_provider_util {

    bool parseTimeStr(std::string time_str, std::chrono::duration<uint64_t> &time_obj);
}
}
}
}
}

#endif //__CREDENTIAL_PROVIDER_UTIL_H__
