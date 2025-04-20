
#ifndef THREAD_PARTITION_HASH_MAP_H
#define THREAD_PARTITION_HASH_MAP_H

#include "AbstractHashMap.h" // Assumes AbstractHashMap.h defines the base class correctly
#include <vector>
#include <array>
#include <atomic>
#include <string>
#include <functional>   // For std::hash
#include <shared_mutex> // For std::shared_mutex
#include <mutex>        // Added for std::unique_lock, std::lock

// Define states for entries (Tombstones)
// DEFINED ONLY HERE, NOT IN .CPP
enum class EntryState : uint8_t {
    EMPTY,
    OCCUPIED,
    DELETED // Tombstone
};


class ThreadPartitionHashMap : public AbstractHashMap {
private:
    // Number of submaps - typically a power of 2
    static constexpr size_t NUM_SUBMAPS = 32;

    // Entry in the hashmap
    struct Entry {
        std::atomic<EntryState> state;
        std::string key;

        // Default constructor
        Entry() : state(EntryState::EMPTY) {}

        // Delete copy operations for safety with atomics/mutexes
        Entry(const Entry& other) = delete;
        Entry& operator=(const Entry& other) = delete;

        // Move constructor
        Entry(Entry&& other) noexcept : state(other.state.load(std::memory_order_relaxed)), key(std::move(other.key)) {}

        // Move assignment
        Entry& operator=(Entry&& other) noexcept {
            if (this != &other) {
                // Assigning atomics requires store/load
                state.store(other.state.load(std::memory_order_relaxed));
                key = std::move(other.key);
            }
            return *this;
        }
    };

    // Submap (partition of the full map)
    class Submap {
    public:
        ThreadPartitionHashMap* parent; // Pointer back to parent for load factor
        std::vector<Entry> entries;
        std::atomic<size_t> size;
        mutable std::shared_mutex rw_mutex; // Mutex per submap for read/write locking

        // Default constructor - essential for std::array
         Submap() : parent(nullptr), size(0) {
             // Provide a default initial capacity
             entries.resize(8);
         }

        // Constructor with parent and initial capacity
        Submap(ThreadPartitionHashMap* p, size_t initial_capacity = 8)
            : parent(p), size(0) {
             // Ensure capacity is reasonable
             entries.resize(initial_capacity > 0 ? initial_capacity : 8);
        }

        // Delete copy operations
        Submap(const Submap& other) = delete;
        Submap& operator=(const Submap& other) = delete;

        // Move constructor
        Submap(Submap&& other) noexcept
        : parent(other.parent),
          entries(std::move(other.entries)),
          size(other.size.load(std::memory_order_relaxed))
          // rw_mutex is default-constructed for the new object.
          {
            other.parent = nullptr;
        }

        // Move assignment operator
        Submap& operator=(Submap&& other) noexcept {
            if (this != &other) {
                // Lock both mutexes before modifying state to prevent deadlock
                std::unique_lock lhs_lock(rw_mutex, std::defer_lock);
                std::unique_lock rhs_lock(other.rw_mutex, std::defer_lock);
                std::lock(lhs_lock, rhs_lock); // Lock both

                parent = other.parent;
                entries = std::move(other.entries);
                size.store(other.size.load(std::memory_order_relaxed));
                // rw_mutex remains the mutex of *this* object.
                other.parent = nullptr;
            }
            return *this;
        }

        // Get capacity - needs lock if called when resize might happen concurrently
         size_t capacity_unsafe() const {
             // Assumes caller holds appropriate lock (shared or unique)
             return entries.size();
         }
    };

    // --- Member Variables ---
    std::array<Submap, NUM_SUBMAPS> submaps;
    std::atomic<size_t> total_size; // Using size_t for consistency

    // Hash function
    std::hash<std::string> hasher;

    // --- Private Helper Method Declarations ---
    size_t get_submap_index(const std::string& key) const;
    // get_entry_index is no longer needed - calculation done inline
    // get_thread_submaps is no longer needed - removed partitioning logic

    // Resize a specific submap (declaration only)
    void resize_submap(size_t submap_idx);

public:
    // Constructor
    explicit ThreadPartitionHashMap(float loadFactor = 0.7f); // Mark explicit

    // --- Overridden Virtual Functions ---
    // Signatures MUST exactly match AbstractHashMap

    bool insert(std::string key) override; // Takes string by value

    bool search(std::string key) const override; // Takes string by value

    bool remove(std::string key) override; // Takes string by value

    int size() const override; // Returns int as per base class

    void rehash() override;

    // Destructor
    ~ThreadPartitionHashMap() override;
};

#endif // THREAD_PARTITION_HASH_MAP_H
