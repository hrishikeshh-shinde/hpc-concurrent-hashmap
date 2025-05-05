#ifndef THREAD_PARTITION_HASH_MAP_H
#define THREAD_PARTITION_HASH_MAP_H

#include "AbstractHashMap.h"
#include <vector>
#include <array>
#include <atomic>
#include <string>
#include <functional>   // For std::hash
#include <stdexcept>    // For exceptions
#include <limits>       // For numeric_limits

// Define states for entries
enum class EntryState : uint8_t {
    EMPTY,
    OCCUPIED,
    DELETED // Tombstone
};


class ThreadPartitionHashMap : public AbstractHashMap {
private:
    // --- Configuration ---
    static constexpr size_t NUM_SUBMAPS = 32;
    const size_t num_worker_threads_; // Number of threads the application WILL use

    // --- Data Structures ---
    struct Entry {
        std::atomic<EntryState> state;
        std::string key;

        Entry() : state(EntryState::EMPTY) {}
        Entry(const Entry& other) = delete;
        Entry& operator=(const Entry& other) = delete;
        Entry(Entry&& other) noexcept : state(other.state.load(std::memory_order_relaxed)), key(std::move(other.key)) {}
        Entry& operator=(Entry&& other) noexcept {
            if (this != &other) {
                state.store(other.state.load(std::memory_order_relaxed));
                key = std::move(other.key);
            }
            return *this;
        }
    };

    class Submap {
    public:
        ThreadPartitionHashMap* parent;
        std::vector<Entry> entries;
        size_t size; // Non-atomic

        Submap() : parent(nullptr), size(0) { entries.resize(8); }
        Submap(ThreadPartitionHashMap* p, size_t initial_capacity = 8)
            : parent(p), size(0) { entries.resize(initial_capacity > 0 ? initial_capacity : 8); }
        Submap(const Submap& other) = delete;
        Submap& operator=(const Submap& other) = delete;
        Submap(Submap&& other) noexcept
        : parent(other.parent), entries(std::move(other.entries)), size(other.size)
          { other.parent = nullptr; other.size = 0; }
        Submap& operator=(Submap&& other) noexcept {
            if (this != &other) {
                parent = other.parent; entries = std::move(other.entries); size = other.size;
                other.parent = nullptr; other.size = 0;
            }
            return *this;
        }
        size_t capacity() const { return entries.size(); }
    };

    // --- Member Variables ---
    std::array<Submap, NUM_SUBMAPS> submaps;
    std::atomic<size_t> total_size; // Atomic total count across submaps
    std::hash<std::string> hasher;

    // --- Private Helper Methods ---

    // Performs resizing for a specific submap. Assumes called only by owner thread.
    void resize_submap(size_t submap_idx);

public:
    // Constructor - REQUIRES the number of worker threads the application will use
    explicit ThreadPartitionHashMap(size_t num_worker_threads, float loadFactor = 0.7f);

    // --- Public Interface (Compatible with AbstractHashMap) ---
    // IMPORTANT: The caller (application) MUST ensure that these methods are only
    // called by the thread that owns the submap corresponding to the key.
    // Use get_submap_index() and get_owner_thread_index() to check ownership
    // before calling these methods in a multi-threaded context.

    bool insert(std::string key) override;
    bool search(std::string key) const override;
    bool remove(std::string key) override;

    // Returns the total size across all submaps
    int size() const override;

    // Global rehash is not supported in this model.
    // Resizing happens locally within insert().
    void rehash() override;

    // Destructor
    ~ThreadPartitionHashMap() override;

    // --- Public Helper Methods for Application Ownership Check ---

    // Determines which submap a key belongs to
    size_t get_submap_index(const std::string& key) const;

    // Determines which worker thread index (0 to num_worker_threads - 1) owns a given submap
    // The application should compare this to its own worker index.
    size_t get_owner_thread_index(size_t submap_idx) const;

    // Allows application to know the configured number of threads
    size_t get_num_worker_threads() const { return num_worker_threads_; }

    // Allows application to know the number of submaps
    size_t get_num_submaps() const { return NUM_SUBMAPS; }

};

#endif