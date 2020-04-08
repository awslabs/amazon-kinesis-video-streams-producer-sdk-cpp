/** Copyright 2017 Amazon.com. All rights reserved. */

#pragma once

#include "com/amazonaws/kinesis/video/client/Include.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

/**
* Kinesis Video client level callback provider
*
*   getClientReadyCallback();
*   getStorageOverflowPressureCallback();
*
*/
class ClientCallbackProvider {
public:
    /**
     * Returns the custom data for this object to be used with the callbacks.
     *
     * @return Custom data
     */
    virtual UINT64 getCallbackCustomData() = 0;

    /**
     * Reports a ready state for the client.
     *
     * Optional callback
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual ClientReadyFunc getClientReadyCallback() {
        return nullptr;
    };

    /**
     * Reports storage overflow/pressure
     *
     * Optional callback
     *
     *  @return a function pointer conforming to the description above.
     */
    virtual StorageOverflowPressureFunc getStorageOverflowPressureCallback() {
        return nullptr;
    };

    virtual ~ClientCallbackProvider() {};
};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
