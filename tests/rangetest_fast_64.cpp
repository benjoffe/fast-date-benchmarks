// SPDX-License-Identifier: BSL-1.0
// Copyright (c) 2025 Ben Joffe - https://www.benjoffe.com/fast-date-64

#include "eaf/date.hpp"
#include "algorithms/_portable_int128.hpp"
#include "algorithms/_portable_uint128.hpp"
#include <random>
#include <stdint.h>
#include <sstream>
#include <iomanip>
#include <tuple>
#include <stdint.h>
#include <iostream>
#include <limits>


#if defined(__aarch64__) || defined(_M_ARM64)
#define TEST_IS_ARM 1
#else
#define TEST_IS_ARM 0
#endif

/**
 * This is the same as the one in the algorithms folder except updated to: 
 * 1. Accept 64-bit input and 64-bit output year
 * 2. Use the larger constant for ERAS to use a wider range.
 * 3. 64-bit intermediaries where required.
 */
inline date64_t benjoffe_fast64_wide(int64_t dayNumber)
{
  static uint64_t constexpr ERAS = 7453743012;
  static uint64_t constexpr BIAS = 5;

  static int64_t constexpr D_SHIFT = -146097l * ERAS + 719469l;
  static uint64_t constexpr Y_SHIFT = 400 * ERAS;

  static uint64_t constexpr CEN_MUL = 505054698555331ull;          // floor(2^64 / 36524.25)
  static uint64_t constexpr YRS_MUL = 50504432782230120ull + BIAS; // floor(2^64 / 365.25) + BIAS
  static uint64_t constexpr DAY_MUL = 2204962377560594841ull;      // floor(2^64 * 30.6 / 256)
  static uint64_t constexpr YPT_MUL = 3056ull;                     // ceil(365.25 / 30.6 * 256)
    
  // Use different variable sizes in some places for optimal codegen.
  // Does not change any math / overflow characteristics.
  #if defined(__aarch64__) || defined(_M_ARM64)
    using uintXX_t = uint64_t;
  #else
    using uintXX_t = uint32_t;
  #endif

  int64_t const rev = D_SHIFT + dayNumber;
  int64_t const cen = CEN_MUL * int128_t(rev) >> 64;
  int64_t const jul = rev + cen - (cen >> 2);

  int128_t const num = int128_t(jul) * YRS_MUL;
  uint64_t const yrs = uint64_t(num >> 64);
  uint64_t const low = uint64_t(num);
  uintXX_t const ypt = uintXX_t(low * uint128_t(YPT_MUL) >> 64);

  uintXX_t const bump = ypt > 10u * 256;
  int32_t const shift = bump ? 765 - 12 * 256 : 765;

  uint32_t const N = (yrs & 3) * 2 + ypt + shift;
  uint32_t const month = N / 256;
  uint32_t const day = uint32_t(uint64_t((N % 256) * uint128_t(DAY_MUL) >> 64) + 1);
  int64_t const year = int64_t(yrs + bump + Y_SHIFT);

  return date64_t{year, month, day};
}

// SPDX-FileCopyrightText: 2022 Cassio Neri <cassio.neri@gmail.com>
// SPDX-FileCopyrightText: 2022 Lorenz Schneider <schneider@em-lyon.com>
// Function where wide range is known.
// Neri-Schneider algorithm adapted to support a wider 64-bit
// input range which exceeds the range of our new formula
inline date64_t neri_schneider_to_date(int64_t N_U) noexcept
{
  static uint64_t constexpr s = (1ull << 61) / 146097ull;
  static uint64_t constexpr K = 719468ull + 146097ull * s;
  static uint64_t constexpr L = 400ull * s;

  // Rata die shift.
  uint64_t const N = N_U + K;

  // Century.
  uint64_t const N_1 = 4 * N + 3;
  uint64_t const C   = N_1 / 146097ull;
  uint32_t const N_C = (N_1 % 146097ull) / 4;

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
  int64_t  const Y_G = (Y - L) + J;
  uint32_t const M_G = J ? M - 12 : M;
  uint32_t const D_G = D + 1;

  return { Y_G, M_G, D_G };
}

inline bool same_ymd(const date64_t& a, const date64_t& b)
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

int main()
{
  int64_t EXPECT_FAIL_UP =  178800658870273;
  int64_t EXPECT_FAIL_DOWN = -178800658817456;

  int64_t RANGE_CHECK = (1ll << 32);

  int64_t UP_START   =  EXPECT_FAIL_UP - RANGE_CHECK;
  int64_t DOWN_START =  EXPECT_FAIL_DOWN + RANGE_CHECK;
  int64_t FULL_RANGE =  UP_START - DOWN_START;

  int64_t output_freq = 1<<24;

  std::cout << "STARTING UP SEARCH (COUNT: " << RANGE_CHECK << ")\n";
  {
    for (uint64_t i = 0; ; ++i) {
      
      int64_t z = UP_START + i;

      date64_t j = benjoffe_fast64_wide(z);
      date64_t h = neri_schneider_to_date(z);

      if (i % output_freq == 0) {
        std::cout << "\rIterations: " << i << std::flush;
      }

      if (!same_ymd(j, h)) {
        std::cout << "\n" << std::flush;
        std::cout << "UPWARD MISMATCH after "<< i << " successes.\n";
        std::cout << "First failure at z = " << z << "\n";
        std::cout << "Ben Joffe:      " << j.year << "-" << pad2(j.month) << "-" << pad2(j.day) << "\n";
        std::cout << "Neri-Schneider: " << h.year << "-" << pad2(h.month) << "-" << pad2(h.day) << "\n";
        if (z == EXPECT_FAIL_UP) {
          std::cout << "\033[32mPass: This matches expectations.\033[0m\n";
        }
        else {
          std::cout << "\033[31mFail: This does not match expectations. "
                    << "Expected " << EXPECT_FAIL_UP << ".\033[0m\n";
        }
        break;
      }
    }
  }

  std::cout << "STARTING DOWNWARD SEARCH (COUNT: " << RANGE_CHECK << ")\n";
  {
    for (uint64_t i = 0; ; ++i) {
        
      int64_t z = DOWN_START - i;

      date64_t j = benjoffe_fast64_wide(z);
      date64_t h = neri_schneider_to_date(z);

      if (i % output_freq == 0) {
        std::cout << "\rIterations: " << i << std::flush;
      }

      if (!same_ymd(j, h)) {
        std::cout << "\n" << std::flush;
        std::cout << "DOWNWARD MISMATCH after "<< i << " successes.\n";
        std::cout << "First failure at z = " << z << "\n";
        std::cout << "Ben Joffe:      " << j.year << "-" << pad2(j.month) << "-" << pad2(j.day) << "\n";
        std::cout << "Neri-Schneider: " << h.year << "-" << pad2(h.month) << "-" << pad2(h.day) << "\n";
        if (z == EXPECT_FAIL_DOWN) {
          std::cout << "\033[32mPass: This matches expectations.\033[0m\n";
        }
        else {
          std::cout << "\033[31mFail: This does not match expectations. "
                    << "Expected fail at " << EXPECT_FAIL_DOWN << ".\033[0m\n";
        }
        break;
      }
    }
  }

  std::cout << "STARTING SEARCH AROUND ZERO (+- 2^32)\n";
  {
    for (int64_t z = -(1ll << 32); z <= (1ll << 32); ++z) {

      date64_t j = benjoffe_fast64_wide(z);
      date64_t h = neri_schneider_to_date(z);

      if (z % output_freq == 0) {
        std::cout << "\rRata Die: " << z << "       " << std::flush;
      }

      if (!same_ymd(j, h)) {
        std::cout << "\n" << std::flush;
        std::cout << "Mismatch at z = " << z << "\n";
        std::cout << "Ben Joffe:      " << j.year << "-" << pad2(j.month) << "-" << pad2(j.day) << "\n";
        std::cout << "Neri-Schneider: " << h.year << "-" << pad2(h.month) << "-" << pad2(h.day) << "\n";
        std::cout << "\033[31mFail: This does not match expectations." << "\033[0m\n";
        return 0;
      }
    }
  }

  std::cout << "\n" << std::flush;
  std::cout << "\033[32mPass: All dates in range match.\033[0m\n";

  std::mt19937_64 rng(std::random_device{}());
  std::uniform_int_distribution<int64_t> dist(DOWN_START, UP_START);

  std::cout << "STARTING RANDOM SEARCH OF 2^32 DATES:\n";

  for (uint64_t i = 0; i < (1ll << 32); ++i) {
    int64_t z = dist(rng);
    date64_t j = benjoffe_fast64_wide(z);
    date64_t h = neri_schneider_to_date(z);

    if (!same_ymd(j, h)) {
      std::cout << "\033[31mFail: RANDOM MISMATCH at z = " << z << "\033[0m\n";
      std::cout << "Ben Joffe:      " << j.year << "-" << j.month << "-" << j.day << "\n";
      std::cout << "Neri-Schneider: " << h.year << "-" << h.month << "-" << h.day << "\n";
      break;
    }

    if (i % output_freq == 0) {
      std::cout << "\rIterations: " << i << " latest: " << z << " = " 
                << j.year << "-" << pad2(j.month) << "-" << pad2(j.day)
                << "      " << std::flush;
    }
  }

  std::cout << "\n" << std::flush;
  std::cout << "\033[32mPass: All randomly selected dates match.\033[0m\n";

  std::cout << "STARTING FULL DATE SEARCH (this will take a very long time):\n";

  for (int64_t z = DOWN_START; z < UP_START; ++z) {
    date64_t j = benjoffe_fast64_wide(z);
    date64_t h = neri_schneider_to_date(z);

    if (!same_ymd(j, h)) {
      std::cout << "\033[31mFail: MISMATCH at z = " << z << "\033[0m\n";
      std::cout << "Ben Joffe:      " << j.year << "-" << j.month << "-" << j.day << "\n";
      std::cout << "Neri-Schneider: " << h.year << "-" << h.month << "-" << h.day << "\n";
      return 0;
    }

    if (z % output_freq == 0) {
      double progress = double(z - DOWN_START) / double(FULL_RANGE);

      std::cout << "\rRata Die: " << z << " - "
                << "Progress: " << std::fixed << std::setprecision(5)
                << progress * 100.0   // if you want percent
                << "% - latest: " << z << " = " 
                << j.year << "-" << pad2(j.month) << "-" << pad2(j.day)
                << "      " << std::flush;
    }
  }

  std::cout << "\033[32mPass: All dates within range match.\033[0m\n";

  return 0;
}