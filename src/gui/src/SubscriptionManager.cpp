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

#include <QMessageBox>

SubscriptionManager::SubscriptionManager()
{
}

bool SubscriptionManager::activateSerial(const QString& serial)
{
	CoreInterface coreInterface;

	try
	{
		coreInterface.activateSerial(serial);
	}
	catch (std::exception& e)
	{
		showErrorDialog(e.what());
		return false;
	}

	return true;
}

bool SubscriptionManager::checkSubscription(int& edition)
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
		showErrorDialog(e.what());
		return false;
	}

	if (output.contains("subscription will expire soon")) {
		QMessageBox::warning(
			this, tr("Activate Subscription"),
			tr("Your subscription will be expired soon."));
	}

	edition = getEditionType(output);

	return true;
}

void SubscriptionManager::showErrorDialog(const QString& errorMsg)
{
	QMessageBox::critical(
			this, "Activate Subscription",
			tr("An error occurred while trying to activate using a serial key. "
			"Please contact the helpdesk, and provide the "
			"following details.\n\n%1").arg(errorMsg));
}

int SubscriptionManager::getEditionType(QString& string)
{
	if (string.contains("full subscription valid")) {
		return Pro;
	}

	return Unknown;
}
