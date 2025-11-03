// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2022 Cassio Neri <cassio.neri@gmail.com>
// SPDX-FileCopyrightText: 2022 Lorenz Schneider <schneider@em-lyon.com>
// SPDX-FileCopyrightText: 2025 Ben Joffe - https://www.benjoffe.com/fast-date

/**
 * @brief A New Faster Algorithm for Gregorian Date Conversion.
 * 
 * Fast month/day conversion from [1].
 * Full algorithm explained in [2].
 * 
 *     [1] Neri C, and Schneider L, "Euclidean Affine Functions and their
 *     Application to Calendar Algorithms" (2022).
 * 
 *     [2] Ben Joffe - "A New Faster Algorithm for Gregorian Date Conversion"
 *     https://www.benjoffe.com/fast-date
 */

#ifndef EAF_ALGORITHMS_JOFFE_H
#define EAF_ALGORITHMS_JOFFE_H

#include "eaf/date.hpp"

#include <stdint.h>

struct joffe {

  static uint32_t constexpr s = 82;
  static uint32_t constexpr K = 719162 + 146097 * s + 306;
  static uint32_t constexpr L = 400 * s;

  static uint32_t constexpr INVERSE_SHIFT_Y = 400 * 14700;
  static uint32_t constexpr INVERSE_SHIFT_RD = 719162 + 146097 * 14700u + 306;

  static inline
  date32_t to_date(int32_t dayNumber) {
    
    uint32_t const d0 = dayNumber + K;

    uint32_t const qds = d0 * 4 + 3;
    uint32_t const cen = qds / 146097;
    uint32_t const jul = qds - (cen & ~ 3) + cen * 4;
    uint32_t const yrs = jul / 1461;
    uint32_t const rem = (jul % 1461) / 4;

    uint32_t const N = rem * 2141 + 197913;
    uint32_t const M = N / 65536;
    uint32_t const D = N % 65536 / 2141;

    uint32_t const bump = (rem >= 306);
    uint32_t const day = D + 1;
    uint32_t const month = bump ? M - 12 : M;
    int32_t const year = (yrs - L) + bump;

    return { year, month, day };
  }

  static inline
  int32_t to_rata_die(int32_t year, uint32_t month, uint32_t day) {

    uint32_t const bump = month <= 2;
    uint32_t const yrs = uint32_t(year + INVERSE_SHIFT_Y) - bump;
    uint32_t const cen = yrs / 100;
    int32_t const phase = bump ? 8829 : -2919;

    // Similar to Neri-Scheinder but slightly slower to avoid early overflow:
    uint32_t const year_days = yrs * 365 + yrs / 4 - cen + cen / 4;
    uint32_t const month_days = (979 * month + phase) / 32;
    
    return year_days + month_days + day - (INVERSE_SHIFT_RD + 1);
  }

}; // struct joffe

#endif // EAF_ALGORITHMS_JOFFE_H
