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

#include <string>

struct SerialKey {
  std::string key;
  Product product;
  SerialKeyType keyType;
  unsigned long long warnTime = 0;
  unsigned long long expireTime = 0;

  explicit SerialKey(const std::string &key) : key(key) {}
  explicit SerialKey(Edition edition = Edition::kUnregistered)
      : product(edition) {}
};

inline bool operator==(SerialKey const &lhs, SerialKey const &rhs) {
  return (lhs.key == rhs.key) && (lhs.warnTime == rhs.warnTime) &&
         (lhs.expireTime == rhs.expireTime) && (lhs.product == rhs.product) &&
         (lhs.keyType == rhs.keyType);
}
