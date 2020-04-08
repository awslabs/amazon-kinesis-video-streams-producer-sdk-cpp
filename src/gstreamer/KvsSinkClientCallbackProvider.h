#ifndef __KVS_SINK_CLIENT_CALLBACK_PROVIDER_H__
#define __KVS_SINK_CLIENT_CALLBACK_PROVIDER_H__

#include <ClientCallbackProvider.h>
#include <Logger.h>

namespace com { namespace amazonaws { namespace kinesis { namespace video {

    class KvsSinkClientCallbackProvider: public ClientCallbackProvider {
    public:

        StorageOverflowPressureFunc getStorageOverflowPressureCallback() override {
            return storageOverflowPressure;
        }

        UINT64 getCallbackCustomData() override {
            return reinterpret_cast<UINT64> (this);
        }

    private:
        static STATUS storageOverflowPressure(UINT64 custom_handle, UINT64 remaining_bytes);
    };
}
}
}
}


#endif //__KVS_SINK_CLIENT_CALLBACK_PROVIDER_H__
