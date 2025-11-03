// SPDX-License-Identifier: BSL-1.0
// SPDX-SnippetCopyrightText: 2002-2003 CrystalClear Software, Inc.

/**
 * @brief Algorithms on the Gregorian calendar from boost [1].
 *
 * This code is a supplement material to [2].
 * 
 * Duplicate of boost.htpp file with modifications as supplementary material to [3].
 *
 *     [1] https://www.boost.org/
 *
 *     [2] Neri C, and Schneider L, "Euclidean Affine Functions and their
 *     Application to Calendar Algorithms" (2022).
 * 
 *     [3] Ben Joffe - "A New Faster Algorithm for Gregorian Date Conversion"
 *     https://www.benjoffe.com/fast-date
 */

#ifndef EAF_OTHERS_BOOST_JOFFE_1_HPP
#define EAF_OTHERS_BOOST_JOFFE_1_HPP

#include "eaf/date.hpp"

#include <cstdint>

struct boost_joffe_1 {

  // Original epoch: 24 November -4713.
  static constexpr int32_t ajustment = 2472632;

  // https://tinyurl.com/ybq2ozhm
  static inline
  date32_t to_date(int32_t dayNumber) {

    uint32_t a = dayNumber + ajustment;
    uint32_t b = (4*a + 3)/146097;
    uint32_t c = a + b - b/4; // line changed (`c` is now `a` mapped to julian)
    uint32_t d = (4*c + 3)/1461;
    uint32_t e = c - (1461*d)/4;
    uint32_t m = (5*e + 2)/153;
    uint32_t day = static_cast<uint32_t>(e - ((153*m + 2)/5) + 1);
    uint32_t month = static_cast<uint32_t>(m + 3 - 12 * (m/10));
    int32_t year = static_cast<int32_t>(d - 4800 + (m/10)); // removed 100*b term

    return { year, month, day };
  }

  // https://tinyurl.com/ybmm7dzr
  static inline
  int32_t to_rata_die(int32_t year, uint32_t month, uint32_t day) {

    unsigned short a = static_cast<unsigned short>((14-month)/12);
    unsigned short y = static_cast<unsigned short>(year + 4800 - a);
    unsigned short m = static_cast<unsigned short>(month + 12*a - 3);
    unsigned long  d = day + ((153*m + 2)/5) + 365*y + (y/4) - (y/100) + (y/400) - 1;

    return d - ajustment;
  }

}; // struct boost

#endif // EAF_OTHERS_BOOST_JOFFE_1_HPP
