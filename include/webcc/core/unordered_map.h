#pragma once
#include "hash.h"
#include "vector.h"
#include "utility.h"
#include "allocator.h"

namespace webcc
{
    // A lightweight open-addressing hash map implementation.
    // Uses linear probing for collision resolution.
    // Does not support standard node-based stability guarantees, but is cache-friendly.
    template <typename Key, typename T, typename Hash = webcc::hash<Key>>
    class unordered_map
    {
    private:
        // State of each slot in the hash table
        enum State : uint8_t { 
            EMPTY = 0,    // Slot is empty and has never been used (stops probing)
            OCCUPIED = 1, // Slot contains a valid key-value pair
            DELETED = 2   // Slot was used but is now deleted (continues probing)
        };

        struct Entry
        {
            Key key;
            T value;
            State state = EMPTY;
        };

        Entry* m_data = nullptr;
        size_t m_capacity = 0;
        size_t m_size = 0;
        Hash m_hasher;

        // Resize the hash table and re-insert all active elements
        void rehash(size_t new_capacity)
        {
            Entry* old_data = m_data;
            size_t old_capacity = m_capacity;

            m_capacity = new_capacity;
            m_data = (Entry*)webcc::malloc(m_capacity * sizeof(Entry));
            m_size = 0; // Re-inserting will update this

            // Initialize new block
            for (size_t i = 0; i < m_capacity; ++i) {
                m_data[i].state = EMPTY;
            }

            // Move old elements
            if (old_data) {
                for (size_t i = 0; i < old_capacity; ++i) {
                    if (old_data[i].state == OCCUPIED) {
                        insert_internal(webcc::move(old_data[i].key), webcc::move(old_data[i].value));
                        old_data[i].key.~Key();
                        old_data[i].value.~T();
                    }
                }
                webcc::free(old_data);
            }
        }

        void insert_internal(Key&& key, T&& value)
        {
            // Check load factor (0.7)
            if (m_size >= m_capacity * 0.7) {
                rehash(m_capacity == 0 ? 16 : m_capacity * 2);
            }

            size_t idx = m_hasher(key) & (m_capacity - 1);
            size_t insertion_idx = (size_t)-1;

            // Linear probing: search for key or empty slot
            while (m_data[idx].state != EMPTY) {
                if (m_data[idx].state == OCCUPIED) {
                    if (m_data[idx].key == key) {
                        m_data[idx].value = webcc::move(value); // Update existing
                        return;
                    }
                } else if (m_data[idx].state == DELETED) {
                    // Remember first deleted slot to reuse if key not found
                    if (insertion_idx == (size_t)-1) insertion_idx = idx;
                }
                idx = (idx + 1) & (m_capacity - 1);
            }

            // If we didn't find the key, use the first deleted slot we found,
            // otherwise use the empty slot we ended up at.
            if (insertion_idx == (size_t)-1) insertion_idx = idx;
            
            // Construct in place
            new (&m_data[insertion_idx].key) Key(webcc::move(key));
            new (&m_data[insertion_idx].value) T(webcc::move(value));
            m_data[insertion_idx].state = OCCUPIED;
            m_size++;
        }

    public:
        unordered_map() = default;

        ~unordered_map()
        {
            if (m_data) {
                for (size_t i = 0; i < m_capacity; ++i) {
                    if (m_data[i].state == OCCUPIED) {
                        m_data[i].key.~Key();
                        m_data[i].value.~T();
                    }
                }
                webcc::free(m_data);
            }
        }

        T& operator[](const Key& key)
        {
            if (m_capacity == 0) rehash(16);

            size_t idx = m_hasher(key) & (m_capacity - 1);
            size_t start_idx = idx;
            size_t insertion_idx = (size_t)-1;

            // Linear probing search
            while (m_data[idx].state != EMPTY) {
                if (m_data[idx].state == OCCUPIED) {
                    if (m_data[idx].key == key) return m_data[idx].value;
                } else if (m_data[idx].state == DELETED) {
                    // Remember first deleted slot
                    if (insertion_idx == (size_t)-1) insertion_idx = idx;
                }
                idx = (idx + 1) & (m_capacity - 1);
                if (idx == start_idx) break; // Full loop (shouldn't happen with load factor check)
            }

            // Not found, insert default
            if (m_size >= m_capacity * 0.7) {
                rehash(m_capacity * 2);
                return operator[](key); // Retry insert after rehash
            }

            if (insertion_idx == (size_t)-1) insertion_idx = idx;

            new (&m_data[insertion_idx].key) Key(key);
            new (&m_data[insertion_idx].value) T();
            m_data[insertion_idx].state = OCCUPIED;
            m_size++;
            return m_data[insertion_idx].value;
        }

        // Basic iterator support
        struct iterator {
            Entry* ptr;
            Entry* end;

            iterator& operator++() {
                do {
                    ptr++;
                } while (ptr < end && ptr->state != OCCUPIED);
                return *this;
            }

            bool operator!=(const iterator& other) const { return ptr != other.ptr; }
            
            struct Pair {
                Key& first;
                T& second;
            };

            Pair operator*() { return {ptr->key, ptr->value}; }
            Pair* operator->() { return (Pair*)ptr; } // Hacky, but works for simple usage
        };

        iterator begin() {
            if (!m_data) return {nullptr, nullptr};
            Entry* ptr = m_data;
            while (ptr < m_data + m_capacity && ptr->state != OCCUPIED) ptr++;
            return {ptr, m_data + m_capacity};
        }

        iterator end() {
            if (!m_data) return {nullptr, nullptr};
            return {m_data + m_capacity, m_data + m_capacity};
        }
        
        size_t size() const { return m_size; }
        bool empty() const { return m_size == 0; }
        
        iterator find(const Key& key) {
            if (m_capacity == 0) return end();
            size_t idx = m_hasher(key) & (m_capacity - 1);
            while (m_data[idx].state != EMPTY) {
                if (m_data[idx].state == OCCUPIED && m_data[idx].key == key) {
                    return {&m_data[idx], m_data + m_capacity};
                }
                idx = (idx + 1) & (m_capacity - 1);
            }
            return end();
        }
        
        bool contains(const Key& key) const {
            if (m_capacity == 0) return false;
            size_t idx = m_hasher(key) & (m_capacity - 1);
            while (m_data[idx].state != EMPTY) {
                if (m_data[idx].state == OCCUPIED && m_data[idx].key == key) {
                    return true;
                }
                idx = (idx + 1) & (m_capacity - 1);
            }
            return false;
        }
        
        void erase(const Key& key) {
            if (m_capacity == 0) return;
            size_t idx = m_hasher(key) & (m_capacity - 1);
            while (m_data[idx].state != EMPTY) {
                if (m_data[idx].state == OCCUPIED && m_data[idx].key == key) {
                    m_data[idx].key.~Key();
                    m_data[idx].value.~T();
                    m_data[idx].state = DELETED;
                    m_size--;
                    return;
                }
                idx = (idx + 1) & (m_capacity - 1);
            }
        }
        
        void clear() {
             if (m_data) {
                for (size_t i = 0; i < m_capacity; ++i) {
                    if (m_data[i].state == OCCUPIED) {
                        m_data[i].key.~Key();
                        m_data[i].value.~T();
                        m_data[i].state = EMPTY;
                    }
                }
                m_size = 0;
            }
        }
    };
} // namespace webcc
