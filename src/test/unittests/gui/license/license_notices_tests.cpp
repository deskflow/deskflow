/*
 * Deskflow -- mouse and keyboard sharing utility
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

#include "gui/license/license_notices.h"

#include <chrono>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace std::chrono;
using namespace deskflow::license;
using namespace deskflow::gui;
using ::testing::HasSubstr;

const auto kPast = system_clock::now() - hours(1);
const auto kFutureOneHour = system_clock::now() + hours(1);
const auto kFutureOneDay = system_clock::now() + days(1) + hours(1);
const auto kFutureOneWeek = system_clock::now() + days(7) + hours(1);

TEST(license_notices_tests, licenseNotice_trialExpired_correctText) {
  SerialKey serialKey("");
  serialKey.isValid = true;
  serialKey.warnTime = kPast;
  serialKey.expireTime = kPast;
  serialKey.type.setType("trial");
  License license(serialKey);

  QString notice = licenseNotice(license);

  EXPECT_THAT(notice.toStdString(), HasSubstr("Your trial has expired"));
}

TEST(license_notices_tests, licenseNotice_trialExpiringInOneHour_correctText) {
  SerialKey serialKey("");
  serialKey.isValid = true;
  serialKey.warnTime = kFutureOneHour;
  serialKey.expireTime = kFutureOneHour;
  serialKey.type.setType("trial");
  License license(serialKey);

  QString notice = licenseNotice(license);

  EXPECT_THAT(notice.toStdString(), HasSubstr("Your trial expires today"));
}

TEST(license_notices_tests, licenseNotice_trialExpiringInOneDay_correctText) {
  SerialKey serialKey("");
  serialKey.isValid = true;
  serialKey.warnTime = kFutureOneDay;
  serialKey.expireTime = kFutureOneDay;
  serialKey.type.setType("trial");
  License license(serialKey);

  QString notice = licenseNotice(license);

  EXPECT_THAT(notice.toStdString(), HasSubstr("Your trial expires in 1 day"));
}

TEST(license_notices_tests, licenseNotice_trialExpiringInOneWeek_correctText) {
  SerialKey serialKey("");
  serialKey.isValid = true;
  serialKey.warnTime = kFutureOneWeek;
  serialKey.expireTime = kFutureOneWeek;
  serialKey.type.setType("trial");
  License license(serialKey);

  QString notice = licenseNotice(license);

  EXPECT_THAT(notice.toStdString(), HasSubstr("Your trial expires in 7 days"));
}

TEST(license_notices_tests, licenseNotice_subscriptionExpired_correctText) {
  SerialKey serialKey("");
  serialKey.isValid = true;
  serialKey.warnTime = kPast;
  serialKey.expireTime = kPast;
  serialKey.type.setType("subscription");
  License license(serialKey);

  QString notice = licenseNotice(license);

  EXPECT_THAT(notice.toStdString(), HasSubstr("Your license has expired"));
}

TEST(
    license_notices_tests,
    licenseNotice_subscriptionExpiringInOneHour_correctText) {
  SerialKey serialKey("");
  serialKey.isValid = true;
  serialKey.warnTime = kFutureOneHour;
  serialKey.expireTime = kFutureOneHour;
  serialKey.type.setType("subscription");
  License license(serialKey);

  QString notice = licenseNotice(license);

  EXPECT_THAT(notice.toStdString(), HasSubstr("Your license expires today"));
}

TEST(
    license_notices_tests,
    licenseNotice_subscriptionExpiringInOneDay_correctText) {
  SerialKey serialKey("");
  serialKey.isValid = true;
  serialKey.warnTime = kFutureOneDay;
  serialKey.expireTime = kFutureOneDay;
  serialKey.type.setType("subscription");
  License license(serialKey);

  QString notice = licenseNotice(license);

  EXPECT_THAT(notice.toStdString(), HasSubstr("Your license expires in 1 day"));
}

TEST(
    license_notices_tests,
    licenseNotice_subscriptionExpiringInOneWeek_correctText) {
  SerialKey serialKey("");
  serialKey.isValid = true;
  serialKey.warnTime = kFutureOneWeek;
  serialKey.expireTime = kFutureOneWeek;
  serialKey.type.setType("subscription");
  License license(serialKey);

  QString notice = licenseNotice(license);

  EXPECT_THAT(
      notice.toStdString(), HasSubstr("Your license expires in 7 days"));
}
