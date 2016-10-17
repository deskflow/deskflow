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
#include <utility>
#include <QThread>

SubscriptionManager::SubscriptionManager(AppConfig* appConfig) :
    m_AppConfig(appConfig),
    m_serialKey(appConfig->edition()) {
    try {
        setSerialKey(m_AppConfig->serialKey());
    } catch (...) {
		/* Remove garbage serial keys from the registry */
        m_AppConfig->setSerialKey("");
		m_AppConfig->setEdition(kUnregistered);
        m_AppConfig->saveSettings();
    }
}

std::pair<bool, QString>
SubscriptionManager::setSerialKey(QString serialKeyString)
{
	std::pair<bool, QString> ret (true, "");

    SerialKey serialKey (serialKeyString.toStdString());
	if (serialKey.isExpired(::time(0))) {
		ret.first = false;
		ret.second = "Serial key expired";
		return ret;
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

    return ret;
}

Edition 
SubscriptionManager::activeEdition() const
{
	return m_serialKey.edition();
}

QString
SubscriptionManager::activeEditionName() const
{
	return getEditionName(activeEdition(), m_serialKey.isTrial());
}

SerialKey 
SubscriptionManager::serialKey() const
{
	return m_serialKey;
}

void SubscriptionManager::refresh() const
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

QString
SubscriptionManager::getEditionName(Edition const edition, bool trial)
{
	std::string name ("Synergy ");
	switch (edition) {
		case kUnregistered:
			name += "(UNREGISTERED)";
			return QString::fromUtf8 (name.c_str(), name.size());
		case kBasic:
			name += "Basic";
			break;
		default:
			name += "Pro";
	}
	if (trial) {
		name += " (Trial)";
	}
	return QString::fromUtf8 (name.c_str(), name.size());
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
