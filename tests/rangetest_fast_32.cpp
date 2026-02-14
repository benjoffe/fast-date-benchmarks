// SPDX-License-Identifier: BSL-1.0
// Copyright (c) 2025 Ben Joffe - https://www.benjoffe.com/fast-date-64

#include "eaf/date.hpp"
#include "algorithms/benjoffe_fast64_v2.hpp"
#include <random>
#include <stdint.h>
#include <sstream>
#include <iomanip>
#include <tuple>
#include <stdint.h>
#include <iostream>
#include <limits>

// SPDX-FileCopyrightText: 2022 Cassio Neri <cassio.neri@gmail.com>
// SPDX-FileCopyrightText: 2022 Lorenz Schneider <schneider@em-lyon.com>
// Function where wide range is known.
// Neri-Schneider algorithm adapted to support a wider 64-bit
// input range which exceeds the range of our new formula
inline date32_t neri_schneider_to_date(int32_t N_U) noexcept
{
  static uint64_t constexpr s = ((1ull << 61) / 146097ull);
  static uint64_t constexpr K = 719468ull + 146097ull * s;
  static uint64_t constexpr L = 400ull * s;

  // Rata die shift.
  uint64_t const N = int64_t(N_U) + int64_t(K);

  // Century.
  uint64_t const N_1 = 4 * N + 3;
  uint64_t const C   = N_1 / 146097ull;
  uint32_t const N_C = uint32_t((N_1 - 146097ull * C) / 4ull);

  // Year.
  uint32_t const N_2 = 4 * N_C + 3;
  uint64_t const P_2 = uint64_t(2939745) * N_2;
  uint32_t const Z   = uint32_t(P_2 / 4294967296);
  uint32_t const N_Y = uint32_t(P_2 % 4294967296) / 2939745 / 4;
  uint64_t const Y   = 100 * C + Z;

  // Month and day.
  uint32_t const N_3 = 2141 * N_Y + 197913;
  uint32_t const M   = N_3 / 65536;
  uint32_t const D   = N_3 % 65536 / 2141;

  // Map. (Notice the year correction, including type change.)
  uint32_t const J   = N_Y >= 306;
  int32_t  const Y_G = int32_t((Y - L) + J);
  uint32_t const M_G = J ? M - 12 : M;
  uint32_t const D_G = D + 1;

  return { Y_G, M_G, D_G };
}

inline bool same_ymd(const date32_t& a, const date32_t& b)
{
  return a.year  == b.year &&
         a.month == b.month &&
         a.day   == b.day;
}

std::string pad2(int x) {
    std::ostringstream ss;
    ss << std::setw(2) << std::setfill('0') << x;
    return ss.str();
}

using date_fn = date32_t (*)(int32_t);

int run_search(const char* label,
                date_fn test_fn,
                date_fn ref_fn)
{
  int64_t output_freq = 1<<24;
  int64_t sum = 0;

  std::cout << "STARTING UP SEARCH: " 
    "\033[33m" << label << "\033[0m" << "\n";
  {
    for (int32_t z = 0; z >= 0; ++z) {

      date32_t j = test_fn(z);
      date32_t h = ref_fn(z);

      if (z % output_freq == 0) {
        std::cout << "\rIterations: " << z << std::flush;
      }

      if (!same_ymd(j, h)) {
        std::cout << "\n" << std::flush;
        std::cout << "First upward failure at z = " << z << "\n";
        std::cout << "Ben Joffe:      " << j.year << "-" << pad2(j.month) << "-" << pad2(j.day) << "\n";
        std::cout << "Neri-Schneider: " << h.year << "-" << pad2(h.month) << "-" << pad2(h.day) << "\n";

        sum += z;
        break;
      }
    }
  }

  std::cout << "STARTING DOWN SEARCH\n";
  {
    for (int32_t z = -1; z <= 0; --z) {

      date32_t j = test_fn(z);
      date32_t h = ref_fn(z);

      if (z % output_freq == 0) {
        std::cout << "\rIterations: " << z << std::flush;
      }

      if (!same_ymd(j, h)) {
        std::cout << "\n" << std::flush;
        std::cout << "First Downward failure at z = " << z << "\n";
        std::cout << "Ben Joffe:      " << j.year << "-" << pad2(j.month) << "-" << pad2(j.day) << "\n";
        std::cout << "Neri-Schneider: " << h.year << "-" << pad2(h.month) << "-" << pad2(h.day) << "\n";

        sum += -z;
        break;
      }
    }
  }

  --sum; // overcounted by 1

  double percent = double(sum) / double(1ull << 32);

  std::cout << "\033[32m"
            << "Coverage: " << sum
            << " / 2^32"
            << " = " << std::fixed << std::setprecision(8)
            << (percent * 100.0) << "%"
            << "\033[0m" << "\n";

  std::cout << "-------------------\n";

  return 0;
}

int main() {

  run_search(
    "benjoffe_fast64_v2",
    benjoffe_fast64_v2::to_date,
    neri_schneider_to_date
  );
  
  return 0;
}