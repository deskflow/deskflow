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

#include "ActivationNotifier.h"
#include "AppConfig.h"

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

#ifdef SYNERGY_ENABLE_LICENSING
  if (!serialKey.isValid()) {
    throw std::runtime_error("The serial key is not valid.");
  }
#endif // SYNERGY_ENABLE_LICENSING
}

} // namespace

License::License(AppConfig *appConfig)
    : m_pAppConfig(appConfig),
      m_serialKey(appConfig->edition()),
      m_registry(*appConfig) {}

void License::setSerialKey(SerialKey serialKey, bool acceptExpired) {
  checkSerialKey(serialKey, acceptExpired);

  if (serialKey != m_serialKey) {
    using std::swap;
    swap(serialKey, m_serialKey);
    m_pAppConfig->setSerialKey(QString::fromStdString(m_serialKey.toString()));

    emit showLicenseNotice(getLicenseNotice());
    validateSerialKey();
    m_registry.scheduleRegistration();

    if (m_serialKey.edition() != serialKey.edition()) {
      m_pAppConfig->setEdition(m_serialKey.edition());
      emit editionChanged(m_serialKey.edition());
    }
  }
}

void License::notifyUpdate(QString fromVersion, QString toVersion) const {
  if ((fromVersion == "Unknown") && (m_serialKey == SerialKey(kUnregistered))) {
    return;
  }

  ActivationNotifier *notifier = new ActivationNotifier();
  notifier->setUpdateInfo(
      fromVersion, toVersion, QString::fromStdString(m_serialKey.toString()));

  QThread *thread = new QThread();
  connect(notifier, SIGNAL(finished()), thread, SLOT(quit()));
  connect(notifier, SIGNAL(finished()), notifier, SLOT(deleteLater()));
  connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

  notifier->moveToThread(thread);
  thread->start();

  QMetaObject::invokeMethod(notifier, "notifyUpdate", Qt::QueuedConnection);
}

Edition License::activeEdition() const { return m_serialKey.edition(); }

QString License::activeEditionName() const {
  return getEditionName(activeEdition(), m_serialKey.isTrial());
}

const SerialKey &License::serialKey() const { return m_serialKey; }

void License::refresh() {
  if (!m_pAppConfig->serialKey().isEmpty()) {
    try {
      SerialKey serialKey(m_pAppConfig->serialKey().toStdString());
      setSerialKey(serialKey, true);
    } catch (const std::exception &e) {
      qDebug() << e.what();
      m_serialKey = SerialKey();
      m_pAppConfig->clearSerialKey();
    }
  }
  if (!m_serialKey.isValid()) {
    emit invalidSerialKey();
  }
}

QString License::getEditionName(Edition const edition, bool trial) {
  SerialKeyEdition KeyEdition(edition);
  std::string name = KeyEdition.getProductName();

  if (trial) {
    name += " (Trial)";
  }

  return QString::fromUtf8(name.c_str(), static_cast<int>(name.size()));
}

void License::notifyActivation(QString identity) const {
  ActivationNotifier *notifier = new ActivationNotifier();
  notifier->setIdentity(identity);

  QThread *thread = new QThread();
  connect(notifier, SIGNAL(finished()), thread, SLOT(quit()));
  connect(notifier, SIGNAL(finished()), notifier, SLOT(deleteLater()));
  connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

  notifier->moveToThread(thread);
  thread->start();

  QMetaObject::invokeMethod(notifier, "notify", Qt::QueuedConnection);
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

void License::registerLicense() { m_registry.registerLicense(); }

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
