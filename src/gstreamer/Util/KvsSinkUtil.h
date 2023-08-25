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

namespace kvs_sink_util{

    gboolean gstructToMap(GstStructure *g_struct, std::map<std::string, std::string> *user_map);

    gboolean parseIotCredentialGstructure(GstStructure *g_struct,
                                             std::map<std::string, std::string> &iot_cert_params);

}

#endif //__KVS_SINK_UTIL_H__