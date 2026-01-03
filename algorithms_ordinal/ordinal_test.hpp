// SPDX-License-Identifier: BSL-1.0
// Copyright (c) 2025 Ben Joffe - https://www.benjoffe.com/fast-date

#ifndef EAF_ALGORITHMS_ORDINAL_TEST_H
#define EAF_ALGORITHMS_ORDINAL_TEST_H

#include "util/ordinal.hpp"
#include "algorithms/benjoffe_fast64.hpp"

#include <stdint.h>

struct ordinal_test {

  /**
   * This test version uses known accurate YMD algorithms to derive
   * the year, day-of-year, and leap. It is not intended to be fast.
   */

  static inline
  ordinal32_t to_date(int32_t dayNumber) {

    date32_t const ymd = benjoffe_fast64::to_date(dayNumber);
    int32_t const year = ymd.year;
    int32_t const rd_y0 = benjoffe_fast64::to_rata_die(year, 1, 1);
    int32_t const rd_y1 = benjoffe_fast64::to_rata_die(year + 1, 1, 1);

    uint32_t const ordinal = uint32_t(dayNumber - rd_y0) + 1;
    bool const leap = (rd_y1 - rd_y0) == 366;

    return ordinal32_t{year, ordinal, leap};
  }

}; // struct ordinal_test

#endif // EAF_ALGORITHMS_ORDINAL_TEST_H