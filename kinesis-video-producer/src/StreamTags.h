/** Copyright 2017 Amazon.com. All rights reserved. */

#pragma once

#include "com/amazonaws/kinesis/video/client/Include.h"

#include <map>
#include <cstring>
#include <string>

namespace com { namespace amazonaws { namespace kinesis { namespace video {

/**
* Holds a map of tags which are to be set on the Kinesis Video stream.
*/
class StreamTags {
public:
    explicit StreamTags(const std::map<std::string, std::string>* tags);

    /**
     * @return The number of tags in the map.
     */
    size_t count() const;

    /**
     * Convenience method to translate the tags into an Kinesis Video SDK representation of the tags.
     */
    PTag asPTag() const;

private:

    /**
     * Mapping of key/val pairs which are to be set on the Kinesis Video stream for which the tags are associated in the
     * stream definition.
     */
    const std::map<std::string, std::string>* tags_;

};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
