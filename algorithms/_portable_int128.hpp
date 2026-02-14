#pragma once
#include <cstdint>

// Portable signed 64×64 → 128 multiply support.
// Mirrors _portable_uint128.hpp for signed operations.
//
// On GCC/Clang ARM, an inline asm barrier prevents the compiler from
// replacing a signed multiply with an unsigned one when it can prove
// an operand is non-negative. This is needed for algorithms that rely
// on smulh producing a sign-extended high half.


// ============================================================================
//  CASE 1: GCC / Clang / LLVM with native __int128
// ============================================================================
#if defined(__SIZEOF_INT128__)

struct int128_t {
    __extension__ __int128 v;

    int128_t() = default;
    int128_t(__int128 x) : v(x) {}   // wrap parameter
    int128_t(int64_t x) : v(x) {}

    explicit operator uint64_t() const { return (uint64_t)v; }
};

inline int128_t operator*(int128_t a, int64_t b) {
    int64_t lo = (int64_t)a.v;
#if defined(__aarch64__)
    // Prevent compiler from knowing sign, forcing smulh on ARM.
    asm("" : "+r"(lo));
#endif
    __extension__ __int128 tmp = lo;
    return int128_t(tmp * b);
}

inline int128_t operator*(int64_t a, int128_t b) {
    return b * a;
}

inline uint64_t operator>>(int128_t x, unsigned shift) {
    if (shift == 0)  return (uint64_t)x.v;
    if (shift < 64) {
        __extension__ unsigned __int128 tmp = x.v;
        return (uint64_t)(tmp >> shift);
    }
    if (shift == 64) return (uint64_t)(x.v >> 64);
    if (shift < 128) return (uint64_t)(x.v >> shift);
    return 0;
}

inline uint64_t lo128(int128_t x) { return (uint64_t)x.v; }
inline uint64_t hi128(int128_t x) { return (uint64_t)(x.v >> 64); }

#else


// ============================================================================
//  CASE 2: MSVC — implement int128_t manually
// ============================================================================
struct int128_t {
    uint64_t lo;
    int64_t  hi;

    int128_t() = default;
    int128_t(int64_t v) : lo((uint64_t)v), hi(v >> 63) {}
    int128_t(int64_t hi_, uint64_t lo_) : lo(lo_), hi(hi_) {}

    explicit operator uint64_t() const { return lo; }
};


// ============================================================================
//  Backend for signed 64×64 → 128 multiply
// ============================================================================
#if defined(_MSC_VER) && defined(_M_X64)

#include <intrin.h>

inline int128_t s128_mul64(int64_t a, int64_t b) {
    int128_t r;
    r.lo = _mul128(a, b, &r.hi);
    return r;
}

#elif defined(_MSC_VER) && defined(_M_ARM64)

#include <intrin.h>

inline int128_t s128_mul64(int64_t a, int64_t b) {
    int128_t r;
    r.hi = __mulh(a, b);
    r.lo = (uint64_t)((uint64_t)a * (uint64_t)b);
    return r;
}

#else

// Portable fallback: signed 64×64 → 128 using 32-bit multiplies
inline int128_t s128_mul64(int64_t a, int64_t b) {
    // Compute unsigned abs(a) * abs(b), then fix sign
    int neg = (a < 0) != (b < 0);
    uint64_t ua = a < 0 ? (uint64_t)(-(uint64_t)a) : (uint64_t)a;
    uint64_t ub = b < 0 ? (uint64_t)(-(uint64_t)b) : (uint64_t)b;

    uint32_t a_lo = (uint32_t)ua, a_hi = (uint32_t)(ua >> 32);
    uint32_t b_lo = (uint32_t)ub, b_hi = (uint32_t)(ub >> 32);

    uint64_t ll = (uint64_t)a_lo * b_lo;
    uint64_t lh = (uint64_t)a_lo * b_hi;
    uint64_t hl = (uint64_t)a_hi * b_lo;
    uint64_t hh = (uint64_t)a_hi * b_hi;

    uint64_t mid  = lh + (ll >> 32);
    uint64_t mid2 = (uint64_t)(uint32_t)mid + hl;

    uint64_t r_lo = ((uint64_t)(uint32_t)mid2 << 32) | (uint32_t)ll;
    uint64_t r_hi = hh + (mid >> 32) + (mid2 >> 32);

    if (neg) {
        r_lo = ~r_lo + 1;
        r_hi = ~r_hi + (r_lo == 0 ? 1 : 0);
    }

    int128_t r;
    r.lo = r_lo;
    r.hi = (int64_t)r_hi;
    return r;
}

#endif


inline int128_t operator*(int128_t a, int64_t b) {
    return s128_mul64((int64_t)a.lo, b);
}

inline int128_t operator*(int64_t a, int128_t b) {
    return s128_mul64(a, (int64_t)b.lo);
}

inline uint64_t operator>>(const int128_t& v, unsigned shift) {
    if (shift == 0)  return v.lo;
    if (shift < 64)  return (v.lo >> shift) | ((uint64_t)v.hi << (64 - shift));
    if (shift == 64) return (uint64_t)v.hi;
    if (shift < 128) return (uint64_t)(v.hi >> (shift - 64));
    return 0;
}

inline uint64_t lo128(const int128_t& v) { return v.lo; }
inline uint64_t hi128(const int128_t& v) { return (uint64_t)v.hi; }


#endif // __SIZEOF_INT128__
