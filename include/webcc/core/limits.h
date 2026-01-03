#pragma once
#include <cstdint>

namespace webcc {

template<class T> struct numeric_limits {
    static constexpr bool is_specialized = false;
    static constexpr T min() { return T(); }
    static constexpr T max() { return T(); }
};

template<> struct numeric_limits<int8_t> {
    static constexpr bool is_specialized = true;
    static constexpr int8_t min() { return -128; }
    static constexpr int8_t max() { return 127; }
};

template<> struct numeric_limits<uint8_t> {
    static constexpr bool is_specialized = true;
    static constexpr uint8_t min() { return 0; }
    static constexpr uint8_t max() { return 255; }
};

template<> struct numeric_limits<int16_t> {
    static constexpr bool is_specialized = true;
    static constexpr int16_t min() { return -32768; }
    static constexpr int16_t max() { return 32767; }
};

template<> struct numeric_limits<uint16_t> {
    static constexpr bool is_specialized = true;
    static constexpr uint16_t min() { return 0; }
    static constexpr uint16_t max() { return 65535; }
};

template<> struct numeric_limits<int32_t> {
    static constexpr bool is_specialized = true;
    static constexpr int32_t min() { return -2147483648; }
    static constexpr int32_t max() { return 2147483647; }
};

template<> struct numeric_limits<uint32_t> {
    static constexpr bool is_specialized = true;
    static constexpr uint32_t min() { return 0; }
    static constexpr uint32_t max() { return 4294967295U; }
};

template<> struct numeric_limits<int64_t> {
    static constexpr bool is_specialized = true;
    static constexpr int64_t min() { return -9223372036854775807LL - 1; }
    static constexpr int64_t max() { return 9223372036854775807LL; }
};

template<> struct numeric_limits<uint64_t> {
    static constexpr bool is_specialized = true;
    static constexpr uint64_t min() { return 0; }
    static constexpr uint64_t max() { return 18446744073709551615ULL; }
};

template<> struct numeric_limits<float> {
    static constexpr bool is_specialized = true;
    static constexpr float min() { return 1.17549435e-38F; }
    static constexpr float max() { return 3.40282347e+38F; }
};

template<> struct numeric_limits<double> {
    static constexpr bool is_specialized = true;
    static constexpr double min() { return 2.2250738585072014e-308; }
    static constexpr double max() { return 1.7976931348623158e+308; }
};

} // namespace webcc
