#ifndef __KVS_SINK_STATIC_CREDENTIAL_PROVIDER_H__
#define __KVS_SINK_STATIC_CREDENTIAL_PROVIDER_H__

#include "gstkvssink.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

    class KvsSinkStaticCredentialProvider : public StaticCredentialProvider {
        const std::chrono::duration<uint64_t> ROTATION_PERIOD;
    public:
        KvsSinkStaticCredentialProvider(Credentials &credentials, uint64_t rotation_period_seconds) :
                StaticCredentialProvider(credentials), ROTATION_PERIOD(std::chrono::seconds(rotation_period_seconds)) {}

        void updateCredentials(Credentials &credentials) override;
    };
}
}
}
}

#endif //__KVS_SINK_STATIC_CREDENTIAL_PROVIDER_H__
