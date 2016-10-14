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
#include <QThread>

SubscriptionManager::SubscriptionManager(AppConfig* appConfig) :
	m_AppConfig(appConfig) {
}

void 
SubscriptionManager::setSerialKey(QString serialKeyString)
{
	SerialKey serialKey (serialKeyString.toStdString());
	if (serialKey.isValid (::time(0)) && (serialKey != m_serialKey)) {
		m_AppConfig->setSerialKey (serialKeyString);
		notifyActivation ("serial:" + serialKeyString);
		emit serialKeyChanged (serialKey);
	}
}

void SubscriptionManager::notifySkip()
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
