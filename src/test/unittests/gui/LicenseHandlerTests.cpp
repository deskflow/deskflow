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

#include "gui/LicenseHandler.h"

#include <chrono>
#include <gtest/gtest.h>

using namespace synergy::license;
using namespace std::chrono;

const auto kPast = system_clock::now() - hours(1);
const auto kFuture = system_clock::now() + hours(1);

TEST(LicenseHandlerTests, changeSerialKey_validExpiredLicense_returnsTrue) {
  LicenseHandler licenseHandler;
  auto hexString = //
      "7B76323B747269616C3B62617369633B426F623B313B656D6"
      "1696C3B636F6D70616E79206E616D653B313B38363430307D";

  auto result = licenseHandler.changeSerialKey(hexString);

  ASSERT_EQ(LicenseHandler::ChangeSerialKeyResult::kSuccess, result);
}
