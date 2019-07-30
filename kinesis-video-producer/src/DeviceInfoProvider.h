#pragma once

#include "com/amazonaws/kinesis/video/client/Include.h"
#include <string>

namespace com { namespace amazonaws { namespace kinesis { namespace video {

/**
* Interface for the client implementation which will provide the Kinesis Video DeviceInfo struct.
*/
class DeviceInfoProvider {
public:
    using device_info_t = DeviceInfo;

    /**
     * Initializes a struct of type device_info_t and returns it by value to the caller.
     *
     * The elements of this struct shall be populated with the following fields:
     * - UINT32 version; // Version of the struct
     * - CHAR name[MAX_DEVICE_NAME_LEN]: Device name - human readable, NULL terminated. Should be unique per AWS account.
     * - UINT32 tagCount: Number of tags associated with the device.
     * - StorageInfo storageInfo: Storage configuration information (see Kinesis Video struct for info).
     * - UINT32 streamCount: Max number of declared streams for this client instance.
     *
     * @return  A device_info_t struct with the fields above initialized.
     */
    virtual device_info_t getDeviceInfo() = 0;

    /**
     * Return user's custom user agent string which will be appended to the default user agent string
     *
     */
    virtual const std::string getCustomUserAgent() {
        return "";
    }

    virtual const std::string getCertPath() {
        return "";
    }

    virtual ~DeviceInfoProvider() {}
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com

