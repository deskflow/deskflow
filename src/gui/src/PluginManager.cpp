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

#include "PluginManager.h"

#include "CoreInterface.h"
#include "CommandProcess.h"
#include "DataDownloader.h"
#include "QUtility.h"
#include "ProcessorArch.h"
#include "Fingerprint.h"
#include "Plugin.h"

#include <QFile>
#include <QDir>
#include <QProcess>
#include <QCoreApplication>

PluginManager::PluginManager() :
	m_FileSysPluginList()
{
}

void PluginManager::initFromFileSys(QStringList pluginList)
{
	m_FileSysPluginList.clear();
	m_FileSysPluginList.append(pluginList);

	m_PluginDir = m_CoreInterface.getPluginDir();
	if (m_PluginDir.isEmpty()) {
		emit error(tr("Failed to get plugin directory."));
	}

	m_ProfileDir = m_CoreInterface.getProfileDir();
	if (m_ProfileDir.isEmpty()) {
		emit error(tr("Failed to get profile directory."));
	}
}

PluginManager::~PluginManager()
{
}

bool PluginManager::exist(QString name)
{
	CoreInterface coreInterface;
	QString PluginDir = coreInterface.getPluginDir();
	QString pluginName = Plugin::getOsSpecificName(name);
	QString filename;
	filename.append(PluginDir);
	filename.append(QDir::separator()).append(pluginName);
	QFile file(filename);
	bool exist = false;
	if (file.exists()) {
		exist = true;
	}

	return exist;
}

void PluginManager::copyPlugins()
{
	try {
		// Get the Directory where plugins are put on installation
		// If it doesn't exist, there is nothing to do
		QString srcDirName = Plugin::getOsSpecificInstallerLocation();
		QDir srcDir(srcDirName);
		if (!srcDir.exists()) {
			emit info(
				tr("No plugins found to copy from %1")
				.arg(srcDirName));
			emit copyFinished();
		}

		// Get the directory where Plugins are installed into Synergy
		// If it doesn't exist make it
		QString destDirName = m_PluginDir;
		QDir destDir(destDirName);
		if (!destDir.exists()) {
			destDir.mkpath(".");
		}
		// Run through the list of plugins and copy them
		for ( int i = 0 ; i < m_FileSysPluginList.size() ; i++ ) {
			// Get a file entry for the plugin using the full path
			QFile file(srcDirName + m_FileSysPluginList.at(i));
			// construct the destination file name
			QString newName = destDirName;
			newName.append(QDir::separator()).append(m_FileSysPluginList.at(i));

			QFile newFile(newName);
			if(newFile.exists()) {
				newFile.remove();
			}
			// make a copy of the plugin in the new location
			bool result = file.copy(newName);
			if ( !result ) {
					emit error(
							tr("Failed to copy plugin '%1' to: %2\n%3")
							.arg(m_FileSysPluginList.at(i))
							.arg(newName)
							.arg(file.errorString()));
			}
			else {
				emit info(
					tr("Copying '%1' plugin (%2/%3)...")
					.arg(m_FileSysPluginList.at(i))
					.arg(i+1)
					.arg(m_FileSysPluginList.size()));
			}
		}
	}
	catch (std::exception& e)
	{
		emit error(tr("An error occurred while trying to copy the "
								  "plugin list. Please contact the help desk, and "
								  "provide the following details.\n\n%1").arg(e.what()));
	}
	emit copyFinished();
	return;
}
