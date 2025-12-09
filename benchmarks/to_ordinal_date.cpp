// SPDX-License-Identifier: BSL-1.0
// SPDX-FileCopyrightText: Copyright (c) 2025 Ben Joffe - https://www.benjoffe.com/fast-ordinal-date

/**
 * @file to_ordinal_date.cpp
 *
 * @brief Command line program that benchmarks implementations of to_ordinal_date.
 */

#include "algorithms_ordinal/ordinal_benjoffe_fast32.hpp"
#include "algorithms_ordinal/ordinal_benjoffe_fast64.hpp"
#include "algorithms_ordinal/ordinal_time_rs.hpp"
#include "util/ordinal.hpp"

#include <benchmark/benchmark.h>

#include <array>
#include <cstdint>
#include <random>

auto const rata_dies = [](){
  std::uniform_int_distribution<int32_t> uniform_dist(-146097, 146096);
  std::mt19937 rng;
  std::array<int32_t, 16384> ns;
  for (int32_t& n : ns)
    n = uniform_dist(rng);
  return ns;
}();

struct scan {};

template <typename A>
void time(benchmark::State& state);

template <>
void time<scan>(benchmark::State& state) {
  for (auto _ : state)
    for (int32_t rata_die : rata_dies)
      benchmark::DoNotOptimize(rata_die);
}

template <typename A>
void time(benchmark::State& state) {
  for (auto _ : state) {
    for (int32_t rata_die : rata_dies) {
      ordinal32_t date = A::to_date(rata_die);
      benchmark::DoNotOptimize(date);
    }
  }
}


BENCHMARK(time<scan                    >);
BENCHMARK(time<ordinal_benjoffe_fast32 >);
BENCHMARK(time<ordinal_benjoffe_fast64 >);
BENCHMARK(time<ordinal_time_rs         >);
