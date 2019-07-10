#ifndef _IOT_CERT_CREDENTIAL_PROVIDER_H_
#define _IOT_CERT_CREDENTIAL_PROVIDER_H_

#include <string>
#include <Auth.h>

namespace com { namespace amazonaws { namespace kinesis { namespace video {
    class IotCertCredentialProvider : public CredentialProvider {
        PAuthCallbacks iot_callbacks;
        const std::string iot_get_credential_endpoint_, cert_path_, private_key_path_, ca_cert_path_,
	                  role_alias_,stream_name_;

    public:
        IotCertCredentialProvider(const std::string iot_get_credential_endpoint,
                                  const std::string cert_path,
                                  const std::string private_key_path,
                                  const std::string role_alias,
                                  const std::string ca_cert_path,
 				    const std::string stream_name):
                iot_get_credential_endpoint_(iot_get_credential_endpoint),
                cert_path_(cert_path),
                private_key_path_(private_key_path),
                role_alias_(role_alias),
                ca_cert_path_(ca_cert_path),
	          stream_name_(stream_name) {}

        void updateCredentials(Credentials& credentials) override {
            // no-op as credential update is handled in c producer iot auth callbacks
        }

        callback_t getCallbacks(PClientCallbacks) override;
    };

}
}
}
}



#endif //_IOT_CERT_CREDENTIAL_PROVIDER_H_
