// include/concurrent_hashmap/base_hashmap.hpp

#pragma once

#include <optional>
#include <cstddef>

namespace concurrent {

/**
 * Base class interface for all concurrent hashmap implementations.
 * This defines the common interface that all concrete implementations must support.
 */
template <typename Key, typename Value>
class base_hashmap {
public:
    // Core operations
    virtual bool insert(const Key& key, const Value& value) = 0;
    virtual bool insert(Key&& key, Value&& value) = 0;
    
    virtual std::optional<Value> find(const Key& key) const = 0;
    
    virtual bool erase(const Key& key) = 0;
    
    virtual bool contains(const Key& key) const = 0;
    
    virtual size_t size() const = 0;
    
    virtual void clear() = 0;
    
    // Statistics and configuration
    virtual size_t bucket_count() const = 0;
    virtual float load_factor() const = 0;
    virtual void max_load_factor(float ml) = 0;
    
    // Virtual destructor
    virtual ~base_hashmap() = default;
};

} // namespace concurrent