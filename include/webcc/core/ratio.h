#pragma once
#include <cstdint>

namespace webcc {

template<int64_t A, int64_t B>
struct static_gcd {
    static constexpr int64_t value = static_gcd<B, A % B>::value;
};

template<int64_t A>
struct static_gcd<A, 0> {
    static constexpr int64_t value = A;
};

template<int64_t N>
struct static_abs {
    static constexpr int64_t value = (N < 0) ? -N : N;
};

template<int64_t N, int64_t D = 1>
struct ratio {
    static constexpr int64_t abs_n = static_abs<N>::value;
    static constexpr int64_t abs_d = static_abs<D>::value;
    static constexpr int64_t gcd = static_gcd<abs_n, abs_d>::value;
    static constexpr int64_t num = (D < 0 ? -N : N) / gcd;
    static constexpr int64_t den = (D < 0 ? -D : D) / gcd;
    using type = ratio<num, den>;
};

template<typename R1, typename R2>
using ratio_divide = ratio<R1::num * R2::den, R1::den * R2::num>;

using nano = ratio<1, 1000000000>;
using micro = ratio<1, 1000000>;
using milli = ratio<1, 1000>;

} // namespace webcc
