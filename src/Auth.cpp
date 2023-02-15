#include "Auth.h"
#include "Logger.h"

LOGGER_TAG("com.amazonaws.kinesis.video");

namespace com { namespace amazonaws { namespace kinesis { namespace video {

using std::mutex;

CredentialProvider::CredentialProvider()
    :   next_rotation_time_(0),
        security_token_(NULL) {
}

void CredentialProvider::getCredentials(Credentials& credentials) {

    // synchronize credential access since multiple clients may call simultaneously
    std::lock_guard<mutex> guard(credential_mutex_);
    refreshCredentials();
    credentials = credentials_;
}

void CredentialProvider::getUpdatedCredentials(Credentials& credentials) {

    // synchronize credential access since multiple clients may call simultaneously
    std::lock_guard<mutex> guard(credential_mutex_);
    refreshCredentials(true);
    credentials = credentials_;
}

void CredentialProvider::refreshCredentials(bool forceUpdate) {
    auto now_time = systemCurrentTime().time_since_epoch();
    // update if we've exceeded the refresh interval with grace period
    if (forceUpdate || now_time + CredentialProviderGracePeriod > next_rotation_time_) {
        LOG_DEBUG("Refreshing credentials. Force refreshing: " << forceUpdate
                         << " Now time is: " << now_time.count()
                         << " Expiration: " << next_rotation_time_.count());
        updateCredentials(credentials_);
        next_rotation_time_ = credentials_.getExpiration();
    }
}

void CredentialProvider::updateCredentials(Credentials &credentials) {
    // no-op
}
CredentialProvider::~CredentialProvider() {
    freeAwsCredentials(&security_token_);
}

CredentialProvider::callback_t CredentialProvider::getCallbacks(PClientCallbacks clientCallbacks) {
    UNUSED_PARAM(clientCallbacks);

    MEMSET(&callbacks_, 0, SIZEOF(callbacks_));
    callbacks_.customData = reinterpret_cast<uintptr_t>(this);
    callbacks_.version = AUTH_CALLBACKS_CURRENT_VERSION;
    callbacks_.getDeviceCertificateFn = getDeviceCertificateCallback();
    callbacks_.getDeviceFingerprintFn = getDeviceFingerPrintCallback();
    callbacks_.getSecurityTokenFn = getSecurityTokenCallback();
    callbacks_.getStreamingTokenFn = getStreamingTokenCallback();
    callbacks_.deviceCertToTokenFn = deviceCertToTokenCallback();

    addAuthCallbacks(clientCallbacks, &callbacks_);

    return callbacks_;
}

GetDeviceCertificateFunc CredentialProvider::getDeviceCertificateCallback() {
    // we are using a security token, so this callback should be null.
    return nullptr;
}

GetDeviceFingerprintFunc CredentialProvider::getDeviceFingerPrintCallback() {
    // we are using a security token, so this callback should be null.
    return nullptr;
}

GetSecurityTokenFunc CredentialProvider::getSecurityTokenCallback() {
    return getSecurityTokenHandler;
}

GetStreamingTokenFunc CredentialProvider::getStreamingTokenCallback() {
    return getStreamingTokenHandler;
}

DeviceCertToTokenFunc CredentialProvider::deviceCertToTokenCallback() {
    // We are using a security token, so this callback should be null.
    return nullptr;
}

STATUS CredentialProvider::getStreamingTokenHandler(UINT64 custom_data, PCHAR stream_name, STREAM_ACCESS_MODE access_mode, PServiceCallContext p_service_call_context) {
    LOG_DEBUG("getStreamingTokenHandler invoked");
    UNUSED_PARAM(stream_name);
    UNUSED_PARAM(access_mode);

    auto this_obj = reinterpret_cast<CredentialProvider*>(custom_data);

    Credentials credentials;
    this_obj->getUpdatedCredentials(credentials);

    auto access_key = credentials.getAccessKey();
    auto access_key_len = access_key.length();
    auto secret_key = credentials.getSecretKey();
    auto secret_key_len = secret_key.length();
    auto session_token = credentials.getSessionToken();
    auto session_token_len = session_token.length();
    // Credentials expiration count is in seconds. Need to set the expiration in Kinesis Video time
    auto expiration = credentials.getExpiration().count() * HUNDREDS_OF_NANOS_IN_A_SECOND;

    // free current aws credential first
    freeAwsCredentials(&this_obj->security_token_);

    // Store the buffer so we can release it at the end
    createAwsCredentials((PCHAR) access_key.c_str(), access_key_len, (PCHAR) secret_key.c_str(), secret_key_len,
                         (PCHAR) session_token.c_str(), session_token_len, expiration, &this_obj->security_token_);

    STATUS status = getStreamingTokenResultEvent(
            p_service_call_context->customData, SERVICE_CALL_RESULT_OK,
            reinterpret_cast<PBYTE>(this_obj->security_token_),
            this_obj->security_token_->size,
            expiration);

    return status;
}

STATUS CredentialProvider::getSecurityTokenHandler(UINT64 custom_data, PBYTE* pp_token, PUINT32 p_size, PUINT64 p_expiration) {
    LOG_DEBUG("getSecurityTokenHandler invoked");

    auto this_obj = reinterpret_cast<CredentialProvider*>(custom_data);

    Credentials credentials;
    this_obj->getCredentials(credentials);

    auto access_key = credentials.getAccessKey();
    auto access_key_len = access_key.length();
    auto secret_key = credentials.getSecretKey();
    auto secret_key_len = secret_key.length();
    auto session_token = credentials.getSessionToken();
    auto session_token_len = session_token.length();

    // free current aws credential first
    freeAwsCredentials(&this_obj->security_token_);

    // Credentials expiration count is in seconds. Need to set the expiration in Kinesis Video time
    *p_expiration = credentials.getExpiration().count() * HUNDREDS_OF_NANOS_IN_A_SECOND;

    // Store the buffer so we can release it at the end
    createAwsCredentials((PCHAR) access_key.c_str(), access_key_len, (PCHAR) secret_key.c_str(), secret_key_len,
            (PCHAR) session_token.c_str(), session_token_len, *p_expiration, &this_obj->security_token_);

    *pp_token = (PBYTE) (this_obj->security_token_);
    *p_size = this_obj->security_token_->size;

    return STATUS_SUCCESS;
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
