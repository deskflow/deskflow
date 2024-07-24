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

#include "License.h"

#include <QDateTime>
#include <QLocale>
#include <QThread>
#include <QTimer>
#include <ctime>
#include <stdexcept>
#include <utility>

namespace {

std::string getMaintenanceMessage(const SerialKey &serialKey) {
  auto expiration =
      QDateTime::fromSecsSinceEpoch(serialKey.getExpiration()).date();
  QString message =
      "The license key you used will only work with versions of Synergy "
      "released before %1."
      "<p>To use this version, you’ll need to renew your Synergy maintenance "
      "license. "
      "<a href=\"https://symless.com/synergy/account?source=gui\""
      "style=\"text-decoration: none; color: #4285F4;\">Renew today</a>.</p>";
  auto formatedDate = QLocale("en_US").toString(expiration, "MMM dd yyyy");
  return message.arg(formatedDate).toStdString();
}

void checkSerialKey(const SerialKey &serialKey, bool acceptExpired) {
  if (serialKey.isMaintenance()) {
    auto buildDate =
        QDateTime::fromString(__TIMESTAMP__, "ddd MMM dd hh:mm:ss yyyy")
            .toSecsSinceEpoch();

    if (buildDate > serialKey.getExpiration()) {
      throw std::runtime_error(getMaintenanceMessage(serialKey));
    }
  }

  if (!acceptExpired && serialKey.isExpired(::time(nullptr))) {
    throw std::runtime_error("Serial key expired");
  }

  if (!serialKey.isValid()) {
    // TODO: throwing an exception seems a bit extreme... replace with signal.
    throw std::runtime_error("The serial key is not valid.");
  }
}

} // namespace

void License::setSerialKey(SerialKey serialKey, bool acceptExpired) {
  checkSerialKey(serialKey, acceptExpired);

  if (serialKey != m_serialKey) {
    using std::swap;
    swap(serialKey, m_serialKey);

    // TODO: update via signal
    // m_pAppConfig->setSerialKey(QString::fromStdString(m_serialKey.toString()));

    emit showLicenseNotice(getLicenseNotice());
    validateSerialKey();

    if (m_serialKey.edition() != serialKey.edition()) {

      // TODO: update via signal
      // m_pAppConfig->setEdition(m_serialKey.edition());

      emit editionChanged(m_serialKey.edition());
    }
  }
}

Edition License::activeEdition() const { return m_serialKey.edition(); }

QString License::productName() const {
  return getProductName(activeEdition(), m_serialKey.isTrial());
}

const SerialKey &License::serialKey() const { return m_serialKey; }

void License::refresh() {
  // TODO: replace with signal

  // if (!m_pAppConfig->serialKey().isEmpty()) {
  //   try {
  //     SerialKey serialKey(m_pAppConfig->serialKey().toStdString());
  //     setSerialKey(serialKey, true);
  //   } catch (const std::exception &e) {
  //     qDebug() << e.what();
  //     m_serialKey = SerialKey();
  //     m_pAppConfig->clearSerialKey();
  //   }
  // }

  if (!m_serialKey.isValid()) {
    emit invalidSerialKey();
  }
}

QString License::getProductName(Edition const edition, bool trial) {
  SerialKeyEdition KeyEdition(edition);
  std::string name = KeyEdition.getProductName();

  if (trial) {
    name += " (Trial)";
  }

  return QString::fromUtf8(name.c_str(), static_cast<int>(name.size()));
}

QString License::getLicenseNotice() const {
  QString Notice;

  if (m_serialKey.isTemporary()) {
    if (m_serialKey.isTrial()) {
      Notice = getTrialNotice();
    } else {
      Notice = getTemporaryNotice();
    }
  }

  return Notice;
}

QString License::getTrialNotice() const {
  QString Notice;

  if (m_serialKey.isExpired(::time(0))) {
    Notice = "<html><head/><body><p>"
             "Trial expired - "
             "<a href=\"https://symless.com/synergy/purchase?source=gui\" "
             "style=\"color: #FFFFFF;\">Buy now</a>"
             "</p></body></html>";
  } else {
    Notice = "<html><head/><body><p>"
             "Trial expires in %1 day%2 - "
             "<a href=\"https://symless.com/synergy/purchase?source=gui\" "
             "style=\"color: #FFFFFF;\">Buy now</a>"
             "</p></body></html>";

    time_t daysLeft = m_serialKey.daysLeft(::time(0));
    Notice = Notice.arg(daysLeft).arg((daysLeft == 1) ? "" : "s");
  }

  return Notice;
}

QString License::getTemporaryNotice() const {
  QString Notice;

  if (m_serialKey.isExpired(::time(0))) {
    Notice = "<html><head/><body><p>"
             "License expired - "
             "<a href=\"https://symless.com/synergy/purchase?source=gui\" "
             "style=\"color: #FFFFFF;\">Renew now</a>"
             "</p></body></html>";
  } else if (m_serialKey.isExpiring(::time(0))) {
    Notice = "<html><head/><body><p>"
             "License expires in %1 day%2 - "
             "<a href=\"https://symless.com/synergy/purchase?source=gui\" "
             "style=\"color: #FFFFFF;\">Renew now</a>"
             "</p></body></html>";

    time_t daysLeft = m_serialKey.daysLeft(::time(0));
    Notice = Notice.arg(daysLeft).arg((daysLeft == 1) ? "" : "s");
  }

  return Notice;
}

void License::validateSerialKey() const {
  if (m_serialKey.isValid()) {
    if (m_serialKey.isTemporary()) {
      QTimer::singleShot(
          m_serialKey.getSpanLeft(), this, SLOT(validateSerialKey()));
    }
  } else {
    emit invalidSerialKey();
  }
}
