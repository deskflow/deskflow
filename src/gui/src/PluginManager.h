/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
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

#include "CoreInterface.h"

class DataDownloader;

class PluginManager : public QObject
{
	Q_OBJECT

public:
	PluginManager(QStringList pluginList);
	~PluginManager();

	int downloadIndex() { return m_DownloadIndex; }

public slots:
	void downloadPlugins();
	void saveOpenSSLBinary();
	void generateCertificate();
	void doGenerateCertificate();

private:
	void savePlugin();
	QString getPluginUrl(const QString& pluginName);
	QString getOpenSSLBinaryUrl();
	QString getPluginOSSpecificName(const QString& pluginName);
	bool checkOpenSSLBinary();
	void downloadOpenSSLBinary();

signals:
	void error(QString e);
	void downloadNext();
	void downloadFinished();
	void openSSLBinaryReady();
	void generateCertificateFinished();

private:
	QStringList m_PluginList;
	QString m_PluginDir;
	QString m_ProfileDir;
	int m_DownloadIndex;
	DataDownloader* m_pPluginDownloader;
	CoreInterface m_CoreInterface;
};

#endif // PLUGINMANAGER_H
