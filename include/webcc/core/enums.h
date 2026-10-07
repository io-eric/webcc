// GENERATED FILE - DO NOT EDIT
#pragma once
#include <stdint.h>

namespace webcc::dom {
    // Flags, combine with |
    enum class PointerFlags : uint8_t {
        CAPTURE = 1,
        COALESCED = 2,
        NO_SCROLL = 4,
        PREDICT = 8,
    };
    constexpr PointerFlags operator|(PointerFlags a, PointerFlags b) { return PointerFlags((uint8_t)a | (uint8_t)b); }
    constexpr PointerFlags operator&(PointerFlags a, PointerFlags b) { return PointerFlags((uint8_t)a & (uint8_t)b); }
    constexpr PointerFlags operator^(PointerFlags a, PointerFlags b) { return PointerFlags((uint8_t)a ^ (uint8_t)b); }
    constexpr PointerFlags operator~(PointerFlags a) { return PointerFlags(~(uint8_t)a); }
    constexpr PointerFlags& operator|=(PointerFlags& a, PointerFlags b) { return a = a | b; }
    constexpr PointerFlags& operator&=(PointerFlags& a, PointerFlags b) { return a = a & b; }
    // any bit of `of` set
    constexpr bool any(PointerFlags v, PointerFlags of = PointerFlags(~(uint8_t)0)) { return ((uint8_t)v & (uint8_t)of) != 0; }
} // namespace webcc::dom

namespace webcc::dom {
    // Choices
    enum class PointerPhase : uint8_t {
        DOWN = 0,
        MOVE = 1,
        UP = 2,
        CANCEL = 3,
        PREDICTED = 4,
    };
} // namespace webcc::dom

namespace webcc::dom {
    // Choices
    enum class PointerType : uint8_t {
        MOUSE = 0,
        PEN = 1,
        TOUCH = 2,
    };
} // namespace webcc::dom

namespace webcc::dom {
    // Flags, combine with |
    enum class Buttons : uint32_t {
        PRIMARY = 1,
        SECONDARY = 2,
        AUXILIARY = 4,
        BACK = 8,
        FORWARD = 16,
        ERASER = 32,
    };
    constexpr Buttons operator|(Buttons a, Buttons b) { return Buttons((uint32_t)a | (uint32_t)b); }
    constexpr Buttons operator&(Buttons a, Buttons b) { return Buttons((uint32_t)a & (uint32_t)b); }
    constexpr Buttons operator^(Buttons a, Buttons b) { return Buttons((uint32_t)a ^ (uint32_t)b); }
    constexpr Buttons operator~(Buttons a) { return Buttons(~(uint32_t)a); }
    constexpr Buttons& operator|=(Buttons& a, Buttons b) { return a = a | b; }
    constexpr Buttons& operator&=(Buttons& a, Buttons b) { return a = a & b; }
    // any bit of `of` set
    constexpr bool any(Buttons v, Buttons of = Buttons(~(uint32_t)0)) { return ((uint32_t)v & (uint32_t)of) != 0; }
} // namespace webcc::dom

namespace webcc::canvas {
    // Flags, combine with |
    enum class ContextFlags : uint8_t {
        LOW_LATENCY = 1,
        OPAQUE = 2,
    };
    constexpr ContextFlags operator|(ContextFlags a, ContextFlags b) { return ContextFlags((uint8_t)a | (uint8_t)b); }
    constexpr ContextFlags operator&(ContextFlags a, ContextFlags b) { return ContextFlags((uint8_t)a & (uint8_t)b); }
    constexpr ContextFlags operator^(ContextFlags a, ContextFlags b) { return ContextFlags((uint8_t)a ^ (uint8_t)b); }
    constexpr ContextFlags operator~(ContextFlags a) { return ContextFlags(~(uint8_t)a); }
    constexpr ContextFlags& operator|=(ContextFlags& a, ContextFlags b) { return a = a | b; }
    constexpr ContextFlags& operator&=(ContextFlags& a, ContextFlags b) { return a = a & b; }
    // any bit of `of` set
    constexpr bool any(ContextFlags v, ContextFlags of = ContextFlags(~(uint8_t)0)) { return ((uint8_t)v & (uint8_t)of) != 0; }
} // namespace webcc::canvas

namespace webcc::input {
    // Flags, combine with |
    enum class Mods : uint8_t {
        SHIFT = 1,
        CTRL = 2,
        ALT = 4,
        META = 8,
    };
    constexpr Mods operator|(Mods a, Mods b) { return Mods((uint8_t)a | (uint8_t)b); }
    constexpr Mods operator&(Mods a, Mods b) { return Mods((uint8_t)a & (uint8_t)b); }
    constexpr Mods operator^(Mods a, Mods b) { return Mods((uint8_t)a ^ (uint8_t)b); }
    constexpr Mods operator~(Mods a) { return Mods(~(uint8_t)a); }
    constexpr Mods& operator|=(Mods& a, Mods b) { return a = a | b; }
    constexpr Mods& operator&=(Mods& a, Mods b) { return a = a & b; }
    // any bit of `of` set
    constexpr bool any(Mods v, Mods of = Mods(~(uint8_t)0)) { return ((uint8_t)v & (uint8_t)of) != 0; }
} // namespace webcc::input

namespace webcc::pdf {
    // Choices
    enum class LineCap : uint8_t {
        BUTT = 0,
        ROUND = 1,
        SQUARE = 2,
    };
} // namespace webcc::pdf

namespace webcc::pdf {
    // Choices
    enum class LineJoin : uint8_t {
        MITER = 0,
        ROUND = 1,
        BEVEL = 2,
    };
} // namespace webcc::pdf

namespace webcc::websocket {
    // Choices
    enum class ReadyState : uint8_t {
        CONNECTING = 0,
        OPEN = 1,
        CLOSING = 2,
        CLOSED = 3,
    };
} // namespace webcc::websocket
