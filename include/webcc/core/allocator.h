#pragma once
#include <stdint.h>
#include <stddef.h>

// WebCC heap allocator.
//
// A boundary-tag free-list allocator: small fixed overhead per block, with
// splitting on allocation and coalescing on free so the heap can shrink back
// down instead of only ever growing. Tuned for the WebCC workload (lots of
// growing vector/string buffers churning malloc+free).
//
// Layout of every block:
//
//     [ BlockHeader ][ payload (size bytes) ]
//
// The header is exactly 8 bytes on wasm32 (size_t + pointer), so payloads stay
// 8-byte aligned and the per-allocation overhead matches the old bump
// allocator. Blocks are threaded in physical (address) order via `prev_phys`,
// which lets free() find and merge neighbours in O(1). Free blocks additionally
// store their free-list links inside their (otherwise unused) payload, so the
// free list costs no extra space.
//
// The WASM-specific bits (linear memory growth, `__heap_base`) live behind a
// small backend so the allocator also compiles and runs host-native for tests.

namespace webcc
{
    struct BlockHeader
    {
        // payload size in bytes (8-aligned, low 3 bits free); bit 0 = is-free
        size_t size_flags;
        BlockHeader *prev_phys; // previous block in address order (null if first)
    };

    namespace detail
    {
        constexpr size_t ALIGN = 8;
        constexpr size_t FREE_BIT = 1;
        constexpr size_t HEADER_SIZE = sizeof(BlockHeader);
        // Free blocks stash two list pointers in their payload, so every block
        // must be able to hold them once freed.
        constexpr size_t MIN_PAYLOAD = 2 * sizeof(void *);

        inline size_t align_up(size_t n) { return (n + (ALIGN - 1)) & ~(size_t)(ALIGN - 1); }

        // --- Memory backend ----------------------------------------------------
        // Provides the base address and the committed end of the heap region, and
        // a way to commit more. On WASM this maps to linear memory + memory.grow;
        // host builds use a fixed static arena so the allocator is unit-testable.
#if defined(__wasm__)
        extern "C" uint8_t __heap_base; // linker symbol: start of free RAM
        inline uintptr_t backend_base() { return (uintptr_t)&__heap_base; }
        inline uintptr_t backend_end() { return (uintptr_t)((size_t)__builtin_wasm_memory_size(0) * 65536u); }
        inline bool backend_grow(size_t extra_bytes)
        {
            size_t pages = (extra_bytes + 65535) / 65536;
            return __builtin_wasm_memory_grow(0, pages) != (size_t)-1;
        }
#else
        constexpr size_t HOST_ARENA_BYTES = 64u * 1024u * 1024u;
        alignas(16) inline uint8_t g_host_arena[HOST_ARENA_BYTES];
        inline size_t g_host_committed = 0; // bytes "grown" so far, emulating memory.grow
        inline uintptr_t backend_base() { return (uintptr_t)g_host_arena; }
        inline uintptr_t backend_end() { return (uintptr_t)g_host_arena + g_host_committed; }
        inline bool backend_grow(size_t extra_bytes)
        {
            size_t pages = (extra_bytes + 65535) / 65536;
            size_t add = pages * 65536;
            if (g_host_committed + add > HOST_ARENA_BYTES)
                return false;
            g_host_committed += add;
            return true;
        }
#endif

        // --- Allocator state ---------------------------------------------------
        inline uintptr_t heap_ptr = align_up(backend_base()); // top of the bump region
        inline BlockHeader *last_block = nullptr;             // block bordering the wilderness
        inline BlockHeader *free_head = nullptr;              // head of the free list

        // --- Block helpers -----------------------------------------------------
        inline size_t blk_size(BlockHeader *b) { return b->size_flags & ~(size_t)(ALIGN - 1); }
        inline bool blk_free(BlockHeader *b) { return b->size_flags & FREE_BIT; }
        inline void set_blk(BlockHeader *b, size_t size, bool is_free)
        {
            b->size_flags = size | (is_free ? FREE_BIT : 0);
        }
        inline void *payload(BlockHeader *b) { return (uint8_t *)b + HEADER_SIZE; }
        inline BlockHeader *hdr_of(void *p) { return (BlockHeader *)((uint8_t *)p - HEADER_SIZE); }

        // Next physical block, or null if `b` borders the wilderness (the top).
        inline BlockHeader *next_phys(BlockHeader *b)
        {
            uintptr_t end = (uintptr_t)b + HEADER_SIZE + blk_size(b);
            return end < heap_ptr ? (BlockHeader *)end : nullptr;
        }

        // Free-list links live in the payload of free blocks.
        struct FreeLinks
        {
            BlockHeader *prev;
            BlockHeader *next;
        };
        inline FreeLinks *links(BlockHeader *b) { return (FreeLinks *)payload(b); }

        inline void fl_insert(BlockHeader *b)
        {
            FreeLinks *l = links(b);
            l->prev = nullptr;
            l->next = free_head;
            if (free_head)
                links(free_head)->prev = b;
            free_head = b;
            set_blk(b, blk_size(b), true);
        }

        inline void fl_remove(BlockHeader *b)
        {
            FreeLinks *l = links(b);
            if (l->prev)
                links(l->prev)->next = l->next;
            else
                free_head = l->next;
            if (l->next)
                links(l->next)->prev = l->prev;
            set_blk(b, blk_size(b), false);
        }

        inline bool ensure_capacity(uintptr_t new_top)
        {
            if (new_top <= backend_end())
                return true;
            return backend_grow(new_top - backend_end());
        }

        inline void mem_copy(void *dst, const void *src, size_t n)
        {
            uint8_t *d = (uint8_t *)dst;
            const uint8_t *s = (const uint8_t *)src;
            while (n--)
                *d++ = *s++;
        }

        // Forward declaration: place() releases its split-off tail through free().
        inline void heap_free(void *ptr);

        // Trim `b` (an allocated block) to `size`, returning any usable tail to
        // the free list. Returns the payload pointer.
        inline void *place(BlockHeader *b, size_t size)
        {
            size_t total = blk_size(b);
            if (total >= size + HEADER_SIZE + MIN_PAYLOAD)
            {
                set_blk(b, size, false);
                BlockHeader *rem = (BlockHeader *)((uint8_t *)b + HEADER_SIZE + size);
                rem->prev_phys = b;
                set_blk(rem, total - size - HEADER_SIZE, false);
                if (b == last_block)
                {
                    last_block = rem; // the tail now borders the wilderness
                }
                else
                {
                    BlockHeader *nx = next_phys(rem);
                    if (nx)
                        nx->prev_phys = rem;
                }
                heap_free(payload(rem)); // coalesce forward / reclaim wilderness / insert
            }
            return payload(b);
        }

        // Enlarge an allocated block to `size` payload bytes without moving it.
        // `size` must already be aligned, >= MIN_PAYLOAD and > the block's current
        // size. Returns true on success. Shared by realloc() and try_grow_inplace().
        inline bool grow_inplace(BlockHeader *b, size_t size)
        {
            size_t cur = blk_size(b);

            // Absorb a free next neighbour.
            BlockHeader *nx = next_phys(b);
            if (nx && blk_free(nx) && cur + HEADER_SIZE + blk_size(nx) >= size)
            {
                fl_remove(nx);
                BlockHeader *after = next_phys(nx);
                if (nx == last_block)
                    last_block = b;
                else if (after)
                    after->prev_phys = b;
                set_blk(b, cur + HEADER_SIZE + blk_size(nx), false);
                place(b, size); // split off any excess
                return true;
            }

            // Extend into the wilderness if we are the top block.
            if (b == last_block)
            {
                uintptr_t new_top = (uintptr_t)b + HEADER_SIZE + size;
                if (ensure_capacity(new_top))
                {
                    set_blk(b, size, false);
                    heap_ptr = new_top;
                    return true;
                }
            }

            return false;
        }
    } // namespace detail

    inline void *malloc(size_t size)
    {
        using namespace detail;
        if (size == 0 || size > SIZE_MAX - HEADER_SIZE - MIN_PAYLOAD)
            return nullptr;

        size = align_up(size);
        if (size < MIN_PAYLOAD)
            size = MIN_PAYLOAD;

        // 1. First-fit search of the free list.
        for (BlockHeader *b = free_head; b; b = links(b)->next)
        {
            if (blk_size(b) >= size)
            {
                fl_remove(b);
                return place(b, size);
            }
        }

        // 2. Bump a fresh block off the wilderness, growing memory if needed.
        uintptr_t cur_top = heap_ptr;
        uintptr_t new_top = cur_top + HEADER_SIZE + size;
        if (!ensure_capacity(new_top))
            return nullptr;

        BlockHeader *b = (BlockHeader *)cur_top;
        b->prev_phys = last_block;
        set_blk(b, size, false);
        heap_ptr = new_top;
        last_block = b;
        return payload(b);
    }

    namespace detail
    {
        inline void heap_free(void *ptr)
        {
            if (!ptr)
                return;

            BlockHeader *b = hdr_of(ptr);

            // Coalesce with the next physical block if it is free.
            BlockHeader *nx = next_phys(b);
            if (nx && blk_free(nx))
            {
                fl_remove(nx);
                if (nx == last_block)
                    last_block = b;
                set_blk(b, blk_size(b) + HEADER_SIZE + blk_size(nx), false);
                BlockHeader *after = next_phys(b);
                if (after)
                    after->prev_phys = b;
            }

            // Coalesce with the previous physical block if it is free.
            BlockHeader *pv = b->prev_phys;
            if (pv && blk_free(pv))
            {
                fl_remove(pv);
                if (b == last_block)
                    last_block = pv;
                set_blk(pv, blk_size(pv) + HEADER_SIZE + blk_size(b), false);
                BlockHeader *after = next_phys(pv);
                if (after)
                    after->prev_phys = pv;
                b = pv;
            }

            // If the (possibly merged) block borders the wilderness, give it back
            // to the bump pointer instead of holding it on the free list.
            if (b == last_block)
            {
                heap_ptr = (uintptr_t)b;
                last_block = b->prev_phys;
                return;
            }

            fl_insert(b);
        }
    } // namespace detail

    inline void free(void *ptr) { detail::heap_free(ptr); }

    inline void *realloc(void *ptr, size_t size)
    {
        using namespace detail;
        if (!ptr)
            return malloc(size);
        if (size == 0)
        {
            free(ptr);
            return nullptr;
        }
        if (size > SIZE_MAX - HEADER_SIZE - MIN_PAYLOAD)
            return nullptr;

        size = align_up(size);
        if (size < MIN_PAYLOAD)
            size = MIN_PAYLOAD;

        BlockHeader *b = hdr_of(ptr);
        size_t cur = blk_size(b);

        // Shrinking (or same size): trim in place, releasing the tail.
        if (cur >= size)
            return place(b, size);

        // Growing: try to do it in place (no copy) first.
        if (grow_inplace(b, size))
            return ptr;

        // Fall back to allocate + copy + free.
        void *np = malloc(size);
        if (!np)
            return nullptr;
        mem_copy(np, ptr, cur < size ? cur : size);
        free(ptr);
        return np;
    }

    // Enlarge an existing allocation to at least `size` bytes WITHOUT moving any
    // bytes, returning true on success and false if it could not grow in place.
    // Unlike realloc this never relocates, so it is safe for element types whose
    // move/copy constructors must run (the caller relocates them itself on a
    // false result). Returns false for a null pointer.
    inline bool try_grow_inplace(void *ptr, size_t size)
    {
        using namespace detail;
        if (!ptr || size == 0 || size > SIZE_MAX - HEADER_SIZE - MIN_PAYLOAD)
            return false;
        size = align_up(size);
        if (size < MIN_PAYLOAD)
            size = MIN_PAYLOAD;
        BlockHeader *b = hdr_of(ptr);
        if (blk_size(b) >= size)
            return true; // already big enough
        return grow_inplace(b, size);
    }

    namespace detail
    {
        // Introspection helpers (used by tests; tree-shaken when unused).
        inline size_t heap_used() { return (size_t)(heap_ptr - align_up(backend_base())); }
        inline size_t free_block_count()
        {
            size_t n = 0;
            for (BlockHeader *b = free_head; b; b = links(b)->next)
                ++n;
            return n;
        }
        inline size_t free_bytes()
        {
            size_t s = 0;
            for (BlockHeader *b = free_head; b; b = links(b)->next)
                s += blk_size(b);
            return s;
        }
#if !defined(__wasm__)
        // Reset all allocator state. Host/test builds only.
        inline void heap_reset()
        {
            g_host_committed = 0;
            heap_ptr = align_up(backend_base());
            last_block = nullptr;
            free_head = nullptr;
        }
#endif
    } // namespace detail
} // namespace webcc
