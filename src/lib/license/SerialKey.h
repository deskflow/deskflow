/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2022 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "ProductEdition.h"
#include "SerialKeyType.h"
#include "license/ProductEdition.h"

#include <chrono>
#include <ctime>
#include <optional>
#include <string>

namespace synergy::license {

struct SerialKey {
  using time_point = std::chrono::system_clock::time_point;

  bool isValid = false;
  std::string hexString = "";
  Product product;
  SerialKeyType type;
  std::optional<time_point> warnTime = std::nullopt;
  std::optional<time_point> expireTime = std::nullopt;

  explicit SerialKey(const std::string &key) : hexString(key) {}
  explicit SerialKey(Edition edition = Edition::kUnregistered)
      : product(edition) {}
};

inline bool operator==(SerialKey const &lhs, SerialKey const &rhs) {
  return (lhs.hexString == rhs.hexString) && (lhs.warnTime == rhs.warnTime) &&
         (lhs.expireTime == rhs.expireTime) && (lhs.product == rhs.product) &&
         (lhs.type == rhs.type);
}

} // namespace synergy::license
