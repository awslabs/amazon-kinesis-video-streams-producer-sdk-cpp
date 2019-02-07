#ifndef __ROTATING_CREDENTIAL_PROVIDER_H__
#define __ROTATING_CREDENTIAL_PROVIDER_H__

#include "Auth.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

    class RotatingCredentialProvider : public CredentialProvider {
        const int MAX_ROTATION_PERIOD_IN_SECONDS = 2400;
        std::string credential_file_path_;
        const std::chrono::duration<uint64_t> ROTATION_PERIOD = std::chrono::seconds(MAX_ROTATION_PERIOD_IN_SECONDS);
    public:
        RotatingCredentialProvider(std::string credential_file_path): credential_file_path_(credential_file_path) {}

        void updateCredentials(Credentials &credentials) override;
    };

}
}
}
}


#endif /* __ROTATING_CREDENTIAL_PROVIDER_H__ */