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
#ifndef PLUGIN_H
#define PLUGIN_H

#include <QString>
#include <QStringList>
#include <QObject>

#include "SslCertificate.h"
#include "CoreInterface.h"
#include "DataDownloader.h"

class Plugin : public QObject
{
	Q_OBJECT

public:
	//Plugin();
	//~PluginManager();

	static QString getOsSpecificName(const QString& pluginName);
	static QString getOsSpecificExt();
	static QString getOsSpecificLocation();
	static QString getOsSpecificInstallerLocation();
	static QString getOsSpecificUserLocation();

public slots:

private:
//	CoreInterface m_CoreInterface;

signals:

private:

};

#endif // PLUGIN_H
