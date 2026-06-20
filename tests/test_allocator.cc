// Unit tests for the WebCC heap allocator (include/webcc/core/allocator.h).
//
// The allocator's WASM backend is swapped for a fixed static arena on host
// builds (see the #if defined(__wasm__) guard in allocator.h), so the splitting,
// coalescing, reclaim and realloc logic can be exercised natively here. Each
// test starts from a clean heap via detail::heap_reset().

#include "webcc/core/allocator.h"
#include "framework.h"

#include <cstdint>

using webcc::detail::free_block_count;
using webcc::detail::free_bytes;
using webcc::detail::heap_reset;
using webcc::detail::heap_used;

namespace
{
    bool aligned8(void *p) { return ((uintptr_t)p % 8) == 0; }
}

TEST(alloc_basic_roundtrip)
{
    heap_reset();
    int *p = (int *)webcc::malloc(sizeof(int) * 4);
    CHECK(p != nullptr);
    for (int i = 0; i < 4; ++i)
        p[i] = i * 7;
    for (int i = 0; i < 4; ++i)
        CHECK_EQ(p[i], i * 7);
    webcc::free(p);
    // A lone block bordering the wilderness is reclaimed, not leaked.
    CHECK_EQ(heap_used(), (size_t)0);
}

TEST(alloc_returns_8byte_aligned)
{
    heap_reset();
    for (size_t s = 1; s <= 64; ++s)
    {
        void *p = webcc::malloc(s);
        CHECK(p != nullptr);
        CHECK(aligned8(p));
    }
}

TEST(zero_size_returns_null)
{
    heap_reset();
    CHECK(webcc::malloc(0) == nullptr);
    webcc::free(nullptr); // must be a no-op
}

TEST(free_block_is_reused_and_split)
{
    heap_reset();
    // Keep a guard at the top so the freed block lands on the free list
    // (instead of being reclaimed straight into the wilderness).
    void *guard = webcc::malloc(16);
    void *big = webcc::malloc(1024);
    void *top = webcc::malloc(16);
    (void)guard;
    (void)top;

    size_t used_before = heap_used();
    webcc::free(big);
    CHECK_EQ(free_block_count(), (size_t)1);
    CHECK(free_bytes() >= 1024);

    // A small request must carve out of the freed 1KB block and return the
    // remainder to the free list — no new memory, and the leftover survives.
    void *small = webcc::malloc(32);
    CHECK(small != nullptr);
    CHECK_EQ(heap_used(), used_before); // reused, did not grow
    CHECK_EQ(free_block_count(), (size_t)1);
    CHECK(free_bytes() >= 1024 - 32 - 64); // remainder kept (minus header slack)
}

TEST(adjacent_frees_coalesce_and_reclaim)
{
    heap_reset();
    void *a = webcc::malloc(64);
    void *b = webcc::malloc(64);
    void *c = webcc::malloc(64);
    void *d = webcc::malloc(64);
    CHECK(a && b && c && d);

    // Free the middle two: they must merge into a single free block.
    webcc::free(b);
    webcc::free(c);
    CHECK_EQ(free_block_count(), (size_t)1);

    // Freeing the rest must coalesce everything and hand it back to the
    // wilderness, leaving the heap empty.
    webcc::free(a);
    webcc::free(d);
    CHECK_EQ(heap_used(), (size_t)0);
    CHECK_EQ(free_block_count(), (size_t)0);
}

TEST(realloc_grows_top_block_in_place)
{
    heap_reset();
    char *p = (char *)webcc::malloc(16);
    for (int i = 0; i < 16; ++i)
        p[i] = (char)(i + 1);

    char *q = (char *)webcc::realloc(p, 64);
    CHECK_EQ((void *)q, (void *)p); // top block, grown in place
    for (int i = 0; i < 16; ++i)
        CHECK_EQ((int)q[i], i + 1); // data preserved

    // Shrinking trims in place too.
    char *r = (char *)webcc::realloc(q, 16);
    CHECK_EQ((void *)r, (void *)q);
}

TEST(realloc_relocates_and_preserves_data)
{
    heap_reset();
    char *p = (char *)webcc::malloc(16);
    for (int i = 0; i < 16; ++i)
        p[i] = (char)(i + 100);
    void *guard = webcc::malloc(16); // pins p: not top, no free neighbour
    (void)guard;

    char *q = (char *)webcc::realloc(p, 128);
    CHECK(q != nullptr);
    CHECK(q != p); // had to move
    for (int i = 0; i < 16; ++i)
        CHECK_EQ((int)q[i], i + 100); // copied across
}

TEST(realloc_edge_cases)
{
    heap_reset();
    void *p = webcc::realloc(nullptr, 32); // acts as malloc
    CHECK(p != nullptr);
    void *q = webcc::realloc(p, 0); // acts as free
    CHECK(q == nullptr);
    CHECK_EQ(heap_used(), (size_t)0);
}

TEST(try_grow_inplace_extends_top_block)
{
    heap_reset();
    char *p = (char *)webcc::malloc(16);
    for (int i = 0; i < 16; ++i)
        p[i] = (char)(i + 1);

    size_t used_before = heap_used();
    CHECK(webcc::try_grow_inplace(p, 128)); // top block grows into wilderness
    CHECK(heap_used() > used_before);        // it did extend
    for (int i = 0; i < 16; ++i)
        CHECK_EQ((int)p[i], i + 1); // bytes never moved
}

TEST(try_grow_inplace_absorbs_free_neighbour)
{
    heap_reset();
    void *a = webcc::malloc(16);
    void *b = webcc::malloc(64);
    void *guard = webcc::malloc(16); // keep b off the top so it lands on the free list
    (void)guard;
    webcc::free(b); // a's next physical neighbour is now free

    CHECK(webcc::try_grow_inplace(a, 64)); // absorbs the freed neighbour, no move
}

TEST(try_grow_inplace_fails_when_pinned)
{
    heap_reset();
    CHECK(!webcc::try_grow_inplace(nullptr, 16));
    void *p = webcc::malloc(16);
    void *guard = webcc::malloc(16); // p is not the top and has no free neighbour
    (void)guard;
    CHECK(!webcc::try_grow_inplace(p, 64)); // cannot grow without relocating
}

// Headline regression: the growing-buffer churn that the old (no-split,
// no-coalesce) allocator leaked on. Each "vector" grows by malloc(new) +
// free(old) — exactly what webcc::vector/string do. With coalescing the
// freed buffers merge and get reused, so the heap stays tiny no matter how
// many vectors we build.
TEST(vector_growth_churn_stays_bounded)
{
    heap_reset();
    for (int v = 0; v < 2000; ++v)
    {
        void *data = nullptr;
        size_t cap = 0;
        for (size_t n = 1; n <= 512; ++n)
        {
            if (n > cap)
            {
                size_t new_cap = cap ? cap * 2 : 1;
                void *nb = webcc::malloc(new_cap * 8);
                CHECK(nb != nullptr);
                if (data)
                    webcc::free(data);
                data = nb;
                cap = new_cap;
            }
        }
        webcc::free(data);
    }
    // The old allocator would have grown without bound here. A single vector's
    // largest buffer is 512*8 = 4096 bytes; the whole run must stay near that.
    CHECK(heap_used() < (size_t)16 * 1024);
}
