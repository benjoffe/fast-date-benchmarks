// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2022 Cassio Neri <cassio.neri@gmail.com>
// SPDX-FileCopyrightText: 2022 Lorenz Schneider <schneider@em-lyon.com>
// SPDX-FileCopyrightText: 2025 Ben Joffe - https://www.benjoffe.com/fast-date

/**
 * @brief A New Faster Algorithm for Gregorian Date Conversion.
 * 
 * Fast month/day conversion from [1].
 * Full algorithm will be explained in a followup blog post to [2].
 * 
 *     [1] Neri C, and Schneider L, "Euclidean Affine Functions and their
 *     Application to Calendar Algorithms" (2022).
 * 
 *     [2] Ben Joffe - "A New Faster Algorithm for Gregorian Date Conversion"
 *     https://www.benjoffe.com/fast-date
 */

#ifndef EAF_ALGORITHMS_JOFFE_FAST64BIT_H
#define EAF_ALGORITHMS_JOFFE_FAST64BIT_H

#include "eaf/date.hpp"

#include <stdint.h>

struct joffe_fast64bit {

  // IMPORTANT NOTE: This file is not yet included by default in the benchmarks, becuase
  // it does run not cross-platforrm. Compiles fine on Mac but may need changes for other
  // platforms due to the way 128 bit multiplications work.

  // this is a placeholder algorithm that will be cleaned up and explained further in
  // a followup blog post.
  // It seems to be significantly fast on Macbook Pro M4 Pro, being around 20% faster
  // than Neri-Schneider.
  // It shifts the date of epoch by 307 instead of 306, which removes the need
  // to add 3 in the calculation of 'cent', this allows the multiplication and division
  // to also be merged together in a single mul-shift to calculate century.
  // The shift by 307 necessitates the addition of 365 on the next line, however
  // the removal of +3 and multiplcation by 4 continues further down the chain, removing
  // additional cycles.

  // This seems to give a very large range in 64-bit, but far from the full range.
  // The shortfall is small enough however to allow this technique to be used to 
  // cover the full 64-bit Unix time range, where seconds/minutes/hours take up
  // a reasonable number of bits.

  // Specific benchmark numbers and full explaination will be provided later.

  // Todo: the below offsets will be tweaked
  static uint32_t constexpr s = 82;
  static uint32_t constexpr K = 719162 + 146097 * s + 307;
  static uint32_t constexpr L = 400 * s + 1;

  // 2^64 * 4 / 146097 = 505054698555331.09
  static uint64_t constexpr FEA_MUL_1 = 505054698555331ll;
  // 2^64 * 4 / 1461 = 50504432782230120.8
  static uint64_t constexpr FEA_MUL_2 = 50504432782230121ll;

  static inline
  date32_t to_date(int32_t dayNumber) {
    
    uint32_t const d0 = dayNumber + K;

    //uint32_t const cent = d0 * 4 / 146097;
    uint32_t const cent = uint32_t((__uint128_t(FEA_MUL_1) * d0) >> 64);
    uint32_t const jul = d0 + 365 - cent / 4 + cent;

    __uint128_t const num = __uint128_t(FEA_MUL_2) * jul;
    uint32_t const yrs = uint32_t(num >> 64);
    uint32_t const rem = uint32_t(uint64_t(num) / FEA_MUL_2);

    uint32_t const N = rem * 2141 + 197913;
    uint32_t const D = (N % 65536) / 2141;
    uint32_t const M = N / 65536;

    uint32_t const bump = (M > 12);
    uint32_t const day = D + 1;
    uint32_t const month = bump ? M - 12 : M;
    int32_t const year = yrs + bump - L;

    return { year, month, day };
  }

  // below is identical to joffe.hpp
  // it is excluded in the to_rata_die benchmark for this reason
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

}; // struct joffe_fast64bit

#endif // EAF_ALGORITHMS_JOFFE_FAST64BIT_H
