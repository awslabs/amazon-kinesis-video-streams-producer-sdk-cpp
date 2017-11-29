/** Copyright 2017 Amazon.com. All rights reserved. */

#include "StreamTags.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {

using std::get;
using std::string;
using std::map;

StreamTags::StreamTags(const map<string, string>* tags) : tags_(tags) {}

PTag StreamTags::asPTag() const {
    if (nullptr == tags_) {
        return nullptr;
    }

    PTag tags = reinterpret_cast<PTag>(malloc(sizeof(Tag) * tags_->size()));
    size_t i = 0;
    for (const auto &pair : *tags_) {
        Tag &tag = tags[i];
        tag.version = TAG_CURRENT_VERSION;
        auto &name = get<0>(pair);
        auto &val = get<1>(pair);
        assert(MAX_TAG_NAME_LEN >= name.size());
        assert(MAX_TAG_VALUE_LEN >= val.size());

        tag.name = reinterpret_cast<PCHAR>(calloc(name.size() + 1, name.size()));
        tag.value = reinterpret_cast<PCHAR>(calloc(val.size() + 1, val.size()));
        std::memcpy(tag.name, name.c_str(), name.size());
        std::memcpy(tag.value, val.c_str(), val.size());
        ++i;
    }

    return tags;
}

size_t StreamTags::count() const {
    if (nullptr != tags_) {
        return tags_->size();
    } else {
        return 0;
    }
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
