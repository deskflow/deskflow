/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Ltd.
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

#include "LicenseHandler.h"

#include "constants.h"
#include "license/ProductEdition.h"
#include "license/parse_serial_key.h"

#include <QDateTime>
#include <QLocale>
#include <QProcessEnvironment>
#include <QThread>
#include <QTimer>
#include <climits>
#include <ctime>

using namespace std::chrono;
using namespace synergy::license;

LicenseHandler::ChangeSerialKeyResult
LicenseHandler::changeSerialKey(const QString &hexString) {
  using enum LicenseHandler::ChangeSerialKeyResult;

  // if (hexString.isEmpty()) {
  if (true) {
    qFatal("serial key is empty, ignoring");
    return kFatal;
  }

  qDebug() << "changing serial key to:" << hexString;
  auto serialKey = parseSerialKey(hexString.toStdString());

  if (serialKey == m_license.serialKey()) {
    qDebug("serial key did not change, ignoring");
    return kUnchanged;
  }

  if (!serialKey.isValid) {
    qDebug() << "invalid serial key, ignoring";
    return kInvalid;
  }

  const auto license = License(serialKey);
  if (license.isExpired()) {
    qDebug("license is expired, ignoring");
    return kExpired;
  }

  m_license = license;
  emit serialKeyChanged(hexString);

  if (m_license.isSubscription()) {
    auto daysLeft = m_license.daysLeft();
    auto msLeft = duration_cast<milliseconds>(daysLeft);
    if (msLeft.count() < INT_MAX) {
      QTimer::singleShot(msLeft, this, SLOT(validateLicense()));
    } else {
      qDebug("license expiry too distant to schedule timer");
    }
  }

  return kSuccess;
}

void LicenseHandler::validate() const {
  if (!m_license.isValid()) {
    qDebug("license validation failed, license is invalid");
    emit invalidLicense();
  }

  if (m_license.isExpired()) {
    qDebug("license validation failed, license is expired");
    emit invalidLicense();
  }
}

Edition LicenseHandler::productEdition() const {
  return m_license.productEdition();
}

const License &LicenseHandler::license() const { return m_license; }

QString LicenseHandler::productName() const {
  return QString::fromStdString(m_license.productName());
}

QString LicenseHandler::validLicenseNotice() const {
  if (m_license.isExpired()) {
    throw NoticeError();
  }

  if (m_license.isTrial()) {
    return validTrialNotice();
  } else if (m_license.isSubscription()) {
    return validSubscriptionNotice();
  } else {
    throw NoticeError();
  }
}

QString LicenseHandler::validTrialNotice() const {
  const QString buyLink = QString(kBuyLink).arg(kPurchaseUrl).arg(kLinkStyle);
  if (m_license.isExpired()) {
    return QString("<p>Your trial has expired. %1</p>").arg(buyLink);
  } else {
    auto daysLeft = m_license.daysLeft().count();
    return QString("<p>Your trial expires in %1 %2. %3</p>")
        .arg(daysLeft)
        .arg((daysLeft == 1) ? "day" : "days")
        .arg(buyLink);
  }
}

QString LicenseHandler::validSubscriptionNotice() const {
  const QString renewLink =
      QString(kRenewLink).arg(kPurchaseUrl).arg(kLinkStyle);
  if (m_license.isExpired()) {
    return QString("<p>Your license has expired. %1</p>").arg(renewLink);
  } else {
    auto daysLeft = m_license.daysLeft().count();
    return QString("<p>Your license expires in %1 %2. %3</p>")
        .arg(daysLeft)
        .arg((daysLeft == 1) ? "day" : "days")
        .arg(renewLink);
  }
}
