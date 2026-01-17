#pragma once

#include <stdint.h>

namespace webcc
{
    // Fast xorshift128+ random number generator
    // State must be initialized with non-zero values
    struct RandomState
    {
        uint64_t s[2];
    };

    // Global random state (initialized at runtime with time-based seed)
    inline RandomState& get_random_state()
    {
        static RandomState state = {{0, 0}};
        static bool initialized = false;
        if (!initialized)
        {
            // Initialize with simple time-based seed
            // Will be properly seeded when webcc runtime starts
            state.s[0] = 1234567890ULL;
            state.s[1] = 9876543210ULL;
            initialized = true;
        }
        return state;
    }

    // Seed the random number generator
    inline void random_seed(uint64_t seed)
    {
        RandomState& state = get_random_state();
        state.s[0] = seed;
        state.s[1] = seed ^ 0x9E3779B97F4A7C15ULL;
        // Warm up the generator
        for (int i = 0; i < 10; i++)
        {
            uint64_t s1 = state.s[0];
            uint64_t s0 = state.s[1];
            state.s[0] = s0;
            s1 ^= s1 << 23;
            state.s[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
        }
    }

    // Generate a random uint64_t
    inline uint64_t random_u64()
    {
        RandomState& state = get_random_state();
        uint64_t s1 = state.s[0];
        uint64_t s0 = state.s[1];
        state.s[0] = s0;
        s1 ^= s1 << 23;
        state.s[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
        return state.s[1] + s0;
    }

    // Generate a random float in range [0, 1)
    inline float random()
    {
        // Use upper 53 bits to get good precision
        uint64_t r = random_u64() >> 11;
        return (float)(r / (double)(1ULL << 53));
    }

    // Generate a random integer in range [min, max]
    inline int random_int(int min, int max)
    {
        if (min > max)
        {
            int temp = min;
            min = max;
            max = temp;
        }
        uint64_t range = (uint64_t)(max - min + 1);
        return min + (int)(random_u64() % range);
    }

    // Generate a random float in range [min, max]
    inline float random_range(float min, float max)
    {
        return min + random() * (max - min);
    }

} // namespace webcc
