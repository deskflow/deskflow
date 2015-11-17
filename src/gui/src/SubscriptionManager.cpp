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

SubscriptionManager::SubscriptionManager(QWidget* parent, int& edition) :
	m_pParent(parent),
	m_Edition(edition)
{

}

bool SubscriptionManager::activateSerial(const QString& serial)
{
	m_Edition = Unknown;
	CoreInterface coreInterface;
	QString output;

	try
	{
		output = coreInterface.activateSerial(serial);
	}
	catch (std::exception& e)
	{
		m_ErrorMessage = e.what();
		checkError(m_ErrorMessage);
		return false;
	}

	checkOutput(output);

	return true;
}

bool SubscriptionManager::checkSubscription()
{
	m_Edition = Unknown;
	CoreInterface coreInterface;
	QString output;
	try
	{
		output = coreInterface.checkSubscription();
	}
	catch (std::exception& e)
	{
		m_ErrorMessage = e.what();
		checkError(m_ErrorMessage);
		return false;
	}

	checkOutput(output);

	return true;
}

bool SubscriptionManager::checkSubscriptionExist()
{
	CoreInterface coreInterface;
	QString subscriptionFilename = coreInterface.getSubscriptionFilename();

	return QFile::exists(subscriptionFilename);
}

void SubscriptionManager::checkError(QString& error)
{
	if (error.contains("trial has expired")) {
		QMessageBox::warning(m_pParent, tr("Subscription warning"),
			tr("Your trial has expired. Click <a href='https://synergy-project.org/account/'>here</a> to purchase"));
	}
	else {
		QMessageBox::warning(m_pParent, tr("Subscription error"),
			tr("An error occurred while trying to activate using a serial key. "
				"Please contact the helpdesk, and provide the "
				"following details.\n\n%1").arg(error));
	}
}

void SubscriptionManager::checkOutput(QString& output)
{
	getEditionType(output);
	checkExpiring(output);
}

void SubscriptionManager::getEditionType(QString& output)
{
	if (output.contains("pro subscription valid")) {
		m_Edition = Pro;
	}
	else if (output.contains("basic subscription valid")) {
		m_Edition = Basic;
	}
	else if (output.contains("trial subscription valid")) {
		m_Edition = Trial;
	}
}

void SubscriptionManager::checkExpiring(QString& output)
{
	if (output.contains("trial will end in")) {
		QRegExp dayLeftRegex(".*trial will end in ([0-9]+) day.*");
		if (dayLeftRegex.exactMatch(output)) {
			QString dayLeft = dayLeftRegex.cap(1);

			// TODO: warn user once a day
			QMessageBox::warning(m_pParent, tr("Subscription warning"),
				tr("Your trial will end in %1 %2. Click <a href='https://synergy-project.org/account/'>here</a> to purchase")
				.arg(dayLeft)
				.arg(dayLeft == "1" ? "day" : "days"));
		}
	}
}
