#include "KvsSinkDeviceInfoProvider.h"

using namespace com::amazonaws::kinesis::video;

KvsSinkDeviceInfoProvider::device_info_t KvsSinkDeviceInfoProvider::getDeviceInfo(){
    auto device_info = DefaultDeviceInfoProvider::getDeviceInfo();
    // Set the storage size to user specified size in MB
    device_info.storageInfo.storageSize = static_cast<UINT64>(storage_size_mb_) * 1024 * 1024;
    return device_info;
}
