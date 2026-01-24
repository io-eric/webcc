#pragma once

// Freestanding stddef.h for wasm32 target
// Use compiler built-ins to match ABI expectations

typedef __SIZE_TYPE__    size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;

typedef decltype(nullptr) nullptr_t;

#define NULL nullptr

#define offsetof(type, member) __builtin_offsetof(type, member)
