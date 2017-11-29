/** Copyright 2017 Amazon.com. All rights reserved. */

#pragma once

#include <condition_variable>
#include <map>
#include <mutex>

namespace com { namespace amazonaws { namespace kinesis { namespace video {

/**
 * Thread safe implementation of std::map.
 * @tparam K The key
 * @tparam V
 */
template <typename K, typename V> class ThreadSafeMap {
public:
    /**
     * Put an item into the map.
     * @param k key
     * @param v value
     */
    void put(K k, V v) {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.emplace(std::pair<K, V>(k, v));
    }

    /**
     * Retrieve an item form the map. It checks if the item in the map and returns it in an atomic operation.
     * @param k Key to look up.
     * @return The value at k or nullptr.
     */
    V get(K k) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (contains(k)) {
            return map_[k];
        } else {
            return nullptr;
        }
    }

    /**
     * Remove the pair stored the map at k, if it exists.
     * @param k Key to be removed.
     */
    void remove(K k) {
        std::unique_lock<std::mutex> lock(mutex_);
        auto it = map_.find(k);
        if (it != map_.end()) {
            map_.erase(it);
        }
    }

    /**
     * Check if a key exists in the map.
     * @param k Key to be checked
     * @return True if the key exists and false otherwise.
     */
    bool exists(K k) {
        std::unique_lock<std::mutex> lock(mutex_);
        return contains(k);
    }

    /**
     * UNSAFE!!! Returns the underlying map
     */
    std::map<K, V> getMap() {
        return map_;
    }

private:
    /**
     * Private function to check whether the key exists.
     *
     * NOTE: This is a thread unsafe op.
     */
    bool contains(K k) {
        return map_.find(k) != map_.end();
    }
    /**
     * Non thread safe implementation of the map.
     */
    std::map<K, V> map_;

    /**
     * Mutual exclusion over R/W operations on the map.
     */
    std::mutex mutex_;

};

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
