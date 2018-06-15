#ifndef __KVS_SINK_ROTATING_CREDENTIAL_PROVIDER_H__
#define __KVS_SINK_ROTATING_CREDENTIAL_PROVIDER_H__

#include "gstkvssink.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

    class KvsSinkRotatingCredentialProvider : public CredentialProvider {
        const int MAX_ROTATION_PERIOD_IN_SECONDS = 2400;
        shared_ptr<CustomData> data;
        const std::chrono::duration<uint64_t> ROTATION_PERIOD = std::chrono::seconds(MAX_ROTATION_PERIOD_IN_SECONDS);
    public:
        KvsSinkRotatingCredentialProvider(shared_ptr<CustomData> data): data(data) {}

        void updateCredentials(Credentials &credentials) override;
    };

}
}
}
}


#endif /* __KVS_SINK_ROTATING_CREDENTIAL_PROVIDER_H__ */