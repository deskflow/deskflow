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

#include "LicenseDisplay.h"
#include "license/Product.h"
#include "license/ProductEdition.h"

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

bool LicenseDisplay::isValid(const License &license, bool acceptExpired) const {
  if (license.isExpired()) {
    if (acceptExpired) {
      qDebug("Ignoring expired license");
      return true;
    } else {
      qDebug("License is expired");
      return false;
    }
  }

  return true;
}

bool LicenseDisplay::setLicense(const License &license, bool acceptExpired) {
  if (!isValid(license, acceptExpired)) {
    m_license = License::invalid();
    emit serialKeyChanged("");
    return false;
  }

  if (license != m_license) {
    m_license = license;

    emit serialKeyChanged(QString::fromStdString(m_license.toString()));

    if (m_license.edition() != license.edition()) {
      emit editionChanged(m_license.edition());
    }
  }

  if (m_license.isTimeLimited()) {
    auto daysLeft = m_license.daysLeft();
    auto msLeft = duration_cast<milliseconds>(daysLeft);
    QTimer::singleShot(msLeft, this, SLOT(validateLicense()));
  }

  return true;
}

void LicenseDisplay::validateLicense() const {
  if (!isValid(m_license, false)) {
    emit invalidLicense();
  }
}

Edition LicenseDisplay::productEdition() const { return m_license.edition(); }

const License &LicenseDisplay::license() const { return m_license; }

QString LicenseDisplay::productName() const {
  auto edition = productEdition();
  if (edition == Edition::kUnregistered) {
    return QString("%1 (unregistered)").arg(kLicensedProductName);
  }

  Product product(edition);
  std::string name = product.name();

  if (m_license.isTrial()) {
    name += " (Trial)";
  }

  return QString::fromUtf8(name.c_str(), static_cast<qsizetype>(name.size()));
}

QString LicenseDisplay::noticeMessage() const {
  if (m_license.isTrial()) {
    return getTrialNotice();
  } else if (m_license.isTimeLimited()) {
    return getTimeLimitedNotice();
  } else {
    throw NoticeError();
  }
}

QString LicenseDisplay::getTrialNotice() const {
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

QString LicenseDisplay::getTimeLimitedNotice() const {
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
