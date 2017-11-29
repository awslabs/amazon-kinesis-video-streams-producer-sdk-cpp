#pragma once

#include "Request.h"
#include "Response.h"
#include "Auth.h"
#include "OngoingPutFrameState.h"
#include "Logger.h"

#include <memory>

namespace com { namespace amazonaws { namespace kinesis { namespace video {

// Provides APIs for executing HTTP requests.
class CurlCallManager {
public:

    static CurlCallManager &getInstance();

    std::unique_ptr<Response> call(std::unique_ptr<Request> request,
                                   const RequestSigner* request_signer) const;

    std::unique_ptr<Response> call(std::unique_ptr<Request> request,
                                   const RequestSigner* request_signer,
                                   std::shared_ptr<OngoingPutFrameState> ongoing_state) const;

private:
    // RAII initializer for curl.
    struct CurlGlobalInitializer {
        CurlGlobalInitializer();

        ~CurlGlobalInitializer();
    };

    // singleton with a protected instance - no public constructor
    CurlCallManager();

    ~CurlCallManager();

    // non-copyable
    CurlCallManager(CurlCallManager const &);

    void operator=(CurlCallManager const &);

    void dumpCurlEasyInfo(CURL *curl_easy);

    CurlGlobalInitializer curl_init_;  ///< Ensures curl initialization.
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
