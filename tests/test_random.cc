#include "webcc/core/random.h"
#include "framework.h"

// the page's answer to the seed import, here: two fixed 53-bit numbers, then nothing
extern "C" double webcc_system_get_random_seed()
{
    static int asked = 0;
    asked++;
    return asked == 1 ? 6157351234567891.0 : asked == 2 ? 2251799813685249.0 : 0.0;
}

namespace
{
    // the first number out of the fixed start, what an unseeded program drew every time
    uint64_t unseeded_first()
    {
        uint64_t s1 = 1234567890ULL, s0 = 9876543210ULL;
        s1 ^= s1 << 23;
        return (s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5)) + s0;
    }
}

TEST(random_first_use_takes_the_seed_from_the_page)
{
    uint64_t first = webcc::random_u64();
    CHECK(first != unseeded_first());
    // and goes on from there: not stuck, inside its ranges
    uint64_t second = webcc::random_u64();
    CHECK(second != first);
    for (int i = 0; i < 1000; i++)
    {
        float f = webcc::random();
        CHECK(f >= 0.0f && f <= 1.0f);
        int n = webcc::random_int(3, 9);
        CHECK(n >= 3 && n <= 9);
    }
}

TEST(random_seed_given_by_hand_repeats)
{
    webcc::random_seed(42);
    uint64_t a = webcc::random_u64();
    webcc::random_seed(42);
    CHECK_EQ(webcc::random_u64(), a);
}
