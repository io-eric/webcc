#pragma once
#include <stdint.h>

namespace webcc {

struct handle {
    int32_t value;

    constexpr handle() : value(0) {}
    constexpr explicit handle(int32_t v) : value(v) {}

    constexpr bool is_valid() const { return value != 0; }
    
    // Allow explicit conversion to int32_t
    constexpr explicit operator int32_t() const { return value; }
    
    bool operator==(const handle& other) const { return value == other.value; }
    bool operator!=(const handle& other) const { return value != other.value; }
};

} // namespace webcc
