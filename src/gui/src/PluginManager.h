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

class PluginManager : public QObject
{
	Q_OBJECT

public:
	PluginManager(QStringList pluginList);
	~PluginManager();

	int downloadIndex() { return m_DownloadIndex; }

	static bool exist(QString name);

public slots:
	void downloadPlugins();

private:
	bool savePlugin();
	QString getPluginUrl(const QString& pluginName);
	bool runProgram(
		const QString& program,
		const QStringList& args,
		const QStringList& env);

	static QString getPluginOsSpecificName(const QString& pluginName);

signals:
	void error(QString e);
	void info(QString i);
	void downloadNext();
	void downloadFinished();

private:
	QStringList m_PluginList;
	QString m_PluginDir;
	QString m_ProfileDir;
	int m_DownloadIndex;
	DataDownloader m_DataDownloader;
	CoreInterface m_CoreInterface;
	SslCertificate m_SslCertificate;
};

#endif // PLUGINMANAGER_H
