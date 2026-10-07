// next_event() tests
// Event format: [Opcode:1][SizeHi:1][SizeLo:2][Data...], size includes the header.
#include "framework.h"
#include "event_buffer.h"

#include <cstdint>
#include <cstring>

using namespace webcc;

namespace
{
    // same layout as push_event_*
    void push_raw_event(uint8_t opcode, uint32_t payload_len, uint8_t fill)
    {
        uint8_t *buf = webcc_event_buffer_ptr();
        uint32_t *offset = webcc_event_offset_ptr();
        uint32_t start = *offset;
        uint32_t len = 4 + payload_len;
        buf[start] = opcode;
        buf[start + 1] = (len >> 16) & 0xFF;
        buf[start + 2] = len & 0xFF;
        buf[start + 3] = (len >> 8) & 0xFF;
        std::memset(buf + start + 4, fill, payload_len);
        *offset = start + len;
    }
}

TEST(event_buffer_reads_events_in_order)
{
    reset_event_buffer();
    push_raw_event(3, 8, 0xAA);
    push_raw_event(7, 4, 0xBB);

    uint8_t opcode = 0;
    const uint8_t *data = nullptr;
    uint32_t len = 0;

    CHECK(next_event(opcode, &data, len));
    CHECK_EQ((int)opcode, 3);
    CHECK_EQ(len, (uint32_t)8);
    CHECK_EQ((int)data[0], 0xAA);

    CHECK(next_event(opcode, &data, len));
    CHECK_EQ((int)opcode, 7);
    CHECK_EQ(len, (uint32_t)4);
    CHECK_EQ((int)data[0], 0xBB);

    CHECK(!next_event(opcode, &data, len));
    CHECK_EQ(event_buffer_size(), (uint32_t)0); // drained buffer is reset
}

TEST(event_buffer_event_larger_than_64kb)
{
    reset_event_buffer();
    const uint32_t big = 200000; // > 16 bits of length
    push_raw_event(5, big, 0x11);
    push_raw_event(9, 4, 0x22);

    uint8_t opcode = 0;
    const uint8_t *data = nullptr;
    uint32_t len = 0;

    CHECK(next_event(opcode, &data, len));
    CHECK_EQ((int)opcode, 5);
    CHECK_EQ(len, big);
    CHECK_EQ((int)data[big - 1], 0x11);

    // Next event still found at the right offset
    CHECK(next_event(opcode, &data, len));
    CHECK_EQ((int)opcode, 9);
    CHECK_EQ(len, (uint32_t)4);
    CHECK_EQ((int)data[0], 0x22);

    CHECK(!next_event(opcode, &data, len));
}

TEST(event_buffer_malformed_length_resets)
{
    reset_event_buffer();
    // Size smaller than the header
    uint8_t *buf = webcc_event_buffer_ptr();
    buf[0] = 1;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;
    *webcc_event_offset_ptr() = 8;

    uint8_t opcode = 0;
    const uint8_t *data = nullptr;
    uint32_t len = 0;
    CHECK(!next_event(opcode, &data, len));
    CHECK_EQ(event_buffer_size(), (uint32_t)0);
}
