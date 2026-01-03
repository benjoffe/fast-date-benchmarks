// SPDX-License-Identifier: BSL-1.0
// SPDX-SnippetCopyrightText: 2026 Ben Joffe - https://www.benjoffe.com/fast-ordinal-date

#ifndef EAF_ALGORITHMS_BENJOFFE_ORDINAL_ALTERNATIVE_H
#define EAF_ALGORITHMS_BENJOFFE_ORDINAL_ALTERNATIVE_H

#include "eaf/date.hpp"
#include "util/ordinal.hpp"
#include "algorithms_ordinal/ordinal_benjoffe_fast64.hpp"
#include "algorithms/benjoffe_fast64.hpp"

#include <stdint.h>

#if defined(__aarch64__) || defined(_M_ARM64)
#define IS_ARM 1
#else
#define IS_ARM 0
#endif

struct benjoffe_ordinal_alternative {

  // This is a testcase for converting year/ordinal/leap to month/day
  // It uses another ordinal algorithm to establish the year/ordinal/leap
  // from the rata die for simplicity.
  //
  // See the 'algorithms_ordinal' folder for general rata-die to year/ordinal/leap algorithms.
  //
  // This is built as a standard YMD algorithm in this codebase for two reasons:
  // 1. I haven't spent time creating a test suite for this specific scenario
  // 2. It is interesting to see what the overall "penalty" is for calculating dates this way.

#if IS_ARM
  // ARM benefits from smaller constants
  static uint32_t constexpr SCALE = 1;
#else
  // Use larger constants on x86, resulting in a DIVISOR
  // of 2^16 which has speed benefits on this platform.
  static uint32_t constexpr SCALE = 2;
#endif

  static uint32_t constexpr STEP = 1071 * SCALE;
  static uint32_t constexpr DIVISOR = SCALE << 15;
  static uint32_t constexpr SHIFT_0 = DIVISOR - 439 * SCALE;
  static uint32_t constexpr SHIFT_1 = SHIFT_0 + STEP;
  static uint32_t constexpr SHIFT_2 = SHIFT_1 + STEP;

  static inline
  date32_t to_date(int32_t dayNumber) {

    // Reuse other fast algorithm for setting up the variables we need
    ordinal32_t const alt = ordinal_benjoffe_fast64::to_date(dayNumber);
    int32_t const year = alt.year;
    uint32_t const ordinal = alt.ordinal;
    bool const leap = alt.leap;
    
    // The logic below is where this algorithm really begins.
    // It is similar to the algorithm presented in Calendrical Calculations, but:
    // 1. Performs a shift after multiplication by STEP instead of prior
    // 2. Uses a scaled ratio so that the divisor is a power of 2 (as used already by time-rs).
    // 3. Uses the Neri-Schneider technique to use high and low parts of multiplication.
    // 4. Uses platform specific scale for micro optimisations (ARM vs x86).

    uint32_t jan_feb_len = 59 + leap;
    uint32_t shift = ordinal <= jan_feb_len ? SHIFT_0 : (leap ? SHIFT_1 : SHIFT_2);
    uint32_t num = ordinal * STEP + shift;
    uint32_t month = num / DIVISOR;
    uint32_t day = (num % DIVISOR) / STEP + 1;

    return { year, month, day };
  }

  static inline
  int32_t to_rata_die(int32_t year, uint32_t month, uint32_t day) {
    return benjoffe_fast64::to_rata_die(year, month, day);
  }

}; // struct benjoffe_ordinal_alternative

#undef IS_ARM

#endif // EAF_ALGORITHMS_BENJOFFE_ORDINAL_ALTERNATIVE_H
