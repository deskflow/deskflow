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

#include "CoreInterface.h"
#include "EditionType.h"
#include "SubscriptionState.h"

#include <QMessageBox>
#include <QFile>

SubscriptionManager::SubscriptionManager()
{
}

bool SubscriptionManager::activateSerial(const QString& serial, int& edition)
{
	edition = Unknown;
	CoreInterface coreInterface;
	QString output;

	try
	{
		output = coreInterface.activateSerial(serial);
	}
	catch (std::exception& e)
	{
		m_ErrorMessage = e.what();
		return false;
	}

	edition = getEditionType(output);

	return true;
}

int SubscriptionManager::checkSubscription(int& edition)
{
	edition = Unknown;
	CoreInterface coreInterface;
	QString output;
	try
	{
		output = coreInterface.checkSubscription();
	}
	catch (std::exception& e)
	{
		m_ErrorMessage = e.what();

		if (m_ErrorMessage.contains("subscription has expired")) {
			return kExpired;
		}

		return kInvalid;
	}

	if (output.contains("subscription will expire soon")) {
		return kExpiredSoon;
	}

	edition = getEditionType(output);

	return kValid;
}

bool SubscriptionManager::checkSubscriptionExist()
{
	CoreInterface coreInterface;
	QString subscriptionFilename = coreInterface.getSubscriptionFilename();

	return QFile::exists(subscriptionFilename);
}

int SubscriptionManager::getEditionType(QString& string)
{
	if (string.contains("full subscription valid")) {
		return Pro;
	}

	return Unknown;
}
