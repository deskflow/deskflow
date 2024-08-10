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

#include "ILicense.h"
#include "SerialKey.h"

#include <chrono>
#include <ctime>
#include <functional>
#include <string>

class Server;
class LicenseHandler;
class LicenseTests;

namespace synergy::license {

class License : public ILicense {
  friend class ::Server;
  friend class ::LicenseHandler;
  friend class ::LicenseTests;

  using days = std::chrono::days;
  using system_clock = std::chrono::system_clock;
  using time_point = system_clock::time_point;
  using NowFunc = std::function<time_point()>;
  using LicenseError = std::runtime_error;

public:
  explicit License(const SerialKey &serialKey);
  explicit License(const std::string &hexString);
  ~License() override = default;

  friend bool operator==(License const &lhs, License const &rhs) {
    return lhs.m_serialKey == rhs.m_serialKey;
  }

  bool isTlsAvailable() const override;

  bool isValid() const { return m_serialKey.isValid; }
  bool isExpiringSoon() const;
  bool isExpired() const;
  bool isTrial() const;
  bool isSubscription() const;
  bool isTimeLimited() const;
  days daysLeft() const;
  Edition productEdition() const;
  std::string productName() const;
  const SerialKey &serialKey() const { return m_serialKey; }
  void invalidate() { m_serialKey = SerialKey::invalid(); }

  class InvalidSerialKey : public LicenseError {
  public:
    explicit InvalidSerialKey() : LicenseError("invalid serial key") {}
  };

  class NoTimeLimitError : public LicenseError {
  public:
    explicit NoTimeLimitError()
        : LicenseError("serial key has no time limit") {}
  };

protected:
  void setNowFunc(const NowFunc &nowFunc) { m_nowFunc = nowFunc; }

private:
  // for intentionality, force use of `invalid()` static function.
  License() = default;

  // prevent copy, so that changes can be reflected in one instance.
  License(const License &) = default;
  License &operator=(const License &) = default;

  static License invalid() { return License(); }

  SerialKey m_serialKey = SerialKey::invalid();
  NowFunc m_nowFunc = []() { return system_clock::now(); };
};

} // namespace synergy::license
