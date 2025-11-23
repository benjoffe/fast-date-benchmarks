#pragma once
#include <cstdint>

// This is a minimal wrapper enabling __uint128_t-style behaviour on
// Microsoft’s compiler. It implements only the small subset of 128-bit
// operations required by these algorithms. MSVC’s lack of native
// unsigned 128-bit support has historical and architectural reasons,
// but from a practical low-level optimisation perspective it is
// unfortunate that a compatibility layer is necessary.


// ============================================================================
//  CASE 1: GCC / Clang / LLVM with native unsigned __int128
// ============================================================================
#if defined(__SIZEOF_INT128__)

using uint128_t = __uint128_t;

// No operator overloads here — GCC/Clang already support them natively.

inline uint64_t lo128(uint128_t v) { return (uint64_t)v; }
inline uint64_t hi128(uint128_t v) { return (uint64_t)(v >> 64); }

#else


// ============================================================================
//  CASE 2: MSVC — implement uint128_t manually
// ============================================================================
struct uint128_t {
    uint64_t lo;
    uint64_t hi;

    uint128_t() = default;

    // unsigned
    uint128_t(uint64_t v) : lo(v), hi(0) {}
    uint128_t(uint32_t v) : lo(v), hi(0) {}

    // signed literal support (needed!)
    uint128_t(int v) : lo((uint64_t)(uint32_t)v), hi(0) {}
    uint128_t(int64_t v) : lo((uint64_t)v), hi(0) {}

    // explicit hi/lo constructor
    uint128_t(uint64_t hi_, uint64_t lo_) : lo(lo_), hi(hi_) {}

    explicit operator uint64_t() const { return lo; }
};


// ============================================================================
//  Backend for 64×64 → 128 multiply
// ============================================================================
#if defined(_MSC_VER) && defined(_M_X64)

#include <intrin.h>

inline uint128_t u128_mul64(uint64_t a, uint64_t b) {
    uint128_t r;
    r.lo = _umul128(a, b, &r.hi);
    return r;
}

#elif defined(_MSC_VER) && defined(_M_ARM64)

#include <intrin.h>

// ARM64 MSVC: hi via __umulh, lo via normal mul
inline uint128_t u128_mul64(uint64_t a, uint64_t b) {
    uint128_t r;
    r.hi = __umulh(a, b);
    r.lo = a * b;
    return r;
}

#else
#error "MSVC platform missing 128-bit multiply support."
#endif


inline uint128_t operator*(uint128_t a, uint64_t b) {
    uint128_t r = u128_mul64(a.lo, b);
    r.hi += a.hi * b;
    return r;
}

inline uint128_t operator*(uint64_t a, uint128_t b) {
    uint128_t r = u128_mul64(a, b.lo);
    r.hi += b.hi * a;
    return r;
}


// ============================================================================
//  Right-shift operator: return 64-bit high part when >> 64
// ============================================================================
inline uint64_t operator>>(const uint128_t& v, unsigned shift) {
    if (shift == 0)     return v.lo;
    if (shift < 64)     return (v.lo >> shift) | (v.hi << (64 - shift));
    if (shift == 64)    return v.hi;
    if (shift < 128)    return v.hi >> (shift - 64);
    return 0;
}

inline uint64_t lo128(const uint128_t& v) { return v.lo; }
inline uint64_t hi128(const uint128_t& v) { return v.hi; }


#endif // __SIZEOF_INT128__