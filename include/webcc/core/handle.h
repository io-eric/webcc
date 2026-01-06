#pragma once
#include <stdint.h>

namespace webcc {

    // Minimal type traits to avoid <type_traits> dependency
    template<bool B, class T = void> struct enable_if {};
    template<class T> struct enable_if<true, T> { typedef T type; };
    template<bool B, class T = void> using enable_if_t = typename enable_if<B, T>::type;

    template<class T, T v> struct integral_constant { static constexpr T value = v; };
    template<typename Base, typename Derived> struct is_base_of : integral_constant<bool, __is_base_of(Base, Derived)> {};
    template<typename Base, typename Derived> inline constexpr bool is_base_of_v = is_base_of<Base, Derived>::value;

// Invalid handle sentinel value (-1 since 0 is valid, e.g., document.body)
constexpr int32_t INVALID_HANDLE = -1;

// Base handle type - untyped
struct handle {
    int32_t value;

    constexpr handle() : value(INVALID_HANDLE) {}
    constexpr explicit handle(int32_t v) : value(v) {}

    constexpr bool is_valid() const { return value != INVALID_HANDLE; }
    
    // Allow explicit conversion to int32_t
    constexpr explicit operator int32_t() const { return value; }
    
    bool operator==(const handle& other) const { return value == other.value; }
    bool operator!=(const handle& other) const { return value != other.value; }
};

// Typed handle template for type-safe handles
// Tag is an empty struct used just to distinguish types at compile time
template<typename Tag>
struct typed_handle {
    int32_t value;

    constexpr typed_handle() : value(INVALID_HANDLE) {}
    constexpr explicit typed_handle(int32_t v) : value(v) {}
    
    // Allow construction from untyped handle
    constexpr typed_handle(handle h) : value(h.value) {}

    // Allow conversion from handle types whose tag inherits from this tag (e.g., Canvas -> DOMElement)
    template <typename OtherTag, typename = enable_if_t<is_base_of_v<Tag, OtherTag>>>
    constexpr typed_handle(typed_handle<OtherTag> other) : value(other.value) {}

    constexpr bool is_valid() const { return value != INVALID_HANDLE; }
    
    // Allow explicit conversion to int32_t
    constexpr explicit operator int32_t() const { return value; }
    
    // Allow implicit conversion to untyped handle for API compatibility
    constexpr operator handle() const { return handle(value); }
    
    bool operator==(const typed_handle& other) const { return value == other.value; }
    bool operator!=(const typed_handle& other) const { return value != other.value; }
};

} // namespace webcc
