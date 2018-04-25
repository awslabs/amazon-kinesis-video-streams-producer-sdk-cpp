/** Copyright 2017 Amazon.com. All rights reserved. */

#include "CallbackProvider.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

CallbackProvider::callback_t CallbackProvider::getCallbacks() {
    CallbackProvider::callback_t callbacks;
    callbacks.customData = reinterpret_cast<uintptr_t>(this);
    callbacks.version = CALLBACKS_CURRENT_VERSION;  // from kinesis video client include
    callbacks.getDeviceCertificateFn = getDeviceCertificateCallback();
    callbacks.getSecurityTokenFn = getSecurityTokenCallback();
    callbacks.getDeviceFingerprintFn = getDeviceFingerprintCallback();
    callbacks.streamUnderflowReportFn = getStreamUnderflowReportCallback();
    callbacks.storageOverflowPressureFn = getStorageOverflowPressureCallback();
    callbacks.streamLatencyPressureFn = getStreamLatencyPressureCallback();
    callbacks.droppedFrameReportFn = getDroppedFrameReportCallback();
    callbacks.droppedFragmentReportFn = getDroppedFragmentReportCallback();
    callbacks.streamErrorReportFn = getStreamErrorReportCallback();
    callbacks.streamReadyFn = getStreamReadyCallback();
    callbacks.streamClosedFn = getStreamClosedCallback();
    callbacks.createStreamFn = getCreateStreamCallback();
    callbacks.describeStreamFn = getDescribeStreamCallback();
    callbacks.getStreamingEndpointFn = getStreamingEndpointCallback();
    callbacks.getStreamingTokenFn = getStreamingTokenCallback();
    callbacks.putStreamFn = getPutStreamCallback();
    callbacks.tagResourceFn = getTagResourceCallback();
    callbacks.clientReadyFn = getClientReadyCallback();
    callbacks.createDeviceFn = getCreateDeviceCallback();
    callbacks.deviceCertToTokenFn = getDeviceCertToTokenCallback();
    callbacks.streamConnectionStaleFn = getStreamConnectionStaleCallback();
    callbacks.fragmentAckReceivedFn = getFragmentAckReceivedCallback();
    callbacks.streamDataAvailableFn = getStreamDataAvailableCallback();

    // These callbacks are optional and platform specific defaults are provided by
    // the SDK if  the callback function pointers are defined as NULL.
    callbacks.createMutexFn = getCreateMutexCallback();
    callbacks.lockMutexFn = getLockMutexCallback();
    callbacks.unlockMutexFn = getUnlockMutexCallback();
    callbacks.tryLockMutexFn = getTryLockMutexCallback();
    callbacks.freeMutexFn = getFreeMutexCallback();
    callbacks.getCurrentTimeFn = getCurrentTimeCallback();
    callbacks.getRandomNumberFn = getRandomNumberCallback();
    callbacks.logPrintFn = getLogPrintCallback();

    return callbacks;
}

void CallbackProvider::shutdown() {
    // No-op
}

void CallbackProvider::shutdownStream(STREAM_HANDLE stream_handle) {
    UNUSED_PARAM(stream_handle);
    // No-op
}

CreateMutexFunc CallbackProvider::getCreateMutexCallback() {
    return nullptr;
}

LockMutexFunc CallbackProvider::getLockMutexCallback() {
    return nullptr;
}

UnlockMutexFunc CallbackProvider::getUnlockMutexCallback() {
    return nullptr;
}

TryLockMutexFunc CallbackProvider::getTryLockMutexCallback() {
    return nullptr;
}

FreeMutexFunc CallbackProvider::getFreeMutexCallback() {
    return nullptr;
}

GetCurrentTimeFunc CallbackProvider::getCurrentTimeCallback() {
    return nullptr;
}

GetRandomNumberFunc CallbackProvider::getRandomNumberCallback() {
    return nullptr;
}

LogPrintFunc CallbackProvider::getLogPrintCallback() {
    return nullptr;
}

ClientReadyFunc CallbackProvider::getClientReadyCallback() {
    return nullptr;
}

StreamDataAvailableFunc CallbackProvider::getStreamDataAvailableCallback() {
    return nullptr;
}

StreamReadyFunc CallbackProvider::getStreamReadyCallback() {
    return nullptr;
}

StreamClosedFunc CallbackProvider::getStreamClosedCallback() {
    return nullptr;
}

DroppedFragmentReportFunc CallbackProvider::getDroppedFragmentReportCallback() {
    return nullptr;
}

StreamErrorReportFunc CallbackProvider::getStreamErrorReportCallback() {
    return nullptr;
}

StreamConnectionStaleFunc CallbackProvider::getStreamConnectionStaleCallback() {
    return nullptr;
}

DroppedFrameReportFunc CallbackProvider::getDroppedFrameReportCallback() {
    return nullptr;
}

StreamLatencyPressureFunc CallbackProvider::getStreamLatencyPressureCallback() {
    return nullptr;
}

FragmentAckReceivedFunc CallbackProvider::getFragmentAckReceivedCallback() {
    return nullptr;
}

StorageOverflowPressureFunc CallbackProvider::getStorageOverflowPressureCallback() {
    return nullptr;
}

StreamUnderflowReportFunc CallbackProvider::getStreamUnderflowReportCallback() {
    return nullptr;
}

GetDeviceFingerprintFunc CallbackProvider::getDeviceFingerprintCallback() {
    return nullptr;
}

GetSecurityTokenFunc CallbackProvider::getSecurityTokenCallback() {
    return nullptr;
}

GetDeviceCertificateFunc CallbackProvider::getDeviceCertificateCallback() {
    return nullptr;
}

DeviceCertToTokenFunc CallbackProvider::getDeviceCertToTokenCallback() {
    return nullptr;
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
