#pragma once

namespace com { namespace amazonaws { namespace kinesis { namespace video {

class Response;

/**
 * Abstract response listener
 */
class ResponseAcceptor {
public:
    virtual void setResponse(STREAM_HANDLE stream_handle, std::shared_ptr<Response> response) = 0;
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
