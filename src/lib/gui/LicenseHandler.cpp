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

#include "license/ProductEdition.h"
#include "license/parse_serial_key.h"

#include <QDateTime>
#include <QLocale>
#include <QThread>
#include <QTimer>
#include <ctime>

using namespace std::chrono;
using namespace synergy::license;

const char *const kLinkStyle = "color: #4285F4";
const char *const kPurchaseUrl =
    "https://symless.com/synergy/purchase?source=gui";
const char *const kBuyLink = R"(<a href="%1" style="%2">Buy now</a>)";
const char *const kRenewLink = R"(<a href="%1" style="%2">Renew now</a>)";

bool LicenseHandler::isValid(const License &license, bool acceptExpired) const {
  if (license.isExpired()) {
    if (acceptExpired) {
      qDebug("ignoring expired license");
      return true;
    } else {
      qDebug("license is expired");
      return false;
    }
  }

  return true;
}

bool LicenseHandler::changeSerialKey(
    const QString &hexString, bool acceptExpired) {

  qDebug() << "changing serial key to:"
           << (hexString.isEmpty() ? "empty" : hexString);

  if (hexString.isEmpty()) {
    if (!m_license.serialKey().hexString.empty()) {
      m_license.invalidate();
      emit serialKeyChanged("");
      qDebug("cleared serial key");
      return true;
    } else {
      qDebug("already empty serial key, ignoring");
      return false;
    }
  }

  auto serialKey = parseSerialKey(hexString.toStdString());

  if (serialKey == m_license.serialKey()) {
    qDebug("serial key did not change, ignoring");
    return false;
  }

  if (!serialKey.isValid) {
    qDebug() << "invalid serial key";
    return false;
  }

  m_license.setSerialKey(serialKey);

  if (isValid(m_license, acceptExpired)) {
    emit serialKeyChanged(hexString);
  } else {
    m_license.invalidate();
    emit serialKeyChanged("");
    return false;
  }

  if (m_license.isTimeLimited()) {
    auto daysLeft = m_license.daysLeft();
    auto msLeft = duration_cast<milliseconds>(daysLeft);
    QTimer::singleShot(msLeft, this, SLOT(validateLicense()));
  }

  return true;
}

void LicenseHandler::validate() const {
  if (!isValid(m_license, false)) {
    emit invalidLicense();
  }
}

Edition LicenseHandler::productEdition() const { return m_license.edition(); }

const License &LicenseHandler::license() const { return m_license; }

QString LicenseHandler::productName() const {
  return QString::fromStdString(m_license.productName());
}

QString LicenseHandler::noticeMessage() const {
  if (m_license.isTrial()) {
    return trialNotice();
  } else if (m_license.isTimeLimited()) {
    return timeLimitedNotice();
  } else {
    throw NoticeError();
  }
}

QString LicenseHandler::trialNotice() const {
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

QString LicenseHandler::timeLimitedNotice() const {
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
