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

#include <chrono>
#define TEST_ENV

#include "license/ProductEdition.h"

#include "license/License.h"

#include <climits>
#include <gtest/gtest.h>

using enum Edition;
using namespace synergy::license;
using time_point = std::chrono::system_clock::time_point;
using seconds = std::chrono::seconds;

class LicenseTests : public ::testing::Test {
protected:
  void setNow(License &license, int unixTime) const {
    license.setNowFunc([unixTime]() { return time_point{seconds{unixTime}}; });
  }
};

TEST_F(LicenseTests, isExpiring_validV2TrialBasicSerial_isTrial) {
  // {v2;trial;basic;Bob;1;email;company name;1;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B313B38363430307D");
  setNow(license, 0);

  EXPECT_TRUE(license.isTrial());
}

TEST_F(LicenseTests, isExpiring_validV2TrialBasicSerial_isTimeLimited) {
  // {v2;trial;basic;Bob;1;email;company name;1;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B313B38363430307D");
  setNow(license, 0);

  EXPECT_TRUE(license.isTimeLimited());
}

TEST_F(LicenseTests, isExpiring_validV2TrialBasicSerial_isNotSubscription) {
  // {v2;trial;basic;Bob;1;email;company name;1;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B313B38363430307D");
  setNow(license, 0);

  EXPECT_FALSE(license.isSubscription());
}

TEST_F(LicenseTests, isExpiring_validV2TrialBasicSerial_isExpiring) {
  // {v2;trial;basic;Bob;1;email;company name;1;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B313B38363430307D");
  setNow(license, 0);

  EXPECT_FALSE(license.isExpiring());
}

TEST_F(LicenseTests, isExpiring_validV2TrialBasicSerial_isBasicEdition) {
  // {v2;trial;basic;Bob;1;email;company name;1;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B313B38363430307D");
  setNow(license, 0);

  EXPECT_EQ(kBasic, license.productEdition());
}

TEST_F(LicenseTests, isExpiring_expiringV2TrialBasicSerial_returnTrue) {
  // {v2;trial;basic;Bob;1;email;company name;0;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B303B38363430307D");
  setNow(license, 1);

  EXPECT_TRUE(license.isTrial());
  EXPECT_TRUE(license.isExpiring());
}

TEST_F(LicenseTests, isExpired_validV2TrialBasicSerial_returnFalse) {
  // {v2;trial;basic;Bob;1;email;company name;0;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B303B38363430307D");
  setNow(license, 0);

  EXPECT_TRUE(license.isTrial());
  EXPECT_FALSE(license.isExpired());
}

TEST_F(LicenseTests, isExpired_expiringV2TrialBasicSerial_returnFalse) {
  // {v2;trial;basic;Bob;1;email;company name;0;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B303B38363430307D");
  setNow(license, 1);

  EXPECT_TRUE(license.isTrial());
  EXPECT_FALSE(license.isExpired());
}

TEST_F(LicenseTests, isExpired_expiredV2TrialBasicSerial_returnTrue) {
  // {v2;trial;basic;Bob;1;email;company name;0;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B303B38363430307D");
  setNow(license, 86401);

  EXPECT_TRUE(license.isTrial());
  EXPECT_TRUE(license.isExpired());
}

TEST_F(LicenseTests, daysLeft_validExactlyOneDayV2TrialBasicSerial_returnOne) {
  // {v2;trial;basic;Bob;1;email;company name;0;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B303B38363430307D");
  setNow(license, 0);

  EXPECT_EQ(1, license.daysLeft().count());
}

TEST_F(LicenseTests, daysLeft_validWithinOneDayV2TrialBasicSerial_returnOne) {
  // {v2;trial;basic;Bob;1;email;company name;0;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B303B38363430307D");
  setNow(license, 0);

  EXPECT_EQ(1, license.daysLeft().count());
}

TEST_F(LicenseTests, daysLeft_expiredV2TrialBasicSerial_returnZero) {
  // {v2;trial;basic;Bob;1;email;company name;0;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B303B38363430307D");
  setNow(license, 86401);

  EXPECT_EQ(0, license.daysLeft().count());
}

// Subscription license tests
TEST_F(LicenseTests, isExpiring_validV2SubscriptionBasicSerial_returnFalse) {
  // {v2;subscription;basic;Bob;1;email;company name;1;86400}
  License license("7B76323B737562736372697074696F6E3B62617369633B426F623B313B6"
                  "56D61696C3B636F6D70616E79206E616D653B313B38363430307D");
  setNow(license, 0);

  EXPECT_TRUE(license.isSubscription());
  EXPECT_FALSE(license.isExpiring());
  EXPECT_EQ(kBasic, license.productEdition());
}

TEST_F(LicenseTests, isExpiring_expiringV2SubscriptionBasicSerial_returnTrue) {
  // {v2;subscription;basic;Bob;1;email;company name;0;86400}
  License license("7B76323B737562736372697074696F6E3B62617369633B426F623B313B6"
                  "56D61696C3B636F6D70616E79206E616D653B303B38363430307D");
  setNow(license, 1);

  EXPECT_TRUE(license.isSubscription());
  EXPECT_TRUE(license.isExpiring());
}

TEST_F(LicenseTests, isExpired_expiredV2SubscriptionBasicSerial_returnTrue) {
  // {v2;subscription;basic;Bob;1;email;company name;0;86400}
  License license("7B76323B737562736372697074696F6E3B62617369633B426F623B313B6"
                  "56D61696C3B636F6D70616E79206E616D653B303B38363430307D");
  setNow(license, 86401);

  EXPECT_TRUE(license.isSubscription());
  EXPECT_TRUE(license.isExpired());
}
