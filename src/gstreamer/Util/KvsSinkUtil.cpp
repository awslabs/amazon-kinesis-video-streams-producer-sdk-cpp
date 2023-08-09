#include "KvsSinkUtil.h"
#include <iterator>
#include <iomanip>

LOGGER_TAG("com.amazonaws.kinesis.video.gstkvs");

static const std::set<std::string> iot_param_set = {IOT_GET_CREDENTIAL_ENDPOINT,
                                                    CERTIFICATE_PATH,
                                                    PRIVATE_KEY_PATH,
                                                    CA_CERT_PATH,
                                                    ROLE_ALIASES,
                                                    IOT_THING_NAME,
                                                    IOT_CONNECTION_TIMEOUT,
                                                    IOT_COMPLETION_TIMEOUT};

static const time_t time_point = std::time(NULL);
static const long timezone_offset =
        static_cast<long> (std::mktime(std::gmtime(&time_point)) - std::mktime(std::localtime(&time_point)));


gboolean setParams(GQuark field_id, const GValue *value, gpointer g_ptr_user_map) {
    std::map<std::string, std::string> *target_map = reinterpret_cast<std::map<std::string, std::string> *>(g_ptr_user_map);
    std::string field_str = std::string(g_quark_to_string (field_id));
    std::string value_str;
    gboolean ret = TRUE;

    if (!G_VALUE_HOLDS_STRING(value)) {
        LOG_ERROR("Value should be of \"String\" type for " << field_str);
        ret = FALSE;
        goto CleanUp;
    }

    value_str = std::string(g_value_get_string(value));

    if (value_str.empty() || field_str.empty()) {
        LOG_ERROR("Field and value should not be empty. field: " << field_str << " , value: " << value_str);
        ret = FALSE;
        goto CleanUp;
    }

    target_map->insert(std::pair<std::string,std::string>(field_str, value_str));

CleanUp:
    return ret;
}

namespace kvs_sink_util {

gboolean gstructToMap(GstStructure *g_struct, std::map<std::string, std::string> *user_map) {
    std::map<std::string, std::string> temp;
    gboolean ret = gst_structure_foreach (g_struct, setParams, user_map);
    if (ret) { // if conversion failed, user_map will be unchanged
        user_map->insert(temp.begin(), temp.end());
    }
    return ret;
}

gboolean parseIotCredentialGstructure(GstStructure *g_struct, std::map<std::string, std::string> &iot_cert_params) {
    gboolean ret;
    std::set<std::string> params_key_set;

    ret = gstructToMap(g_struct, &iot_cert_params);

    if (ret == FALSE) {
        goto CleanUp;
    }

    for(std::map<std::string, std::string>::iterator it = iot_cert_params.begin(); it != iot_cert_params.end();
        ++it) {
        params_key_set.insert(it->first);
    }

    if(params_key_set.count(IOT_THING_NAME) == 0) {
        params_key_set.insert(IOT_THING_NAME);
    }

    if(params_key_set.count(IOT_CONNECTION_TIMEOUT) == 0) {
        params_key_set.insert(IOT_CONNECTION_TIMEOUT);
    }

    if(params_key_set.count(IOT_COMPLETION_TIMEOUT) == 0) {
        params_key_set.insert(IOT_COMPLETION_TIMEOUT);
    }

    if (params_key_set != iot_param_set) {
        std::ostringstream ostream;
        std::copy(iot_param_set.begin(), iot_param_set.end(), std::ostream_iterator<std::string>(ostream, ","));
        LOG_ERROR("Missing parameters for iot certificate credential. The following keys are expected"
                          << ostream.str());
        ret = FALSE;
    }

CleanUp:
    return ret;
}
}
