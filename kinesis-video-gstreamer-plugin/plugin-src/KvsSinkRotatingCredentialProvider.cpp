#include "KvsSinkRotatingCredentialProvider.h"
#include <iomanip>
#include <iostream>
#include <fstream>
#include <vector>

LOGGER_TAG("com.amazonaws.kinesis.video.gstkvs");

using namespace com::amazonaws::kinesis::video;

void KvsSinkRotatingCredentialProvider::updateCredentials(Credentials &credentials) {

    std::chrono::duration<uint64_t> expiration;
    string word, access_key, expiration_string, secret_key, session_token;
    vector<string> credential_data;
    const int ACCESS_KEY_AND_SECRET_KEY = 3;
    const int ACCESS_KEY_AND_EXPIRATION_AND_SECRET_KEY_AND_SESSION_TOKEN = 5;

    ifstream credential_file (data->kvsSink->credential_file_path);
    if (!credential_file.is_open()) {
        LOG_ERROR("Failed to open credential file");
        goto CleanUp;
    }

    /*
     * Expected credential file format:
     * - Static credential file:
     * CREDENTIALS <AWS_ACCESS_KEY_ID> <AWS_SECRET_ACCESS_KEY>
     * - Rotating credential file:
     * CREDENTIALS <AWS_ACCESS_KEY_ID> <AWS_TOKEN_EXPIRATION> <AWS_SECRET_ACCESS_KEY> <AWS_SESSION_TOKEN>
     */
    while(credential_file >> word) {
        credential_data.push_back(word);
    }

    credential_file.close();

    if (credential_data.size() == ACCESS_KEY_AND_SECRET_KEY) {
        access_key = credential_data[1];
        secret_key = credential_data[2];
    } else if (credential_data.size() == ACCESS_KEY_AND_EXPIRATION_AND_SECRET_KEY_AND_SESSION_TOKEN) {
        access_key          = credential_data[1];
        expiration_string   = credential_data[2];
        secret_key          = credential_data[3];
        session_token       = credential_data[4];
    } else {
        LOG_ERROR("Invalid credential information. Possible formats include: " << \
                    "\nCREDENTIALS AWS_ACCESS_KEY_ID AWS_TOKEN_EXPIRATION AWS_SECRET_ACCESS_KEY AWS_SESSION_TOKEN, or" << \
                    "\nCREDENTIALS AWS_ACCESS_KEY_ID AWS_SECRET_ACCESS_KEY.");
        goto CleanUp;
    }

    if (expiration_string.empty()) {
        LOG_WARN("AWS_TOKEN_EXPIRATION not found. Default expiration to now_time + 40min.");
        auto now_time = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch());
        expiration = now_time + ROTATION_PERIOD;
    } else {
        std::tm timeinfo = std::tm();
        std::istringstream ss(expiration_string);
        if (ss >> std::get_time(&timeinfo, "%Y-%m-%dT%H:%M:%SZ"))
        {
            std::time_t tt = std::mktime(&timeinfo);
            system_clock::time_point tp = system_clock::from_time_t (tt);
            expiration = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch());

        } else {
            LOG_ERROR("Failed to parse AWS_TOKEN_EXPIRATION.");
            goto CleanUp;
        }
    }

    credentials.setAccessKey(std::string(access_key));
    credentials.setSecretKey(std::string(secret_key));
    credentials.setSessionToken(session_token);
    credentials.setExpiration(expiration);

CleanUp:

    return;
}
