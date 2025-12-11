// SPDX-License-Identifier: BSL-1.0
// Copyright (c) 2025 Ben Joffe - https://www.benjoffe.com/fast-date-64

#include <stdint.h>
#include <sstream>
#include <iomanip>
#include <tuple>
#include <iostream>
#include <limits>
#include <thread>
#include <chrono>
#include <vector>
#include <random>

using highres = std::chrono::high_resolution_clock;

static std::vector<int32_t> RANDOM_YEARS;
static constexpr size_t RANDOM_COUNT = 500'000'000;

inline bool isleap16_textbook(int16_t year)
{
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

inline bool isleap32_textbook(int32_t year)
{
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

inline bool u_isleap32_textbook(uint32_t year)
{
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

// Wide-range leap-year logic reimplemented from
// Neri & Schneider (Overload 155).
// See: https://github.com/cassioneri/calendar/blob/master/calendar.hpp
inline bool isleap32_cassioneri(int32_t year)
{
  const bool is_cen = year % 100 == 0;
  
  return (year & (is_cen ? 15 : 3)) == 0;
}

// Wide-range leap-year logic reimplemented from
// Neri & Schneider (Overload 155).
// See: https://github.com/cassioneri/calendar/blob/master/calendar.hpp
inline bool isleap64_cassioneri(int64_t year)
{
  const bool is_cen = year % 100ll == 0ll;

  return (year & (is_cen ? 15ll : 3ll)) == 0ll;
}

/**
 * Determine whether a signed 32-bit year is a leap year.
 * Accurate over the full range: -2^31 .. 2^31-1.
 *
 * This implements a constant-time century test for the rule (year % 100 == 0),
 * using a multiply-and-cutoff technique adapted to the signed 32-bit domain.
 *
 * Note: the multiplier for 100 is the standard 32-bit reciprocal constant.
 * The bias and cutoff arrangement for fully-correct signed inputs are specific
 * to this method.
 * 
 * The final check against the year, using a modulus of 16 or 4, is the
 * technique developed by Neri Cassio.
 */
inline bool isleap32_benjoffe(int32_t year)
{
  // 32-bit reciprocal of 100 (division-by-constant constant)
  // Value: 42,949,673
  static constexpr uint32_t CEN_MUL = uint32_t((1ull << 32) / 100) + 1;
  
  // Cutoff selected to isolate the `%100 == 0` remainder
  // after domain biasing and 32-bit wrap.
  // Value: 171,798,692
  static constexpr uint32_t CEN_CUTOFF = CEN_MUL * 4;
  
  // Signed → unsigned domain shift. A multiple of 100 near 2^31
  // so that `%100` residues remain aligned after bias.
  // Value: 2,147,483,600
  static constexpr uint32_t CEN_BIAS = CEN_MUL / 2 * 100;

  const uint32_t low = uint32_t(year + CEN_BIAS) * CEN_MUL;
  const bool cen_check = low < CEN_CUTOFF;
  return (year % (cen_check ? 16 : 4)) == 0;
}

inline bool u_isleap32_benjoffe(uint32_t year)
{
  static constexpr uint32_t CEN_MUL = uint32_t((1ull << 32) / 100 + 1);
  static constexpr uint32_t CEN_CUTOFF = CEN_MUL * 4;
  const uint32_t low = year * CEN_MUL;
  const bool cen_check = low < CEN_CUTOFF;
  return (year % (cen_check ? 16 : 4)) == 0;
}

inline bool isleap16_benjoffe(int16_t year)
{
  // 16-bit constant selection thanks to reddit user `sporule`
  static constexpr uint16_t CEN_MUL = 23593;
  static constexpr uint16_t CEN_CUTOFF = 2622;
  static constexpr uint16_t CEN_BIAS = (1ull<<15) / 100 * 100;

  const uint16_t low = uint16_t(year + CEN_BIAS) * CEN_MUL;
  const bool cen_check = low < CEN_CUTOFF;
  return (year % (cen_check ? 16 : 4)) == 0;
}

inline bool isleap64_benjoffe(int64_t year)
{
  // 64-bit constant selection thanks to reddit user `sporule`
  static constexpr uint64_t CEN_MUL = 1106804644422573097ull;
  static constexpr uint64_t CEN_CUTOFF = 737869762948382065ull;
  static constexpr uint64_t CEN_BIAS = (1ull<<63) / 100 * 100;

  const uint64_t low = uint64_t(year + CEN_BIAS) * CEN_MUL;
  const bool cen_check = low < CEN_CUTOFF;
  return (year % (cen_check ? 16 : 4)) == 0;
}


template <typename F>
uint64_t bench(F fn) {
    uint64_t acc = 0;

    // Loop over pre-generated random years
    for (size_t i = 0; i < RANDOM_COUNT; ++i) {
        acc += fn(RANDOM_YEARS[i]);
    }

    return acc;
}

auto warm_textbook = [](uint64_t sink){
    for (uint32_t i = 0; i < 50'000'000; i++) sink += isleap32_textbook(int32_t(i));
    return sink;
};
auto warm_cassioneri = [](uint64_t sink){
    for (uint32_t i = 0; i < 50'000'000; i++) sink += isleap32_cassioneri(int32_t(i));
    return sink;
};
auto warm_benjoffe = [](uint64_t sink){
    for (uint32_t i = 0; i < 50'000'000; i++) sink += isleap32_benjoffe(int32_t(i));
    return sink;
};
auto warm_benjoffe_usigned = [](uint64_t sink){
    for (uint32_t i = 0; i < 50'000'000; i++) sink += u_isleap32_benjoffe(int32_t(i));
    return sink;
};

uint64_t bench_textbook(uint64_t sink)
{
  sink = warm_textbook(sink);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  {
      auto t1 = highres::now();
      uint64_t r = bench(isleap32_textbook);
      auto t2 = highres::now();
      sink += r;
      std::cout << "Textbook:       " << std::chrono::duration<double>(t2 - t1).count() << "\n";
  }
  return sink;
}

uint64_t bench_cassioneri(uint64_t sink)
{
  sink = warm_cassioneri(sink);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  {
      auto t1 = highres::now();
      uint64_t r = bench(isleap32_cassioneri);
      auto t2 = highres::now();
      sink += r;
      std::cout << "Cassio Neri:    " << std::chrono::duration<double>(t2 - t1).count() << "\n";
  }
  return sink;
}

uint64_t bench_benjoffe(uint64_t sink)
{
  sink = warm_benjoffe(sink);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  {
      auto t1 = highres::now();
      uint64_t r = bench(isleap32_benjoffe);
      auto t2 = highres::now();
      sink += r;
      std::cout << "Ben Joffe:    " << std::chrono::duration<double>(t2 - t1).count() << "\n";
  }
  return sink;
}

uint64_t bench_benjoffe_usigned(uint64_t sink)
{
  sink = warm_benjoffe_usigned(sink);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  {
      auto t1 = highres::now();
      uint64_t r = bench(u_isleap32_benjoffe);
      auto t2 = highres::now();
      sink += r;
      std::cout << "Ben Joffe (unsigned):    " << std::chrono::duration<double>(t2 - t1).count() << "\n";
  }
  return sink;
}

template <typename T, typename Fref, typename Ftest>
uint64_t run_search(const char* label,
                Fref ref_fn,
                Ftest test_fn,
                T domain_min,
                T domain_max)
{
    constexpr int32_t output_freq = 1 << 24;

    uint64_t passCount = 0;
    bool hasError = false;
    bool isUnsigned = domain_min == 0;

    for (int32_t dir = 1; dir >= -1; dir -= 2) {

        if (dir < 0 && isUnsigned) {
          // unsigned
          break;
        }

        std::cout << "STARTING "
                  << (dir > 0 ? "UPWARD" : "DOWNWARD")
                  << " SEARCH \033[33m(" << label << ")\033[0m\n";

        T yStart = (dir > 0 ? 0 : -1);
        T yEnd   = (dir > 0 ? domain_max : domain_min);

        for (T y = yStart; ; y += dir) {

            bool ref  = ref_fn(y);
            bool test = test_fn(y);

            if (passCount % output_freq == 0 || ref != test || y == yEnd) {
                std::cout << "\rTested: " << yStart << " to " << y << std::flush;
            }

            if (ref != test) {
                std::cout << "\nFailure at y = " << y << "\n"
                          << "    * Expected: " << (ref  ? "Leap" : "Non-leap") << "\n"
                          << "    * Tested:   " << (test ? "Leap" : "Non-leap") << "\n";
                hasError = true;
                break;
            }

            ++passCount;

            if (y == yEnd) {
                std::cout << "\n";
                std::cout << "\033[32mPass: " << label
                              << " full " << (isUnsigned ? "" : "half-") << "domain " << yStart << " → " << yEnd
                              << "\033[0m\n";
                break;
            }
        }
    }

    std::cout << "Coverage: ";
    if (!hasError) {
      std::cout << "\033[32m100%\033[0m";
    }
    else {
      std::cout << "\033[36m" << passCount << " years\033[0m";
    }
    std::cout << " of " << label << " domain.\n";

    return passCount;
}

int run_search_64bit()
{
  uint64_t RANGE_CHECK = (1ll << 32);

  int64_t UP_START   =  INT64_MAX - RANGE_CHECK;
  int64_t DOWN_START =  INT64_MIN + RANGE_CHECK;
  int64_t FULL_RANGE =  UP_START - DOWN_START;

  int64_t output_freq = 1<<24;

  std::cout << "STARTING UP SEARCH \033[33m(64-BIT)\033[0m (COUNT: " << RANGE_CHECK << ")\n";
  {
    for (uint64_t i = 0; ; ++i) {
      
      if (i > RANGE_CHECK) {
        std::cout << "\n\033[32mTop of range passed." << "\033[0m\n";
        break;
      }

      int64_t z = UP_START + i;

      bool j = isleap64_benjoffe(z);
      bool h = isleap64_cassioneri(z);

      if (i % output_freq == 0 || j != h) {
        std::cout << "\rTested: " << UP_START << " to " << z << std::flush;
      }

      if (j != h) {
        std::cout << "\n" << std::flush;
        std::cout << "UPWARD MISMATCH after "<< i << " successes.\n";
        std::cout << "First failure at z = " << z << "\n";
        std::cout << "Ben Joffe:      " << (j ? "Leap" : "Non-leap") << "\n";
        std::cout << "Neri-Schneider: " << (h ? "Leap" : "Non-leap")<< "\n";
        std::cout << "\033[31mFail: This does not match expectations. " << ".\033[0m\n";
        break;
      }
    }
  }

  std::cout << "STARTING DOWNWARD SEARCH \033[33m(64-BIT)\033[0m (COUNT: " << RANGE_CHECK << ")\n";
  {
    for (uint64_t i = 0; ; ++i) {
          
      if (i > RANGE_CHECK) {
        std::cout << "\n\033[32mBottom of range passed." << "\033[0m\n";
        break;
      }

      int64_t z = DOWN_START - i;

      bool j = isleap64_benjoffe(z);
      bool h = isleap64_cassioneri(z);

      if (i % output_freq == 0 || j != h) {
        std::cout << "\rTested: " << DOWN_START << " to " << z << std::flush;
      }

      if (j != h) {
        std::cout << "\n" << std::flush;
        std::cout << "DOWNWARD MISMATCH after "<< i << " successes.\n";
        std::cout << "First failure at z = " << z << "\n";
        std::cout << "Ben Joffe:      " << (j ? "Leap" : "Non-leap") << "\n";
        std::cout << "Neri-Schneider: " << (h ? "Leap" : "Non-leap")<< "\n";
        std::cout << "\033[31mFail: This does not match expectations. " << ".\033[0m\n";
        break;
      }      
    }
  }

  std::cout << "STARTING SEARCH AROUND ZERO \033[33m(64-BIT)\033[0m (+- 2^32)\n";
  {
    for (int64_t z = -(1ll << 32); z <= (1ll << 32); ++z) {

      bool j = isleap64_benjoffe(z);
      bool h = isleap64_cassioneri(z);

      if (z % output_freq == 0) {
        std::cout << "\rRata Die: " << z << "       " << std::flush;
      }

      if (j != h) {
        std::cout << "\n" << std::flush;
        std::cout << "Mismatch at z = " << z << "\n";
        std::cout << "Ben Joffe:      " << (j ? "Leap" : "Non-leap") << "\n";
        std::cout << "Neri-Schneider: " << (h ? "Leap" : "Non-leap")<< "\n";
        std::cout << "\033[31mFail: This does not match expectations." << "\033[0m\n";
        return 0;
      }
    }
  }

  std::cout << "\n" << std::flush;
  std::cout << "\033[32mPass: Years around zero passed.\033[0m\n";

  std::mt19937_64 rng(std::random_device{}());
  std::uniform_int_distribution<int64_t> dist(DOWN_START, UP_START);

  std::cout << "STARTING RANDOM SEARCH OF 2^32 DATES \033[33m(64-BIT)\033[0m:\n";

  for (uint64_t i = 0; i < (1ll << 32); ++i) {
    int64_t z = dist(rng);
    
    bool j = isleap64_benjoffe(z);
    bool h = isleap64_cassioneri(z);

    if (j != h) {
      std::cout << "\033[31mFail: RANDOM MISMATCH at z = " << z << "\033[0m\n";
      std::cout << "Ben Joffe:      " << (j ? "Leap" : "Non-leap") << "\n";
      std::cout << "Neri-Schneider: " << (h ? "Leap" : "Non-leap")<< "\n";
      break;
    }

    if (i % output_freq / 256 == 0) {
      std::cout << "\rIterations: " << i << std::flush;
    }
  }

  std::cout << "\n" << std::flush;
  std::cout << "\033[32mPass: All randomly selected years match.\033[0m\n";

  std::cout << "STARTING FULL DATE SEARCH \033[33m(64-BIT)\033[0m (this will take a very long time):\n";

  for (int64_t z = DOWN_START; z < UP_START; ++z) {
    bool j = isleap64_benjoffe(z);
    bool h = isleap64_cassioneri(z);

    if (j != h) {
      std::cout << "\033[31mFail: MISMATCH at z = " << z << "\033[0m\n";
      std::cout << "Ben Joffe:      " << (j ? "Leap" : "Non-leap") << "\n";
      std::cout << "Neri-Schneider: " << (h ? "Leap" : "Non-leap")<< "\n";
      return 0;
    }

    if (z % output_freq == 0) {
      double progress = double(z - DOWN_START) / double(FULL_RANGE);

      std::cout << "\rRata Die: " << z << " - "
                << "Progress: " << std::fixed << std::setprecision(8)
                << progress * 100.0   // if you want percent
                << "%" << std::flush;
    }
  }

  std::cout << "\033[32mPass: All years within range match.\033[0m\n";

  return 0;
}

int main()
{
  {
    RANDOM_YEARS.reserve(RANDOM_COUNT);

    std::mt19937 rng(0xDEADBEEF);
    std::uniform_int_distribution<int32_t> dist(std::numeric_limits<int32_t>::min(),
                                                std::numeric_limits<int32_t>::max());

    for (size_t i = 0; i < RANDOM_COUNT; ++i)
        RANDOM_YEARS.push_back(dist(rng));
  }

  volatile uint64_t sink = 0;

  std::cout << "\r\033[33m" << "Benchmarking all three functions, this might take a short while..." << "\033[0m\n";
  std::cout << "The numbers represent the number of seconds taken to check all values in 32-bit range." << "\n";

  sink = bench_textbook(sink);
  sink = bench_cassioneri(sink);
  sink = bench_benjoffe(sink);
  sink = bench_benjoffe_usigned(sink);

  std::cout << "Done. Some of the results are likely to be very close, they are subject to noise, and may require multiple runs.\n";
  std::cout << "Sink: " << sink << "\n";

  run_search<int32_t>(
    "32-BIT",
    isleap32_textbook,
    isleap32_benjoffe,
    INT32_MIN,
    INT32_MAX
  );

  std::cout << "----------------------------------\n";

  std::this_thread::sleep_for(std::chrono::seconds(1));

  run_search<uint32_t>(
    "32-BIT (unsigned)",
    u_isleap32_textbook,
    u_isleap32_benjoffe,
    0,
    UINT32_MAX
  );

  std::cout << "----------------------------------\n";

  std::this_thread::sleep_for(std::chrono::seconds(1));

  if (run_search<int16_t>(
    "16-BIT",
    isleap16_textbook,
    isleap16_benjoffe,
    INT16_MIN,
    INT16_MAX
  ) == 4199ll) {
    std::cout << "\033[32mPass: This matches expectations.\033[0m\n";
  }

  std::cout << "----------------------------------\n";

  std::this_thread::sleep_for(std::chrono::seconds(1));

  run_search_64bit();

  return 0;
}