#include "KvsSinkDeviceInfoProvider.h"

using namespace com::amazonaws::kinesis::video;

KvsSinkDeviceInfoProvider::device_info_t KvsSinkDeviceInfoProvider::getDeviceInfo(){
    auto device_info = DefaultDeviceInfoProvider::getDeviceInfo();
    // Set the storage size to user specified size in MB
    device_info.storageInfo.storageSize = static_cast<UINT64>(storage_size_mb_) * 1024 * 1024;
    device_info.clientInfo.stopStreamTimeout = static_cast<UINT64>(stop_stream_timeout_sec_ * HUNDREDS_OF_NANOS_IN_A_SECOND);
    device_info.clientInfo.serviceCallCompletionTimeout = static_cast<UINT64>(service_call_completion_timeout_sec_ * HUNDREDS_OF_NANOS_IN_A_SECOND);
    device_info.clientInfo.serviceCallConnectionTimeout = static_cast<UINT64>(service_call_connection_timeout_sec_ * HUNDREDS_OF_NANOS_IN_A_SECOND);
    return device_info;
}
