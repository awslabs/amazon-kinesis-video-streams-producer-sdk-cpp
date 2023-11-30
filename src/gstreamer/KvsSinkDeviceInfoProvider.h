#ifndef __KVS_SINK_DEVICE_INFO_PROVIDER_H__
#define __KVS_SINK_DEVICE_INFO_PROVIDER_H__

#include <DefaultDeviceInfoProvider.h>

namespace com { namespace amazonaws { namespace kinesis { namespace video {
    class KvsSinkDeviceInfoProvider: public DefaultDeviceInfoProvider {
        uint64_t storage_size_mb_;
        uint64_t stop_stream_timeout_sec_;
        uint64_t service_call_connection_timeout_sec_;
        uint64_t service_call_completion_timeout_sec_;
    public:
        KvsSinkDeviceInfoProvider(uint64_t storage_size_mb,
                                  uint64_t stop_stream_timeout_sec,
                                  uint64_t service_call_connection_timeout_sec,
                                  uint64_t service_call_completion_timeout_sec):
                storage_size_mb_(storage_size_mb),
                stop_stream_timeout_sec_(stop_stream_timeout_sec),
                service_call_connection_timeout_sec_(service_call_connection_timeout_sec),
                service_call_completion_timeout_sec_(service_call_completion_timeout_sec) {}
        device_info_t getDeviceInfo() override;
    };
}
}
}
}

#endif //__KVS_SINK_DEVICE_INFO_PROVIDER_H__
