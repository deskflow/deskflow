/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Seamless Inc.
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

#include "LicenseManager.h"
#include "AppConfig.h"
#include <ctime>
#include <stdexcept>
#include <utility>
#include <QThread>
#include <QTimer>

LicenseManager::LicenseManager(AppConfig* appConfig) :
    m_AppConfig(appConfig),
    m_serialKey(appConfig->edition()) {
}

std::pair<bool, QString>
LicenseManager::setSerialKey(SerialKey serialKey, bool acceptExpired)
{
    std::pair<bool, QString> ret (true, "");
    time_t currentTime = ::time(0);

    if (!acceptExpired && serialKey.isExpired(currentTime)) {
        ret.first = false;
        ret.second = "Serial key expired";
        return ret;
    }

    if (serialKey != m_serialKey) {
        using std::swap;
        swap (serialKey, m_serialKey);
        m_AppConfig->setSerialKey(QString::fromStdString
                                  (m_serialKey.toString()));

        emit showLicenseNotice(getLicenseNotice());
        validateSerialKey();

        if (m_serialKey.edition() != serialKey.edition()) {
            m_AppConfig->setEdition(m_serialKey.edition());
            emit editionChanged(m_serialKey.edition());
        }
    }

    return ret;
}

void
LicenseManager::notifyUpdate(QString fromVersion, QString toVersion) const {
    if ((fromVersion == "Unknown")
            && (m_serialKey == SerialKey(kUnregistered))) {
        return;
    }

    ActivationNotifier* notifier = new ActivationNotifier();
    notifier->setUpdateInfo (fromVersion, toVersion,
                             QString::fromStdString(m_serialKey.toString()));

    QThread* thread = new QThread();
    connect(notifier, SIGNAL(finished()), thread, SLOT(quit()));
    connect(notifier, SIGNAL(finished()), notifier, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    notifier->moveToThread(thread);
    thread->start();

    QMetaObject::invokeMethod(notifier, "notifyUpdate",
                              Qt::QueuedConnection);
}

Edition
LicenseManager::activeEdition() const
{
    return m_serialKey.edition();
}

QString
LicenseManager::activeEditionName() const
{
    return getEditionName(activeEdition(), m_serialKey.isTrial());
}

SerialKey
LicenseManager::serialKey() const
{
    return m_serialKey;
}

void
LicenseManager::refresh()
{
    if (!m_AppConfig->serialKey().isEmpty()) {
        try {
            SerialKey serialKey (m_AppConfig->serialKey().toStdString());
            setSerialKey(serialKey, true);
        } catch (...) {
            m_AppConfig->clearSerialKey();
        }
    }
    if (!m_serialKey.isValid()) {
        emit InvalidLicense();
    }
}

void
LicenseManager::skipActivation() const
{
    notifyActivation ("skip:unknown");
}

QString
LicenseManager::getEditionName(Edition const edition, bool trial)
{
    SerialKeyEdition KeyEdition(edition);
    std::string name = KeyEdition.getDisplayName();

    if (trial) {
        name += " (Trial)";
    }

    return QString::fromUtf8 (name.c_str(), static_cast<int>(name.size()));
}

void
LicenseManager::notifyActivation(QString identity) const
{
    ActivationNotifier* notifier = new ActivationNotifier();
    notifier->setIdentity(identity);

    QThread* thread = new QThread();
    connect(notifier, SIGNAL(finished()), thread, SLOT(quit()));
    connect(notifier, SIGNAL(finished()), notifier, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    notifier->moveToThread(thread);
    thread->start();

    QMetaObject::invokeMethod(notifier, "notify", Qt::QueuedConnection);
}

QString
LicenseManager::getLicenseNotice() const
{
    QString Notice;

    if (m_serialKey.isTemporary()){
        if (m_serialKey.isTrial()){
            Notice = getTrialNotice();
        }
        else{
            Notice = getTemporaryNotice();
        }
    }

    return Notice;
}

QString
LicenseManager::getTrialNotice() const
{
    QString Notice;

    if (m_serialKey.isExpired(::time(0))){
        Notice = "<html><head/><body><p>Your %1 trial has expired. <a href="
                 "\"https://members.symless.com/purchase\">"
                 "<span style=\"text-decoration: underline;color:#0000ff;\">"
                 "Buy now!</span></a></p></body></html>";
        Notice = Notice.arg(LicenseManager::getEditionName(activeEdition()));
    }
    else{
        Notice = "<html><head/><body><p><span style=\""
                 "font-weight:600;\">%1</span> day%3 of "
                 "your %2 trial remain%5. <a href="
                 "\"https://members.symless.com/purchase\">"
                 "<span style=\"text-decoration: underline;"
                 " color:#0000ff;\">Buy now!</span></a>"
                 "</p></body></html>";

        time_t daysLeft = m_serialKey.daysLeft(::time(0));
        Notice = Notice
                .arg (daysLeft)
                .arg (LicenseManager::getEditionName(activeEdition()))
                .arg ((daysLeft == 1) ? "" : "s")
                .arg ((daysLeft == 1) ? "s" : "");
    }

    return Notice;
}

QString
LicenseManager::getTemporaryNotice() const
{
    QString Notice;

    if (m_serialKey.isExpired(::time(0))) {
        Notice = "<html><head/><body><p>Your license has expired. <a href="
                 "\"https://members.symless.com/purchase\">"
                 "<span style=\"text-decoration: underline;color:#0000ff;\">"
                 "Renew now!</span></a></p></body></html>";
    }
    else if (m_serialKey.isExpiring(::time(0))) {
        Notice = "<html><head/><body><p><span style=\""
                 "font-weight:600;\">%1</span> day%2 "
                 "before your license expires.<a href="
                 "\"https://members.symless.com/purchase\">"
                 "<span style=\"text-decoration: underline;"
                 " color:#0000ff;\">Renew now!</span></a>"
                 "</p></body></html>";

        time_t daysLeft = m_serialKey.daysLeft(::time(0));
        Notice = Notice.arg (daysLeft).arg ((daysLeft == 1) ? "" : "s");
    }

    return Notice;
}

void
LicenseManager::validateSerialKey() const
{
    if (m_serialKey.isValid()) {
        if (m_serialKey.isTemporary()){
           QTimer::singleShot(m_serialKey.getSpanLeft(), this, SLOT(validateSerialKey()));
        }
    }
    else{
        emit InvalidLicense();
    }
}
