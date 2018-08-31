#include "KvsSinkUtil.h"
#include <iterator>
#include <iomanip>

LOGGER_TAG("com.amazonaws.kinesis.video.gstkvs");

static const std::set<std::string> iot_param_set = {IOT_GET_CREDENTIAL_ENDPOINT,
                                                    CERTIFICATE_PATH,
                                                    PRIVATE_KEY_PATH,
                                                    CA_CERT_PATH,
                                                    ROLE_ALIASES};

static const time_t time_point = std::time(NULL);
static const long timezone_offset =
        static_cast<long> (std::mktime(std::gmtime(&time_point)) - std::mktime(std::localtime(&time_point)));

static gboolean set_params(GQuark field_id, const GValue *value, gpointer user_data) {
    std::map<std::string, std::string> &iot_cert_params = *(std::map<std::string, std::string> *)user_data;
    std::string field_str = std::string(g_quark_to_string (field_id));
    std::string value_str;
    gboolean ret = TRUE;

    if (!G_VALUE_HOLDS_STRING(value)) {
        LOG_ERROR("Value should be of \"String\" type for " << field_str);
        ret = FALSE;
        goto CleanUp;
    }

    value_str = std::string(g_value_get_string(value));

    if (iot_param_set.count(field_str) == 0 || value_str.empty()) {
        LOG_ERROR("Invalid field: " << field_str << " , value: " << value_str);
        ret = FALSE;
        goto CleanUp;
    }

    iot_cert_params[field_str] = value_str;

CleanUp:
    return ret;
}


namespace kvs_sink_util {

gboolean parse_gstructure(GstStructure *g_struct, std::map<std::string, std::string> &iot_cert_params) {
    gboolean ret;
    std::set<std::string> params_key_set;

    ret = gst_structure_foreach (g_struct, set_params, &iot_cert_params);

    if (ret == FALSE) {
        goto CleanUp;
    }

    for(std::map<std::string, std::string>::iterator it = iot_cert_params.begin(); it != iot_cert_params.end();
            ++it) {
        params_key_set.insert(it->first);
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
