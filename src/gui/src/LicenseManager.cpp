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

LicenseManager::LicenseManager (AppConfig* appConfig)
    : m_AppConfig (appConfig), m_serialKey (appConfig->edition ()) {
}

std::pair<bool, QString>
LicenseManager::setSerialKey (SerialKey serialKey, bool acceptExpired) {
    std::pair<bool, QString> ret (true, "");
    time_t currentTime = ::time (0);

    if (!acceptExpired && serialKey.isExpired (currentTime)) {
        ret.first  = false;
        ret.second = "Serial key expired";
        return ret;
    }

    if (serialKey != m_serialKey) {
        using std::swap;
        swap (serialKey, m_serialKey);
        m_AppConfig->setSerialKey (
            QString::fromStdString (m_serialKey.toString ()));
        emit serialKeyChanged (m_serialKey);

        if (serialKey.isTrial ()) {
            emit endTrial (false);
        }

        if (m_serialKey.edition () != serialKey.edition ()) {
            m_AppConfig->setEdition (m_serialKey.edition ());
            emit editionChanged (m_serialKey.edition ());
        }

        if (m_serialKey.isTrial ()) {
            if (m_serialKey.isExpired (currentTime)) {
                emit endTrial (true);
            } else {
                emit beginTrial (m_serialKey.isExpiring (currentTime));
            }
        }

        m_AppConfig->saveSettings ();
    }

    return ret;
}

void
LicenseManager::notifyUpdate (QString fromVersion, QString toVersion) {
    if ((fromVersion == "Unknown") &&
        (m_serialKey == SerialKey (kUnregistered))) {
        return;
    }

    ActivationNotifier* notifier = new ActivationNotifier ();
    notifier->setUpdateInfo (fromVersion,
                             toVersion,
                             QString::fromStdString (m_serialKey.toString ()));

    QThread* thread = new QThread ();
    connect (notifier, SIGNAL (finished ()), thread, SLOT (quit ()));
    connect (notifier, SIGNAL (finished ()), notifier, SLOT (deleteLater ()));
    connect (thread, SIGNAL (finished ()), thread, SLOT (deleteLater ()));

    notifier->moveToThread (thread);
    thread->start ();

    QMetaObject::invokeMethod (notifier, "notifyUpdate", Qt::QueuedConnection);
}

Edition
LicenseManager::activeEdition () const {
    return m_serialKey.edition ();
}

QString
LicenseManager::activeEditionName () const {
    return getEditionName (activeEdition (), m_serialKey.isTrial ());
}

SerialKey
LicenseManager::serialKey () const {
    return m_serialKey;
}

void
LicenseManager::refresh () {
    if (!m_AppConfig->serialKey ().isEmpty ()) {
        try {
            SerialKey serialKey (m_AppConfig->serialKey ().toStdString ());
            setSerialKey (serialKey, true);
        } catch (...) {
            m_AppConfig->clearSerialKey ();
            m_AppConfig->saveSettings ();
        }
    }
    if (m_serialKey.isExpired (::time (0))) {
        emit endTrial (true);
    }
}

void
LicenseManager::skipActivation () {
    notifyActivation ("skip:unknown");
}

QString
LicenseManager::getEditionName (Edition const edition, bool trial) {
    std::string name ("Synergy");
    switch (edition) {
        case kUnregistered:
            name += " (UNREGISTERED)";
            return QString::fromUtf8 (name.c_str (), name.size ());
        case kBasic:
            name += " Basic";
            break;
        default:
            name += " Pro";
    }
    if (trial) {
        name += " (Trial)";
    }
    return QString::fromUtf8 (name.c_str (), name.size ());
}

void
LicenseManager::notifyActivation (QString identity) {
    ActivationNotifier* notifier = new ActivationNotifier ();
    notifier->setIdentity (identity);

    QThread* thread = new QThread ();
    connect (notifier, SIGNAL (finished ()), thread, SLOT (quit ()));
    connect (notifier, SIGNAL (finished ()), notifier, SLOT (deleteLater ()));
    connect (thread, SIGNAL (finished ()), thread, SLOT (deleteLater ()));

    notifier->moveToThread (thread);
    thread->start ();

    QMetaObject::invokeMethod (notifier, "notify", Qt::QueuedConnection);
}
