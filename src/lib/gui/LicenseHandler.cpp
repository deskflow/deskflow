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

#include <QDebug>
#include <QProcessEnvironment>
#include <QTimer>
#include <qglobal.h>

using namespace std::chrono;
using namespace synergy::license;

const License &LicenseHandler::license() const { return m_license; }

Edition LicenseHandler::productEdition() const {
  return m_license.productEdition();
}

QString LicenseHandler::productName() const {
  return QString::fromStdString(m_license.productName());
}

LicenseHandler::ChangeSerialKeyResult
LicenseHandler::changeSerialKey(const QString &hexString) {
  using enum LicenseHandler::ChangeSerialKeyResult;

  if (!m_enabled) {
    qFatal("error: cannot set serial key, licensing is disabled");
  }

  if (hexString.isEmpty()) {
    qFatal("error: serial key is empty");
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
    qDebug("license validation failed, license invalid");
    emit invalidLicense();
  }

  if (m_license.isExpired()) {
    qDebug("license validation failed, license expired");
    emit invalidLicense();
  }
}
