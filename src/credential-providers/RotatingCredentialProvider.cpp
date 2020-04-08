#include "RotatingCredentialProvider.h"
#include <iostream>
#include <fstream>
#include <vector>

LOGGER_TAG("com.amazonaws.kinesis.video");

using namespace com::amazonaws::kinesis::video;
using namespace std;

RotatingCredentialProvider::callback_t RotatingCredentialProvider::getCallbacks(PClientCallbacks client_callbacks) {
    STATUS retStatus = STATUS_SUCCESS;

    if (STATUS_FAILED(retStatus = createFileAuthCallbacks(client_callbacks,
                           STRING_TO_PCHAR(credential_file_path_),
                           &rotating_callbacks))) {
        std::stringstream status_strstrm;
        status_strstrm << std::hex << retStatus;
        LOG_AND_THROW("Unable to create Rotating Credential provider. Error status: 0x" + status_strstrm.str());
    }
    return *rotating_callbacks;
}
