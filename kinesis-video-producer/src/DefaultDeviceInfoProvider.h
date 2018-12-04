/** Copyright 2017 Amazon.com. All rights reserved. */

#pragma once

#include "DeviceInfoProvider.h"
#include <string>

namespace com { namespace amazonaws { namespace kinesis { namespace video {

class DefaultDeviceInfoProvider : public DeviceInfoProvider {
public:
    DefaultDeviceInfoProvider(const std::string &custom_useragent = "");
    device_info_t getDeviceInfo() override;
    const std::string getCustomUserAgent() override;

protected:

    DeviceInfo device_info_;
    const std::string custom_useragent_;
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com

