#include "Auth.h"
#include "Logger.h"

LOGGER_TAG("com.amazonaws.kinesis.video");

namespace com { namespace amazonaws { namespace kinesis { namespace video {

using std::mutex;

CredentialProvider::CredentialProvider()
    :   next_rotation_time_(0) {
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
    auto now_time = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch());
    // update if we've exceeded the refresh interval
    if (now_time > next_rotation_time_ || forceUpdate) {
        LOG_INFO("Refreshing credentials. Force refreshing: " << forceUpdate
                         << " Now time is: " << now_time.count()
                         << " Expiration: " << next_rotation_time_.count());
        updateCredentials(credentials_);
        next_rotation_time_ = credentials_.getExpiration();
    }
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
