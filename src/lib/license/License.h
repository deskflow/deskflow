/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2016 Symless Ltd.
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

#include "SerialKey.h"
#include "license/ProductEdition.h"

#include <ctime>
#include <stdexcept>
#include <string>

class License {
  friend bool operator==(License const &, License const &);

public:
  class InvalidSerialKey : public std::runtime_error {
  public:
    explicit InvalidSerialKey() : std::runtime_error("Invalid serial key") {}
  };

  explicit License(Edition edition = Edition::kUnregistered);
  explicit License(const std::string &serialKey);

  bool isExpiring(time_t currentTime) const;
  bool isExpired(time_t currentTime) const;
  bool isTrial() const;
  bool isTimeLimited() const;
  bool isValid() const;
  time_t daysLeft(time_t currentTime) const;
  time_t getExpiration() const;
  int getTimeLeft(time_t time = ::time(0)) const;
  Edition edition() const;
  const std::string &toString() const;

private:
  SerialKey m_serialKey;
};

inline bool operator==(License const &lhs, License const &rhs) {
  return (lhs.m_serialKey == rhs.m_serialKey);
}

inline bool operator!=(License const &lhs, License const &rhs) {
  return !(lhs == rhs);
}
