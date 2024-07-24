/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
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

#include "gui/LicenseDisplay.h"

#include <chrono>
#include <gtest/gtest.h>

using namespace synergy::license;
using namespace std::chrono;

const auto kPast = system_clock::now() - hours(1);
const auto kFuture = system_clock::now() + hours(1);

TEST(LicenseDisplayTests, setLicense_defaultLicense_returnsFalse) {
  LicenseDisplay licenseDisplay;
  License license;

  auto result = licenseDisplay.setLicense(license, false);

  ASSERT_FALSE(result);
}

TEST(LicenseDisplayTests, setLicense_validExpiredLicense_returnsTrue) {
  LicenseDisplay licenseDisplay;
  SerialKey serialKey;
  serialKey.isValid = true;
  serialKey.expireTime = kPast;
  License license(serialKey);

  auto result = licenseDisplay.setLicense(license, true);

  ASSERT_TRUE(result);
}