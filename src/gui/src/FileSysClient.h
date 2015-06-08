/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si, Std.
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

#ifndef FileSysClient_H
#define FileSysClient_H

#include <QString>
#include <QStringList>
#include <QObject>

#include "Plugin.h"
#include "CoreInterface.h"

class QMessageBox;
class QWidget;
class QStringList;

class FileSysClient : public QObject
{
	Q_OBJECT

public:
	QStringList& getPluginList() { return m_PluginList; }
	bool isDone() { return done; }
	int count() { return copyCount; }

public slots:
	void queryPluginList();

signals:
	void error(QString e);
	void queryPluginDone();

private:
	void isDone(bool b) { done = b; }
	QString request(const QString& email,
			const QString& password,
			QStringList& args);
	Plugin plugin;
	void count(int i) { copyCount = i; }

private:
	int copyCount;
	bool done;
	QStringList m_PluginList;
	CoreInterface m_CoreInterface;
};

#endif // FileSysClient_H
