#pragma once
#include <stdint.h>

namespace webcc
{
    // Non-owning view over raw bytes (schema type `bytes`).
    // Views from events point into the event buffer: valid until the next poll_event().
    class bytes_view
    {
    private:
        const uint8_t* m_data;
        uint32_t m_len;

    public:
        constexpr bytes_view() : m_data(nullptr), m_len(0) {}
        constexpr bytes_view(const uint8_t* data, uint32_t len) : m_data(data), m_len(len) {}

        // vector<uint8_t>, array<uint8_t, N>, ...
        template <typename C>
            requires requires(const C& c) {
                static_cast<const uint8_t*>(c.data());
                static_cast<uint32_t>(c.size());
            }
        constexpr bytes_view(const C& c) : m_data(c.data()), m_len(static_cast<uint32_t>(c.size())) {}

        constexpr const uint8_t* data() const { return m_data; }
        constexpr uint32_t length() const { return m_len; }
        constexpr uint32_t size() const { return m_len; }
        constexpr bool empty() const { return m_len == 0; }

        constexpr const uint8_t* begin() const { return m_data; }
        constexpr const uint8_t* end() const { return m_data + m_len; }

        constexpr uint8_t operator[](uint32_t i) const { return m_data[i]; }
    };
} // namespace webcc
