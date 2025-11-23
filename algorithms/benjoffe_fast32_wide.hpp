// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2022 Cassio Neri <cassio.neri@gmail.com>
// SPDX-FileCopyrightText: 2022 Lorenz Schneider <schneider@em-lyon.com>
// SPDX-FileCopyrightText: 2025 Ben Joffe - https://www.benjoffe.com/fast-date

/**
 * @brief A New Faster Algorithm for Gregorian Date Conversion.
 * 
 * Fast month/day conversion from [1].
 * Modified as supplementary material to [2] and [3]:
 * 
 *     [1] Neri C, and Schneider L, "Euclidean Affine Functions and their
 *     Application to Calendar Algorithms" (2022).
 * 
 *     [2] Ben Joffe - "A New Faster Overflow-Safe Date Algorithm"
 *     https://www.benjoffe.com/safe-date
 * 
 *     [3] Ben Joffe - "A Very Fast 64–Bit Date Algorithm"
 *     https://www.benjoffe.com/fast-date-64
 */

#ifndef EAF_ALGORITHMS_BENJOFFE_FAST32_WIDE_H
#define EAF_ALGORITHMS_BENJOFFE_FAST32_WIDE_H

#include "eaf/date.hpp"

#include <stdint.h>

#if defined(__aarch64__) || defined(_M_ARM64)
#define IS_ARM 1
#else
#define IS_ARM 0
#endif

struct benjoffe_fast32_wide {

  static uint32_t constexpr K = 146097*5 - 719162 - 307 + 3845;
  // Note: 14694 is intentional (see article [2])
  static uint32_t constexpr L = 14694*400 + 1;

  // Bucket technique explained in article [2]
  // Note: when counting backwards, a bucket size can correspond
  // to only one 400-year era, instead of 7 as discussed in the article.
  static uint32_t constexpr BUCK_Y = 400;
  static uint32_t constexpr BUCK_D = 146097;

  static inline
  date32_t to_date(int32_t dayNumber) {
    
    uint32_t const d0 = dayNumber + 2147483648;

    // Bucket technique explained in article [2]
    uint32_t const bucket = d0 >> 17;

    // Backwards counting technique explained in article [3]
    uint32_t const rev = bucket*BUCK_D - d0 + K;
    uint32_t const cen = rev * 3853261555ull >> 47;   // 2^47*4/146097 = 3853261555.1
    uint32_t const jul = rev + cen - cen / 4;
    uint32_t const yrs = (jul * 3010298776ull) >> 40; // 2^40*4/146097 = 30103605.9
    uint32_t const rem = jul - yrs * 1461 / 4;

  #if IS_ARM
    uint32_t const shift = 979360;                       
  #else
    // Jan/Feb cutoff when counting backwards:
    uint32_t const bump = rem <= 59;
    uint32_t const shift = bump ? 192928 : 979360;
  #endif

    // Neri-Schneider technique for Day and Month [1]:
    uint32_t const N = shift - rem * 2141;
    uint32_t const M = N / 65536;
    uint32_t const D = ((N % 65536) * 2006057ull) >> 32;

  #if IS_ARM
    uint32_t const bump = M > 12;
    uint32_t const month = bump ? M - 12 : M;
  #else
    uint32_t const month = M;
  #endif

    uint32_t const day = D + 1;
    int32_t const year = BUCK_Y*bucket - L - yrs + bump;

    return { year, month, day };
  }

  // The below is identical to benjoffe_fast64.hpp
  // It is therefore excluded in the to_rata_die benchmarks
  static inline
  int32_t to_rata_die(int32_t year, uint32_t month, uint32_t day) {

    uint32_t const bump = month <= 2;
    uint32_t const yrs = uint32_t(year + 5880000) - bump;
    uint32_t const cen = yrs / 100;
    int32_t const shift = bump ? 8829 : -2919;

    // Similar to Neri-Scheinder but slightly slower to avoid early overflow:
    uint32_t const year_days = yrs * 365 + yrs / 4 - cen + cen / 4;
    uint32_t const month_days = (979 * int32_t(month) + shift) / 32;
    
    return year_days + month_days + day - 2148345369u;
  }

}; // struct benjoffe_fast32_wide

#undef IS_ARM

#endif // EAF_ALGORITHMS_BENJOFFE_FAST32_WIDE_H
