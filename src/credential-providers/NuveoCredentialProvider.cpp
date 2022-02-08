#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "NuveoCredentialProvider.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include "httplib.h"
#include "json.hpp"

LOGGER_TAG("com.amazonaws.kinesis.video");

using namespace com::amazonaws::kinesis::video;
using namespace std;


static std::string base64_encode(const std::string &in)
{

    std::string out;

    int val = 0, valb = -6;
    for (u_char c : in)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6)
        out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4)
        out.push_back('=');
    return out;
}

nlohmann::json NuveoCredentialProvider::echange_credentials(string client_id, string client_secret)
{
    httplib::Client cli("https://smartvision.auth.us-east-1.amazoncognito.com");
    cli.enable_server_certificate_verification(false);

    auto to_encode = client_id + ":" + client_secret;
    httplib::Headers headers = {
                         {"Authorization", "Basic " + base64_encode(to_encode)}};

    auto res = cli.Post(
        "/oauth2/token",
        headers,
        "grant_type=client_credentials",
        "application/x-www-form-urlencoded");

    auto parsed_res = nlohmann::json::parse(res->body);

    std::cout << parsed_res["access_token"] << std::endl;

    return parsed_res;
}

NuveoCredentialProvider::callback_t NuveoCredentialProvider::getCallbacks(PClientCallbacks client_callbacks) 
{
    auto rc = CredentialProvider::getCallbacks(client_callbacks);
    return rc;
}
