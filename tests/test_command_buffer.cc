// Byte-level tests for the command buffer serialization format.
//
// This is the wire format shared between the C++ producer (CommandBuffer) and
// the generated JS decoder (gen_js_case in generators.cc). Both sides must agree
// on the layout: little-endian integers, IEEE-754 floats/doubles, 8-byte
// alignment for doubles, and 4-byte padding for strings. These tests pin that
// layout so the two stay in sync.
#include "framework.h"
#include "command_buffer.h"

#include <cstring>
#include <string>
#include <vector>

using webcc::CommandBuffer;

namespace webcc
{
    void flush();
}

namespace
{
    // one flush: the bytes and their address mod 8
    struct Chunk
    {
        size_t addr_mod8;
        std::vector<uint8_t> bytes;
    };
    std::vector<Chunk> g_flushed;
}

// Overrides the weak no-op stub in command_buffer.cc
extern "C" void webcc_js_flush(uintptr_t ptr, size_t size)
{
    const uint8_t *p = reinterpret_cast<const uint8_t *>(ptr);
    g_flushed.push_back({ptr % 8, std::vector<uint8_t>(p, p + size)});
}

namespace
{
    // Helpers to read back little-endian values from the buffer.
    uint32_t read_u32(const uint8_t *p)
    {
        return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
               ((uint32_t)p[3] << 24);
    }

    uint64_t read_u64(const uint8_t *p)
    {
        uint64_t lo = read_u32(p);
        uint64_t hi = read_u32(p + 4);
        return lo | (hi << 32);
    }
}

TEST(command_buffer_u32_is_little_endian)
{
    CommandBuffer::reset();
    CommandBuffer::push_u32(0x11223344u);
    CHECK_EQ(CommandBuffer::size(), (size_t)4);
    const uint8_t *d = CommandBuffer::data();
    CHECK_EQ((int)d[0], 0x44);
    CHECK_EQ((int)d[1], 0x33);
    CHECK_EQ((int)d[2], 0x22);
    CHECK_EQ((int)d[3], 0x11);
}

TEST(command_buffer_i32_negative_roundtrips)
{
    CommandBuffer::reset();
    CommandBuffer::push_i32(-2);
    const uint8_t *d = CommandBuffer::data();
    CHECK_EQ(read_u32(d), 0xFFFFFFFEu);
}

TEST(command_buffer_float_bit_exact)
{
    CommandBuffer::reset();
    float v = 1.5f;
    CommandBuffer::push_float(v);
    uint32_t bits;
    std::memcpy(&bits, &v, 4);
    CHECK_EQ(read_u32(CommandBuffer::data()), bits);
}

TEST(command_buffer_double_is_8byte_aligned)
{
    CommandBuffer::reset();
    // Write one u32 first so the offset is at 4 (mis-aligned for a double).
    CommandBuffer::push_u32(0xAABBCCDDu);
    double v = 3.14159265358979;
    CommandBuffer::push_double(v);

    // Expect: [u32 @ 0..4][pad @ 4..8][double @ 8..16] => total 16 bytes.
    CHECK_EQ(CommandBuffer::size(), (size_t)16);
    const uint8_t *d = CommandBuffer::data();
    // Padding bytes are zero.
    CHECK_EQ((int)d[4], 0);
    CHECK_EQ((int)d[5], 0);
    CHECK_EQ((int)d[6], 0);
    CHECK_EQ((int)d[7], 0);

    uint64_t bits;
    std::memcpy(&bits, &v, 8);
    CHECK_EQ(read_u64(d + 8), bits);
}

TEST(command_buffer_double_no_pad_when_already_aligned)
{
    CommandBuffer::reset();
    CommandBuffer::push_u32(1);
    CommandBuffer::push_u32(2); // offset now 8, already aligned
    double v = 2.0;
    CommandBuffer::push_double(v);
    CHECK_EQ(CommandBuffer::size(), (size_t)16); // no extra padding inserted
    uint64_t bits;
    std::memcpy(&bits, &v, 8);
    CHECK_EQ(read_u64(CommandBuffer::data() + 8), bits);
}

TEST(command_buffer_string_length_prefixed_and_padded)
{
    CommandBuffer::reset();
    const char *s = "abc"; // len 3 -> padded to 4
    CommandBuffer::push_string(s, 3);

    // [u32 len=3][a b c][pad 1] => 4 + 4 = 8 bytes
    CHECK_EQ(CommandBuffer::size(), (size_t)8);
    const uint8_t *d = CommandBuffer::data();
    CHECK_EQ(read_u32(d), 3u);
    CHECK_EQ((int)d[4], 'a');
    CHECK_EQ((int)d[5], 'b');
    CHECK_EQ((int)d[6], 'c');
    CHECK_EQ((int)d[7], 0); // pad byte
}

TEST(command_buffer_string_aligned_length_no_padding)
{
    CommandBuffer::reset();
    const char *s = "wxyz"; // len 4 -> no padding
    CommandBuffer::push_string(s, 4);
    // [u32 len=4][w x y z] => 8 bytes, no trailing pad
    CHECK_EQ(CommandBuffer::size(), (size_t)8);
    CHECK_EQ(read_u32(CommandBuffer::data()), 4u);
}

TEST(command_buffer_empty_string)
{
    CommandBuffer::reset();
    CommandBuffer::push_string("", 0);
    // just the length prefix, no data, no padding
    CHECK_EQ(CommandBuffer::size(), (size_t)4);
    CHECK_EQ(read_u32(CommandBuffer::data()), 0u);
}

TEST(command_buffer_reset_clears_offset)
{
    CommandBuffer::reset();
    CommandBuffer::push_u32(1);
    CommandBuffer::push_u32(2);
    CHECK_EQ(CommandBuffer::size(), (size_t)8);
    CommandBuffer::reset();
    CHECK_EQ(CommandBuffer::size(), (size_t)0);
}

// A realistic command stream: opcode + handle + 4 doubles (e.g. fill_rect).
// Pins the exact framing a decoder will walk.
TEST(command_buffer_fill_rect_like_sequence)
{
    CommandBuffer::reset();
    CommandBuffer::push_u32(0x10);        // opcode
    CommandBuffer::push_i32(7);           // ctx handle
    CommandBuffer::push_double(10.0);     // x  (offset 8 -> aligned)
    CommandBuffer::push_double(20.0);     // y
    CommandBuffer::push_double(100.0);    // w
    CommandBuffer::push_double(50.0);     // h

    // 4 (op) + 4 (handle) + 4*8 (doubles) = 40 (offset 8 is already aligned)
    CHECK_EQ(CommandBuffer::size(), (size_t)40);
    const uint8_t *d = CommandBuffer::data();
    CHECK_EQ(read_u32(d), 0x10u);
    CHECK_EQ(read_u32(d + 4), 7u);
    // offset after op+handle = 8 (already aligned), so first double at offset 8.
    double x;
    uint64_t bits = read_u64(d + 8);
    std::memcpy(&x, &bits, 8);
    CHECK_EQ(x, 10.0);
}

// --- Overflow handling -------------------------------------------------------

namespace
{
    // walk a chunk like the JS decoder
    struct Reader
    {
        const Chunk &c;
        size_t off = 0;

        bool done() const { return off >= c.bytes.size(); }
        uint32_t u32()
        {
            uint32_t v = read_u32(c.bytes.data() + off);
            off += 4;
            return v;
        }
        double f64()
        {
            if ((c.addr_mod8 + off) % 8 != 0)
                off += 4;
            uint64_t bits = read_u64(c.bytes.data() + off);
            off += 8;
            double d;
            std::memcpy(&d, &bits, 8);
            return d;
        }
        std::string str()
        {
            uint32_t len = u32();
            std::string out((const char *)c.bytes.data() + off, len);
            off += (len + 3) & ~3u;
            return out;
        }
    };
}

TEST(command_buffer_flushes_early_when_full)
{
    CommandBuffer::reset();
    g_flushed.clear();

    // ~3MB of commands, 3x the buffer
    const uint32_t count = 100000;
    for (uint32_t i = 0; i < count; ++i)
    {
        CommandBuffer::push_command(42);
        CommandBuffer::push_double((double)i);
        CommandBuffer::push_string("payload-xyz", 11);
    }
    webcc::flush();

    CHECK(g_flushed.size() >= 3);

    // Every chunk holds whole commands, and none are lost or reordered
    uint32_t next = 0;
    bool intact = true;
    for (const Chunk &c : g_flushed)
    {
        Reader r{c};
        while (!r.done())
        {
            if (r.u32() != 42 || r.f64() != (double)next || r.str() != "payload-xyz")
                intact = false;
            ++next;
        }
        if (r.off != c.bytes.size())
            intact = false;
    }
    CHECK(intact);
    CHECK_EQ(next, count);
    CHECK_EQ(CommandBuffer::size(), (size_t)0);
}

TEST(command_buffer_grows_for_oversized_command)
{
    CommandBuffer::reset();
    g_flushed.clear();

    std::string big(3 * 1024 * 1024, 'x'); // 3x the buffer
    big.front() = '<';
    big.back() = '>';

    CommandBuffer::push_command(1);
    CommandBuffer::push_u32(11);

    CommandBuffer::push_command(2);
    CommandBuffer::push_u32(22);
    CommandBuffer::push_string(big.data(), big.size());
    CommandBuffer::push_double(1.5);

    CommandBuffer::push_command(3);
    CommandBuffer::push_u32(33);
    webcc::flush();

    // Decode everything that reached JS, in order
    std::vector<uint32_t> opcodes;
    bool big_ok = false;
    for (const Chunk &c : g_flushed)
    {
        Reader r{c};
        while (!r.done())
        {
            uint32_t op = r.u32();
            opcodes.push_back(op);
            uint32_t arg = r.u32();
            if (op == 2)
            {
                std::string s = r.str();
                double d = r.f64();
                big_ok = arg == 22 && s == big && d == 1.5;
            }
        }
    }
    CHECK_EQ(opcodes.size(), (size_t)3);
    if (opcodes.size() == 3)
    {
        CHECK_EQ(opcodes[0], (uint32_t)1);
        CHECK_EQ(opcodes[1], (uint32_t)2);
        CHECK_EQ(opcodes[2], (uint32_t)3);
    }
    CHECK(big_ok);

    // Back to normal afterwards
    CHECK_EQ(CommandBuffer::size(), (size_t)0);
    CommandBuffer::push_command(4);
    CHECK_EQ(CommandBuffer::size(), (size_t)4);
    CommandBuffer::reset();
}
