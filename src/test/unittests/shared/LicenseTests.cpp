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

#include "license/ProductEdition.h"
#define TEST_ENV

#include "license/License.h"

#include <climits>
#include <gtest/gtest.h>

using enum Edition;

TEST(LicenseTests, isExpiring_validV2TrialBasicSerial_returnFalse) {
  // {v2;trial;basic;Bob;1;email;company name;1;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B313B38363430307D");
  EXPECT_EQ(true, license.isTrial());
  EXPECT_EQ(true, license.isTimeLimited());
  EXPECT_FALSE(license.isExpiring(0));
  EXPECT_EQ(kBasic, license.edition());
}

TEST(LicenseTests, isExpiring_expiringV2TrialBasicSerial_returnTrue) {
  // {v2;trial;basic;Bob;1;email;company name;0;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B303B38363430307D");
  EXPECT_EQ(true, license.isTrial());
  EXPECT_EQ(true, license.isTimeLimited());
  EXPECT_EQ(true, license.isExpiring(1));
}

TEST(LicenseTests, isExpiring_expiredV2TrialBasicSerial_returnFalse) {
  // {v2;trial;basic;Bob;1;email;company name;0;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B303B38363430307D");
  EXPECT_EQ(true, license.isTrial());
  EXPECT_EQ(true, license.isTimeLimited());
  EXPECT_FALSE(license.isExpiring(86401));
}

TEST(LicenseTests, isExpired_validV2TrialBasicSerial_returnFalse) {
  // {v2;trial;basic;Bob;1;email;company name;0;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B303B38363430307D");
  EXPECT_EQ(true, license.isTrial());
  EXPECT_EQ(true, license.isTimeLimited());
  EXPECT_FALSE(license.isExpired(0));
}

TEST(LicenseTests, isExpired_expiringV2TrialBasicSerial_returnFalse) {
  // {v2;trial;basic;Bob;1;email;company name;0;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B303B38363430307D");
  EXPECT_EQ(true, license.isTrial());
  EXPECT_EQ(true, license.isTimeLimited());
  EXPECT_FALSE(license.isExpired(1));
}

TEST(LicenseTests, isExpired_expiredV2TrialBasicSerial_returnTrue) {
  // {v2;trial;basic;Bob;1;email;company name;0;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B303B38363430307D");
  EXPECT_EQ(true, license.isTrial());
  EXPECT_EQ(true, license.isTimeLimited());
  EXPECT_EQ(true, license.isExpired(86401));
}

TEST(LicenseTests, daysLeft_validExactlyOneDayV2TrialBasicSerial_returnOne) {
  // {v2;trial;basic;Bob;1;email;company name;0;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B303B38363430307D");
  EXPECT_EQ(1, license.daysLeft(0));
}

TEST(LicenseTests, daysLeft_validWithinOneDayV2TrialBasicSerial_returnOne) {
  // {v2;trial;basic;Bob;1;email;company name;0;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B303B38363430307D");
  EXPECT_EQ(1, license.daysLeft(1));
}

TEST(LicenseTests, daysLeft_expiredV2TrialBasicSerial_returnZero) {
  // {v2;trial;basic;Bob;1;email;company name;0;86400}
  License license("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636"
                  "F6D70616E79206E616D653B303B38363430307D");
  EXPECT_EQ(0, license.daysLeft(86401));
}

// Subscription license tests
TEST(LicenseTests, isExpiring_validV2SubscriptionBasicSerial_returnFalse) {
  // {v2;subscription;basic;Bob;1;email;company name;1;86400}
  License license("7B76323B737562736372697074696F6E3B62617369633B426F623B313B6"
                  "56D61696C3B636F6D70616E79206E616D653B313B38363430307D");
  EXPECT_EQ(false, license.isTrial());
  EXPECT_EQ(true, license.isTimeLimited());
  EXPECT_FALSE(license.isExpiring(0));
  EXPECT_EQ(kBasic, license.edition());
}

TEST(LicenseTests, isExpiring_expiringV2SubscriptionBasicSerial_returnTrue) {
  // {v2;subscription;basic;Bob;1;email;company name;0;86400}
  License license("7B76323B737562736372697074696F6E3B62617369633B426F623B313B6"
                  "56D61696C3B636F6D70616E79206E616D653B303B38363430307D");
  EXPECT_EQ(false, license.isTrial());
  EXPECT_EQ(true, license.isTimeLimited());
  EXPECT_EQ(true, license.isExpiring(1));
}

TEST(LicenseTests, isExpired_expiredV2SubscriptionBasicSerial_returnTrue) {
  // {v2;subscription;basic;Bob;1;email;company name;0;86400}
  License license("7B76323B737562736372697074696F6E3B62617369633B426F623B313B6"
                  "56D61696C3B636F6D70616E79206E616D653B303B38363430307D");
  EXPECT_EQ(false, license.isTrial());
  EXPECT_EQ(true, license.isTimeLimited());
  EXPECT_EQ(true, license.isExpired(86401));
}

// toString method tests
TEST(LicenseTests, toStringV2SubscriptionBasicSerialKey) {
  //{v2;subscription;basic;Bob;1;email;company name;0;86400}
  const std::string Expected =
      "7B76323B737562736372697074696F6E3B62617369633B426F623B313B656D61696C3B63"
      "6F6D70616E79206E616D653B303B38363430307D";
  License license(Expected);
  EXPECT_EQ(Expected, license.toString());
}

TEST(LicenseTests, toStringV2TrialBasicSerialKey) {
  //{v2;trial;basic;Bob;1;email;company name;0;86400}
  const std::string Expected =
      "7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636F6D70616E7920"
      "6E616D653B303B38363430307D";
  License license(Expected);
  EXPECT_EQ(Expected, license.toString());
}

TEST(LicenseTests, toStringV1BasicSerialKey) {
  //{v1;basic;Bob;1;email;company name;0;0}
  const std::string Expected = "7B76313B62617369633B426F623B313B656D61696C3B636"
                               "F6D70616E79206E616D653B303B307D";
  License license(Expected);
  EXPECT_EQ(Expected, license.toString());
}

TEST(LicenseTests, IsValidKey_false) {
  //{v1;basic;Bob;1;email;company name;0;0}
  License license(kUnregistered);
  EXPECT_EQ(false, license.isValid());
}

TEST(LicenseTests, IsValidKey_true) {
  License license(kBasic);
  EXPECT_EQ(true, license.isValid());
}

TEST(LicenseTests, IsValidExpiredKey_false) {
  License license("7B76323B737562736372697074696F6E3B62617369633B426F623B313B6"
                  "56D61696C3B636F6D70616E79206E616D653B303B38363430307D");
  EXPECT_EQ(false, license.isValid());
}

TEST(LicenseTests, test_getTimeLeft) {
  License license;
  int expected{-1};
  EXPECT_EQ(expected, license.getTimeLeft());
}

TEST(LicenseTests, test_getTimeLeft_subscription) {
  // {v2;subscription;basic;Bob;1;email;company name;0;86400}
  License license(
      "7B76323B737562736372697074696F6E3B62617369633B426F623B313B656D"
      "61696C3B636F6D70616E79206E616D653B303B38363430307D");
  EXPECT_EQ(1000, license.getTimeLeft(86399));
  EXPECT_EQ(-1, license.getTimeLeft(86401));
}

TEST(LicenseTests, test_getTimeLeft_max_int) {
  //"{v2;subscription;basic;Bob;1;email;company name;0;3147483647}"
  License license(
      "7B76323B737562736372697074696F6E3B62617369633B426F623B313B656D"
      "61696C3B636F6D70616E79206E616D653B303B333134373438333634377D");
  EXPECT_EQ(INT_MAX, license.getTimeLeft(1));
}
