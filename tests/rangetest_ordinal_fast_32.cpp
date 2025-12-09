// SPDX-License-Identifier: BSL-1.0
// Copyright (c) 2025 Ben Joffe - https://www.benjoffe.com/fast-date-64

#include "eaf/date.hpp"
#include "util/ordinal.hpp"
#include "algorithms_ordinal/ordinal_benjoffe_fast32.hpp"
#include "algorithms_ordinal/ordinal_test.hpp"
#include <random>
#include <stdint.h>
#include <sstream>
#include <iomanip>
#include <tuple>
#include <stdint.h>
#include <iostream>
#include <limits>


inline bool same_ordinal(const ordinal32_t& a, const ordinal32_t& b)
{
    return a.year == b.year &&
           a.doy  == b.doy &&
           a.leap == b.leap;
}

int main()
{
  int64_t output_freq = 1<<24;
  int64_t sum = 0;

  std::cout << "STARTING UP SEARCH\n";
  {
    for (int32_t z = 0; z >= 0; ++z) {

      ordinal32_t j = ordinal_benjoffe_fast32::to_date(z);
      ordinal32_t h = ordinal_test::to_date(z);

      if (z % output_freq == 0) {
        std::cout << "\rIterations: " << z << std::flush;
      }

      if (!same_ordinal(j, h)) {
        std::cout << "\rFirst upward failure at z = " << z << "\n";
        std::cout << "Ben Joffe:       " << j.year << "-" << j.doy << "-" << (j.leap ? "Leap" : "Non-leap") << "\n";
        std::cout << "Test (baseline): " << h.year << "-" << h.doy << "-" << (h.leap ? "Leap" : "Non-leap") << "\n";
        sum += z;
        break;
      }
    }
  }

  std::cout << "STARTING DOWN SEARCH\n";
  {
    for (int32_t z = -1; z <= 0; --z) {

      ordinal32_t j = ordinal_benjoffe_fast32::to_date(z);
      ordinal32_t h = ordinal_test::to_date(z);

      if (z % output_freq == 0) {
        std::cout << "\rIterations: " << z << std::flush;
      }

      if (!same_ordinal(j, h)) {
        std::cout << "\rFirst Downward failure at z = " << z << "\n";
        std::cout << "Ben Joffe:       " << j.year << "-" << j.doy << "-" << (j.leap ? "Leap" : "Non-leap") << "\n";
        std::cout << "Test (baseline): " << h.year << "-" << h.doy << "-" << (h.leap ? "Leap" : "Non-leap") << "\n";

        sum += -z - 1;
        break;
      }
    }
  }

  std::cout << "Coverage: " << sum << "\n";

  return 0;
}