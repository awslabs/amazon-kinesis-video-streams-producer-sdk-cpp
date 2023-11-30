/** Copyright 2017 Amazon.com. All rights reserved. */

#include "CallbackProvider.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

CallbackProvider::callback_t CallbackProvider::getCallbacks() {
    MEMSET(&callbacks_, 0, SIZEOF(callbacks_));
    callbacks_.customData = reinterpret_cast<uintptr_t>(this);
    callbacks_.version = CALLBACKS_CURRENT_VERSION;  // from kinesis video client include
    callbacks_.getDeviceCertificateFn = getDeviceCertificateCallback();
    callbacks_.getSecurityTokenFn = getSecurityTokenCallback();
    callbacks_.getDeviceFingerprintFn = getDeviceFingerprintCallback();
    callbacks_.streamUnderflowReportFn = getStreamUnderflowReportCallback();
    callbacks_.storageOverflowPressureFn = getStorageOverflowPressureCallback();
    callbacks_.streamLatencyPressureFn = getStreamLatencyPressureCallback();
    callbacks_.droppedFrameReportFn = getDroppedFrameReportCallback();
    callbacks_.droppedFragmentReportFn = getDroppedFragmentReportCallback();
    callbacks_.bufferDurationOverflowPressureFn = getBufferDurationOverflowPressureCallback();
    callbacks_.streamErrorReportFn = getStreamErrorReportCallback();
    callbacks_.streamReadyFn = getStreamReadyCallback();
    callbacks_.streamClosedFn = getStreamClosedCallback();
    callbacks_.createStreamFn = getCreateStreamCallback();
    callbacks_.describeStreamFn = getDescribeStreamCallback();
    callbacks_.getStreamingEndpointFn = getStreamingEndpointCallback();
    callbacks_.getStreamingTokenFn = getStreamingTokenCallback();
    callbacks_.putStreamFn = getPutStreamCallback();
    callbacks_.tagResourceFn = getTagResourceCallback();
    callbacks_.clientReadyFn = getClientReadyCallback();
    callbacks_.createDeviceFn = getCreateDeviceCallback();
    callbacks_.deviceCertToTokenFn = getDeviceCertToTokenCallback();
    callbacks_.streamConnectionStaleFn = getStreamConnectionStaleCallback();
    callbacks_.fragmentAckReceivedFn = getFragmentAckReceivedCallback();
    callbacks_.streamDataAvailableFn = getStreamDataAvailableCallback();

    // These callbacks are optional and platform specific defaults are provided by
    // the SDK if  the callback function pointers are defined as NULL.
    callbacks_.createMutexFn = getCreateMutexCallback();
    callbacks_.lockMutexFn = getLockMutexCallback();
    callbacks_.unlockMutexFn = getUnlockMutexCallback();
    callbacks_.tryLockMutexFn = getTryLockMutexCallback();
    callbacks_.freeMutexFn = getFreeMutexCallback();
    callbacks_.createConditionVariableFn = getCreateConditionVariableCallback();
    callbacks_.signalConditionVariableFn = getSignalConditionVariableCallback();
    callbacks_.broadcastConditionVariableFn = getBroadcastConditionVariableCallback();
    callbacks_.waitConditionVariableFn = getWaitConditionVariableCallback();
    callbacks_.freeConditionVariableFn = getFreeConditionVariableCallback();
    callbacks_.getCurrentTimeFn = getCurrentTimeCallback();
    callbacks_.getRandomNumberFn = getRandomNumberCallback();
    callbacks_.logPrintFn = getLogPrintCallback();

    return callbacks_;
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

CreateConditionVariableFunc CallbackProvider::getCreateConditionVariableCallback() {
    return nullptr;
}

SignalConditionVariableFunc CallbackProvider::getSignalConditionVariableCallback() {
    return nullptr;
}

BroadcastConditionVariableFunc CallbackProvider::getBroadcastConditionVariableCallback() {
    return nullptr;
}

WaitConditionVariableFunc CallbackProvider::getWaitConditionVariableCallback() {
    return nullptr;
}

FreeConditionVariableFunc CallbackProvider::getFreeConditionVariableCallback() {
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

BufferDurationOverflowPressureFunc CallbackProvider::getBufferDurationOverflowPressureCallback() {
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

StreamShutdownFunc CallbackProvider::getStreamShutdownCallback() {
    return nullptr;
}

ClientShutdownFunc CallbackProvider::getClientShutdownCallback() {
    return nullptr;
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
