#include "IotCertCredentialProvider.h"

LOGGER_TAG("com.amazonaws.kinesis.video");

using namespace com::amazonaws::kinesis::video;

IotCertCredentialProvider::callback_t IotCertCredentialProvider::getCallbacks(PClientCallbacks client_callbacks)
{
    STATUS retStatus = STATUS_SUCCESS;

    LOG_DEBUG("Creating IoT auth callbacks.");
    if (STATUS_FAILED(retStatus = createIotAuthCallbacks(client_callbacks,
                           STRING_TO_PCHAR(iot_get_credential_endpoint_),
                           STRING_TO_PCHAR(cert_path_),
                           STRING_TO_PCHAR(private_key_path_),
                           STRING_TO_PCHAR(ca_cert_path_),
                           STRING_TO_PCHAR(role_alias_),
                           STRING_TO_PCHAR(stream_name_),
                           &iot_callbacks))) {
        std::stringstream status_strstrm;
        status_strstrm << std::hex << retStatus;
        LOG_AND_THROW("Unable to create Iot Credential provider. Error status: 0x" + status_strstrm.str());
    }
    return *iot_callbacks;
}
