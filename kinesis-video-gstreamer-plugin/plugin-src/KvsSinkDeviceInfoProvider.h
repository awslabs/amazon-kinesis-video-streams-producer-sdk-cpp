#ifndef __KVS_SINK_DEVICE_INFO_PROVIDER_H__
#define __KVS_SINK_DEVICE_INFO_PROVIDER_H__

#include "gstkvssink.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {
    class KvsSinkDeviceInfoProvider: public DefaultDeviceInfoProvider {
        std::shared_ptr<CustomData> data;
    public:
        KvsSinkDeviceInfoProvider(std::shared_ptr<CustomData> data): data(data) {}
        device_info_t getDeviceInfo() override;
    };
}
}
}
}

#endif //__KVS_SINK_DEVICE_INFO_PROVIDER_H__
