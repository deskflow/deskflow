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

#include "SubscriptionManager.h"
#include "EditionType.h"
#include "AppConfig.h"
#include <ctime>
#include <stdexcept>
#include <QThread>

SubscriptionManager::SubscriptionManager(AppConfig* appConfig) :
    m_AppConfig(appConfig),
    m_serialKey(appConfig->edition()) {
    try {
        setSerialKey(m_AppConfig->serialKey());
    } catch (...) {
        m_AppConfig->setSerialKey("");
        m_AppConfig->saveSettings();
    }
}

SerialKey
SubscriptionManager::setSerialKey(QString serialKeyString)
{
    SerialKey serialKey (serialKeyString.toStdString());
    if (!serialKey.isValid (::time(0))) {
        throw std::runtime_error ("Invalid serial key");
    }

    if (serialKey != m_serialKey) {
        using std::swap;
        swap (serialKey, m_serialKey);

        m_AppConfig->setSerialKey (serialKeyString);
        notifyActivation ("serial:" + serialKeyString);
        emit serialKeyChanged (m_serialKey);

        if (m_serialKey.edition() != serialKey.edition()) {
            m_AppConfig->setEdition (m_serialKey.edition());
            emit editionChanged (m_serialKey.edition());
        }

        if (serialKey.isTrial()) {
            emit endTrial(false);
        }

        if (m_serialKey.isTrial()) {
            emit beginTrial(m_serialKey.isExpiring(::time(0)));
        }

        m_AppConfig->saveSettings();
    }

    return serialKey;
}

Edition SubscriptionManager::activeLicense() const
{
    return m_serialKey.edition();
}

void SubscriptionManager::update() const
{
    emit serialKeyChanged (m_serialKey);
    emit editionChanged (m_serialKey.edition());
    if (m_serialKey.isTrial()) {
        emit beginTrial(m_serialKey.isExpiring(::time(0)));
    }
}

void SubscriptionManager::skipActivation()
{
    notifyActivation ("skip:unknown");
}

void SubscriptionManager::notifyActivation(QString identity)
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
