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

namespace kvs_sink_util{

    gboolean parse_gstructure(GstStructure *g_struct, std::map<std::string, std::string> &iot_cert_params);

    bool parseTimeStr(std::string time_str, std::chrono::duration<uint64_t> &time_obj);
}

#endif //__KVS_SINK_UTIL_H__