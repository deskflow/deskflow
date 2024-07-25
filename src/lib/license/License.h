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
#include "license/Product.h"

#include <chrono>
#include <ctime>
#include <functional>
#include <string>

class LicenseTests;

namespace synergy::license {

class License {
  friend class ::LicenseTests;

  using days = std::chrono::days;
  using system_clock = std::chrono::system_clock;
  using time_point = system_clock::time_point;
  using NowFunc = std::function<time_point()>;

public:
  explicit License(const SerialKey &serialKey);
  explicit License(const std::string &serialKey);

  static License invalid() { return License(); }

  friend bool operator==(License const &lhs, License const &rhs) {
    return lhs.m_serialKey == rhs.m_serialKey;
  }

  bool isValid() const { return m_serialKey.isValid; }
  bool isExpiring() const;
  bool isExpired() const;
  bool isTrial() const;
  bool isTimeLimited() const;
  days daysLeft() const;
  Edition edition() const;
  const std::string &toString() const;

  using LicenseError = std::runtime_error;

  class InvalidSerialKey : public LicenseError {
  public:
    explicit InvalidSerialKey() : LicenseError("Invalid serial key") {}
  };

  class NoTimeLimitError : public LicenseError {
  public:
    explicit NoTimeLimitError()
        : LicenseError("Serial key has no time limit") {}
  };

protected:
  void setNowFunc(const NowFunc &nowFunc) { m_nowFunc = nowFunc; }

private:
  License() = default;
  SerialKey m_serialKey = SerialKey::invalid();
  NowFunc m_nowFunc = []() { return system_clock::now(); };
};

} // namespace synergy::license
