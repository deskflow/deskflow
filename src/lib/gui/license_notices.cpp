/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Synergy Ltd.
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

#include "license_notices.h"

#include "constants.h"
#include "license/License.h"

using License = synergy::license::License;

namespace synergy::gui {

QString trialLicenseNotice(const synergy::license::License &license);
QString subscriptionLicenseNotice(const synergy::license::License &license);

QString licenseNotice(const License &license) {
  if (license.isTrial()) {
    return trialLicenseNotice(license);
  } else if (license.isSubscription()) {
    return subscriptionLicenseNotice(license);
  } else {
    qFatal("license notice only for time limited licenses");
  }
}

QString trialLicenseNotice(const License &license) {
  const QString buyLink = QString(kBuyLink).arg(kPurchaseUrl).arg(kWhiteLink);
  if (license.isExpired()) {
    return QString("<p>Your trial has expired. %1</p>").arg(buyLink);
  } else {
    auto daysLeft = license.daysLeft().count();
    return QString("<p>Your trial expires in %1 %2. %3</p>")
        .arg(daysLeft)
        .arg((daysLeft == 1) ? "day" : "days")
        .arg(buyLink);
  }
}

QString subscriptionLicenseNotice(const License &license) {
  const QString renewLink =
      QString(kRenewLink).arg(kPurchaseUrl).arg(kWhiteLink);
  if (license.isExpired()) {
    return QString("<p>Your license has expired. %1</p>").arg(renewLink);
  } else {
    auto daysLeft = license.daysLeft().count();
    return QString("<p>Your license expires in %1 %2. %3</p>")
        .arg(daysLeft)
        .arg((daysLeft == 1) ? "day" : "days")
        .arg(renewLink);
  }
}

} // namespace synergy::gui
