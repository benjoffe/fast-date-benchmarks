// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2025 Ben Joffe - https://www.benjoffe.com/fast-date

#ifndef EAF_ALGORITHMS_JOFFE_FAST64BIT_H
#define EAF_ALGORITHMS_JOFFE_FAST64BIT_H

#include "eaf/date.hpp"

#include <stdint.h>

#if defined(__aarch64__) || defined(_M_ARM64)
#define IS_ARM 1
#else
#define IS_ARM 0
#endif

using uint128_t = __uint128_t;

struct joffe_fast64bit {

  // Very fast algorithm.
  // Specific benchmark numbers and full explaination will be provided later.

  // Shift constants for working with positive numbers.
  // S = 14704 supports full signed 32-bit input range.
  // S = 4726498270 is suitable for 64-bit input range.
  static uint32_t constexpr S = 14704;
  static uint32_t constexpr D_SHIFT = 146097 * S - 719469;
  static uint32_t constexpr Y_SHIFT = 400 * S - 1;

  static uint64_t constexpr C1 = 505054698555331ull;     // floor(2^64*4/146097):
  static uint64_t constexpr C2 = 50504432782230121ull;   // ceil(2^64*4/1461):
  static uint64_t constexpr C3 = 8619973866219416ull;    // floor(2^64/2140):

  static inline
  date32_t to_date(int32_t dayNumber) {
    
    // 1. Adjust for 100/400 leap year rule.
    uint64_t const rev = D_SHIFT - dayNumber;            //  Reverse day count
    uint64_t const cen = uint128_t(C1) * rev >> 64;      //  Divide 365.2425
    uint64_t const jul = rev - cen / 4 + cen;            //  Julian map

    // 2. Determine year and year-part using an EAF numerator.
    uint128_t const num = uint128_t(C2) * jul;           //  Divide 365.25
    uint32_t const yrs = Y_SHIFT - (num >> 64);          //  Forward year
    uint64_t const low = uint64_t(num);                  //  Remainder
    uint32_t const ypt = uint128_t(782336) * low >> 64;  //  Year-part
    
  #if IS_ARM
    uint32_t const phase = 977920;                       //  Bump later on ARM
  #else
    uint32_t const bump = ypt < 126464;                  //  Jan or Feb
    uint32_t const phase = bump ? 191488 : 977920;       //  Phase offse
  #endif

    // 3. Year-modulo-bitshift for leap years,
    // also revert to forward direction.
    // EAF technique similar to Neri-Schneider .
    uint32_t const N = (yrs % 4) * 512 + phase - ypt;
    uint32_t const M = N >> 16;
    uint32_t const D = uint128_t(C3) * (N % 65536) >> 64; //  Divide 2140

  #if IS_ARM
    // ARM is able to compute `month` here in one cycle
    // where x64 takes two. Delaying the computation of
    // bump enables an Apple Silicon speedup, presumably
    // due to the parallelisation of `ypt` and `N`.
    uint32_t const bump = M > 12;
    uint32_t const month = bump ? M - 12 : M;
  #else
    uint32_t const month = M;
  #endif

    uint32_t const day = D + 1;
    int32_t const year = yrs + bump;

    return date32_t{year, month, day};
  }

  // The below is identical to joffe.hpp
  // It is therefore excluded in the to_rata_die benchmarks
  static inline
  int32_t to_rata_die(int32_t year, uint32_t month, uint32_t day) {

    uint32_t const bump = month <= 2;
    uint32_t const yrs = uint32_t(year + 5880000) - bump;
    uint32_t const cen = yrs / 100;
    int32_t const phase = bump ? 8829 : -2919;

    // Similar to Neri-Scheinder but slightly slower to avoid early overflow:
    uint32_t const year_days = yrs * 365 + yrs / 4 - cen + cen / 4;
    uint32_t const month_days = (979 * month + phase) / 32;
    
    return year_days + month_days + day - 2148345369u;
  }

}; // struct joffe_fast64bit

#endif // EAF_ALGORITHMS_JOFFE_FAST64BIT_H
