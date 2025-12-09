// SPDX-License-Identifier: MIT OR Apache-2.0
// SPDX-FileCopyrightText: Copyright (c) 2019–2024 time-rs contributors
// This file contains a reimplementation of algorithms from the
// Rust `time` crate (https://github.com/time-rs/time).

#ifndef EAF_ALGORITHMS_ORDINAL_TIME_RS_H
#define EAF_ALGORITHMS_ORDINAL_TIME_RS_H

#include "util/ordinal.hpp"

#include <stdint.h>

struct ordinal_time_rs {

  static uint32_t constexpr S = 2500;
  static uint32_t constexpr K = 719468 + 146097 * S;
  static uint32_t constexpr L = 400 * S;

  static inline
  ordinal32_t to_date(int32_t dayNumber) {

    uint32_t const n = dayNumber + K;

    uint32_t const n_1 = 4 * n + 3;
    uint32_t const c   = n_1 / 146097;
    uint32_t const n_c = n_1 % 146097 / 4;

    uint32_t const n_2 = 4 * n_c + 3;
    uint64_t const p_2 = uint64_t(2939745) * n_2;
    uint32_t const z   = uint32_t(p_2 >> 32);
    uint32_t const n_y = uint32_t(p_2) / 2939745 / 4;
    uint32_t const y   = 100 * c + z;

    bool const j   = n_y >= 306;
    int32_t const y_g = y - L + j;

    // is_leap_year function inlined:
    uint32_t const d = y_g % 100 == 0 ? 15 : 3;
    bool const is_leap_year = ((y_g & d) == 0);

    uint32_t ordinal = j ? n_y - 305 : n_y + 60 + is_leap_year;

    return ordinal32_t{y_g, ordinal, is_leap_year};
  }

}; // struct ordinal_time_rs

#endif // EAF_ALGORITHMS_ORDINAL_TIME_RS_H