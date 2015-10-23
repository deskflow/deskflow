/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
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

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QString>
#include <QStringList>
#include <QObject>

#include "SslCertificate.h"
#include "CoreInterface.h"
#include "DataDownloader.h"
#include "Plugin.h"

class PluginManager : public QObject
{
	Q_OBJECT

public:
	PluginManager();
	~PluginManager();

	void init();

	int pluginCount() { return m_PluginList.count(); }
	QStringList& getPluginList() { return m_PluginList; }

	bool isDone() { return done; }
	void setDone(bool b) { done = b; }
	static bool exist(QString name);

public slots:
	void copyPlugins();
	void queryPluginList();

private:
	QString getPluginUrl(const QString& pluginName);
	bool runProgram(
		const QString& program,
		const QStringList& args,
		const QStringList& env);

signals:
	void error(QString e);
	void info(QString i);
	void updateCopyStatus(int);
	void copyFinished();
	void queryPluginDone();

private:
	QStringList m_PluginList;
	QString m_PluginDir;
	QString m_ProfileDir;
	QString m_InstalledDir;
	CoreInterface m_CoreInterface;
	SslCertificate m_SslCertificate;
	bool done;
};

#endif // PLUGINMANAGER_H
