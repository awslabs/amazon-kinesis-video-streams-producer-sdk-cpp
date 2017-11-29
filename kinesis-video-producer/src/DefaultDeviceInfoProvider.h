/** Copyright 2017 Amazon.com. All rights reserved. */

#pragma once

#include "DeviceInfoProvider.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

class DefaultDeviceInfoProvider : public DeviceInfoProvider {
public:
    DefaultDeviceInfoProvider();
    device_info_t getDeviceInfo() override;

protected:

    DeviceInfo device_info_;
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com

