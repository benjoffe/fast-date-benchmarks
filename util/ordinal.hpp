// SPDX-License-Identifier: BSL-1.0
// Copyright (c) 2025 Ben Joffe - https://www.benjoffe.com/fast-ordinal-date

#include "eaf/date.hpp"

#ifndef ORDINAL_HPP
#define ORDINAL_HPP

struct ordinal32_t {
  int32_t year;
  uint32_t doy; // day-of-year: 0-indexed (0–364 or 0–365 for leap years)
  bool leap;
};

#endif // ORDINAL_HPP