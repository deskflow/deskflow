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

#pragma once

#include <QWidget>

class AppConfig;

class SubscriptionManager : public QWidget
{
public:
	SubscriptionManager(QWidget* parent, AppConfig& appConfig, int& edition);

	bool activateSerial(const QString& serial);
	bool checkSubscription();
	bool fileExists();
	QString getLastError(){ return m_ErrorMessage; }

private:
	void checkError(QString& error);
	void checkOutput(QString& output);
	void getEditionType(QString& output);
	void checkExpiring(QString& output);
	bool shouldWarnExpiring();
	void persistDirectory();

private:
	QString m_ErrorMessage;
	QWidget* m_pParent;
	AppConfig& m_AppConfig;
	int& m_Edition;
};
