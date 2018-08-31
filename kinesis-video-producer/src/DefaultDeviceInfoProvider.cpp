/** Copyright 2017 Amazon.com. All rights reserved. */

#include "DefaultDeviceInfoProvider.h"

#include <string>

namespace com { namespace amazonaws { namespace kinesis { namespace video {

using std::string;

/**
 * Default storage size is 256MB which will be suitable for
 * the default 3 minutes buffer at 10MBps + some room for defragmentation.
 */
#define DEFAULT_STORAGE_SIZE            256 * 1024 * 1024

/**
 * Default max stream count
 */
#define DEFAULT_MAX_STREAM_COUNT        16

DefaultDeviceInfoProvider::DefaultDeviceInfoProvider() {
    memset(&device_info_, 0, sizeof(device_info_));
    device_info_.version = DEVICE_INFO_CURRENT_VERSION;

    // Set the device name
    const string &device_id = "Kinesis_Video_Device";
    size_t bytes_written = device_id.copy(reinterpret_cast<char *>(&(device_info_.name)), device_id.size(), 0);

    // Null terminate the array.
    device_info_.name[bytes_written] = '\0';

    // TODO: Device tags are not supported as of yet
    device_info_.tags = nullptr;
    device_info_.tagCount = 0;

    // Set storage info
    device_info_.storageInfo.version = STORAGE_INFO_CURRENT_VERSION;
    device_info_.storageInfo.storageType = DEVICE_STORAGE_TYPE_IN_MEM;
    device_info_.storageInfo.storageSize = DEFAULT_STORAGE_SIZE;

    // We put any overflow storage in /tmp as it is in memory.
    string dir = "/tmp";
    memcpy(&device_info_.storageInfo.rootDirectory, dir.c_str(), dir.size());

    // Set the max stream count
    device_info_.streamCount = DEFAULT_MAX_STREAM_COUNT;
}

DeviceInfoProvider::device_info_t DefaultDeviceInfoProvider::getDeviceInfo() {
    return device_info_;
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com

