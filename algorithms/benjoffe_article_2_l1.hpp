// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2022 Cassio Neri <cassio.neri@gmail.com>
// SPDX-FileCopyrightText: 2022 Lorenz Schneider <schneider@em-lyon.com>
// SPDX-FileCopyrightText: 2025 Ben Joffe - https://www.benjoffe.com/fast-date

/**
 * @brief A New Faster Algorithm for Gregorian Date Conversion.
 * 
 * Fast month/day conversion from [1].
 * Modified as supplementary material to [2]:
 * 
 *     [1] Neri C, and Schneider L, "Euclidean Affine Functions and their
 *     Application to Calendar Algorithms" (2022).
 * 
 *     [2] Ben Joffe - "A New Faster Overflow-Safe Date Algorithm"
 *     https://www.benjoffe.com/safe-date
 */

#ifndef EAF_ALGORITHMS_BENJOFFE_ARTICLE_2_L1_H
#define EAF_ALGORITHMS_BENJOFFE_ARTICLE_2_L1_H

#include "eaf/date.hpp"

#include <stdint.h>

struct benjoffe_article_2_l1 {
  
  static uint32_t constexpr INVERSE_SHIFT_Y = 400 * 14700;
  static uint32_t constexpr INVERSE_SHIFT_RD = 719162 + 146097 * 14700u + 306;
  
  static int32_t constexpr OFFSETS[16] = {
                 // [0-7] Days Shifts
         719468, // 719468 + 14696 * 146097 - 4 * 3674 * 146097
     -536040910, // 719468 + 14696 * 146097 - 5 * 3674 * 146097
    -1072801288, // 719468 + 14696 * 146097 - 6 * 3674 * 146097
    -1609561666, // 719468 + 14696 * 146097 - 7 * 3674 * 146097
    -2147206316, // 719468 + 14696 * 146097 - 0 * 3674 * 146097 - 2^32
     1611000602, // 719468 + 14696 * 146097 - 1 * 3674 * 146097
     1074240224, // 719468 + 14696 * 146097 - 2 * 3674 * 146097
      537479846, // 719468 + 14696 * 146097 - 3 * 3674 * 146097

                 // [8-15] Year Shifts.
              0, // 14696*400 - 4 * 3674 * 400
       -1469600, // 14696*400 - 5 * 3674 * 400
       -2939200, // 14696*400 - 6 * 3674 * 400
       -4408800, // 14696*400 - 7 * 3674 * 400
        5878400, // 14696*400 - 0 * 3674 * 400
        4408800, // 14696*400 - 1 * 3674 * 400
        2939200, // 14696*400 - 2 * 3674 * 400
        1469600  // 14696*400 - 3 * 3674 * 400
  };

  static inline
  date32_t to_date(int32_t dayNumber) {

    uint32_t const uday = static_cast<unsigned>(dayNumber);
    uint32_t const bucket = uday >> 29; // [0, 7]
      
    uint32_t const d_off = OFFSETS[bucket];
    uint32_t const y_off = OFFSETS[bucket+8];

    uint32_t const days = dayNumber + d_off;

    uint32_t const qds = days * 4 + 3;
    uint32_t const cen = qds / 146097;
    uint32_t const jul = qds - (cen & ~3) + cen * 4;
    uint32_t const yrs = jul / 1461;
    uint32_t const rem = jul % 1461 / 4;

    // Neri-Schneider technique for Day & Month:
    uint32_t const N = rem * 2141 + 197913;
    uint32_t const M = N / 65536;
    uint32_t const D = N % 65536 / 2141;

    uint32_t const bump = (rem >= 306);
    uint32_t const day = D + 1;
    uint32_t const month = bump ? M - 12 : M;
    
    int32_t const year = yrs - y_off + bump;

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

}; // struct benjoffe_article_2_l1

#endif // EAF_ALGORITHMS_BENJOFFE_ARTICLE_2_L1_H
