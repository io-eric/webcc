// Integration tests: the containers that grow via webcc::try_grow_inplace /
// webcc::realloc must stay correct across reallocation, including for element
// types whose move constructors and destructors must run (the in-place fast
// path must never byte-copy those).

#include "webcc/core/vector.h"
#include "webcc/core/queue.h"
#include "framework.h"

using webcc::detail::heap_reset;
using webcc::detail::heap_used;

namespace
{
    // A non-trivially-movable element: owns a heap allocation. If the vector's
    // in-place growth ever byte-copied these without running the move
    // constructor, teardown would double-free and the heap would not return to
    // empty.
    struct Boxed
    {
        int *p = nullptr;
        explicit Boxed(int v)
        {
            p = (int *)webcc::malloc(sizeof(int));
            *p = v;
        }
        Boxed(Boxed &&o) noexcept : p(o.p) { o.p = nullptr; }
        Boxed &operator=(Boxed &&o) noexcept
        {
            if (this != &o)
            {
                if (p)
                    webcc::free(p);
                p = o.p;
                o.p = nullptr;
            }
            return *this;
        }
        Boxed(const Boxed &) = delete;
        Boxed &operator=(const Boxed &) = delete;
        ~Boxed()
        {
            if (p)
                webcc::free(p);
        }
        int val() const { return p ? *p : -1; }
    };
}

TEST(vector_int_growth_preserves_values)
{
    heap_reset();
    webcc::vector<int> v;
    for (int i = 0; i < 5000; ++i)
        v.push_back(i * 3);
    CHECK_EQ(v.size(), (size_t)5000);
    for (int i = 0; i < 5000; ++i)
        CHECK_EQ(v[i], i * 3);
}

TEST(vector_of_nontrivial_survives_growth)
{
    heap_reset();
    {
        webcc::vector<Boxed> v;
        for (int i = 0; i < 200; ++i)
            v.emplace_back(i);
        for (int i = 0; i < 200; ++i)
            CHECK_EQ(v[i].val(), i); // values intact across many reallocations
    } // every Boxed (and the buffer) destroyed here
    // No leak and no double free: the whole heap coalesces back to empty.
    CHECK_EQ(heap_used(), (size_t)0);
}

TEST(queue_fifo_preserved_linear_growth)
{
    heap_reset();
    webcc::queue<int> q; // m_head stays 0 -> in-place growth fast path
    for (int i = 0; i < 2000; ++i)
        q.push(i);
    for (int i = 0; i < 2000; ++i)
    {
        CHECK_EQ(q.front(), i);
        q.pop();
    }
    CHECK(q.empty());
}

TEST(queue_fifo_preserved_when_wrapped)
{
    heap_reset();
    webcc::queue<int> q;
    for (int i = 0; i < 8; ++i)
        q.push(i);
    for (int i = 0; i < 4; ++i) // advance head so the ring is offset
    {
        CHECK_EQ(q.front(), i);
        q.pop();
    }
    // Growth now happens with m_head != 0 -> must take the relinearizing path.
    for (int i = 8; i < 500; ++i)
        q.push(i);
    for (int i = 4; i < 500; ++i)
    {
        CHECK_EQ(q.front(), i);
        q.pop();
    }
    CHECK(q.empty());
}
