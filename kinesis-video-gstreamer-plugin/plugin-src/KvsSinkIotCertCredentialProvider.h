#ifndef _KVS_SINK_IOT_CERT_CREDENTIAL_PROVIDER_H
#define _KVS_SINK_IOT_CERT_CREDENTIAL_PROVIDER_H

#include <string>
#include <Auth.h>


namespace com { namespace amazonaws { namespace kinesis { namespace video {

    class KvsSinkIotCertCredentialProvider : public CredentialProvider {
        const std::string iot_get_credential_endpoint_, cert_path_, private_key_path_, ca_cert_path_, role_alias_;

    public:
        KvsSinkIotCertCredentialProvider(const std::string iot_get_credential_endpoint,
                                         const std::string cert_path,
                                         const std::string private_key_path,
                                         const std::string role_alias,
                                         const std::string ca_cert_path):
                iot_get_credential_endpoint_(iot_get_credential_endpoint),
                cert_path_(cert_path),
                private_key_path_(private_key_path),
                role_alias_(role_alias),
                ca_cert_path_(ca_cert_path) {}
        void updateCredentials(Credentials &credentials) override;

    private:
        void dumpCurlEasyInfo(CURL *curl_easy);
    };

}
}
}
}



#endif //_KVS_SINK_IOT_CERT_CREDENTIAL_PROVIDER_H
