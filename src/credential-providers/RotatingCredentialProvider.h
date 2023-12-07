#ifndef __ROTATING_CREDENTIAL_PROVIDER_H__
#define __ROTATING_CREDENTIAL_PROVIDER_H__

#include "Auth.h"

namespace com {
namespace amazonaws {
namespace kinesis {
namespace video {

class RotatingCredentialProvider : public CredentialProvider {
    std::string credential_file_path_;
    PAuthCallbacks rotating_callbacks = nullptr;

  public:
    RotatingCredentialProvider(std::string credential_file_path) : credential_file_path_(credential_file_path)
    {
    }
    void updateCredentials(Credentials& credentials) override
    {
        // no-op as credential update is handled in c producer file auth callbacks
    }
    callback_t getCallbacks(PClientCallbacks) override;
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com

#endif /* __ROTATING_CREDENTIAL_PROVIDER_H__ */