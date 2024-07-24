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

#include "License.h"

#include "Product.h"
#include "license/SerialKey.h"
#include "license/parse_serial_key.h"

#include <chrono>
#include <climits>

using namespace std::chrono;

namespace synergy::license {

License::License(const std::string &licenseString)
    : m_serialKey(parseSerialKey(licenseString)) {}

bool License::isExpiring(time_t currentTime) const {
  bool result = false;

  if (isTimeLimited()) {
    auto currentTimeAsLL = static_cast<unsigned long long>(currentTime);
    if ((m_serialKey.warnTime <= currentTimeAsLL) &&
        (currentTimeAsLL < m_serialKey.expireTime)) {
      result = true;
    }
  }

  return result;
}

bool License::isExpired(time_t currentTime) const {
  bool result = false;

  if (isTimeLimited()) {
    auto currentTimeAsLL = static_cast<unsigned long long>(currentTime);
    if (m_serialKey.expireTime <= currentTimeAsLL) {
      result = true;
    }
  }

  return result;
}

bool License::isTrial() const { return m_serialKey.type.isTrial(); }

bool License::isTimeLimited() const { return m_serialKey.type.isTimeLimited(); }

bool License::isValid() const {
  return m_serialKey.product.isValid() && !isExpired(::time(nullptr));
}

Edition License::edition() const { return m_serialKey.product.edition(); }

const std::string &License::toString() const { return m_serialKey.hexString; }

time_t License::daysLeft(time_t currentTime) const {
  auto now = system_clock::now();
  auto expireTime = m_serialKey.expireTime.value();

  if (currentTime >= expireTime) {
    return days(0);
  }

  auto timeLeft = expireTime - currentTime;
  return duration_cast<days>(timeLeft);
}

time_t License::getExpiration() const { return m_serialKey.expireTime; }

int License::getTimeLeft(time_t time) const {
  int result{-1};

  if (isTimeLimited() && !isExpired(time)) {
    auto timeLeft = (m_serialKey.expireTime - time) * 1000;

    if (timeLeft < INT_MAX) {
      result = static_cast<int>(timeLeft);
    } else {
      result = INT_MAX;
    }
  }

  return result;
}

} // namespace synergy::license