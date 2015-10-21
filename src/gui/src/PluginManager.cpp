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
#include "../lib/common/PluginVersion.h"

#include <QTextStream>

#include <QFile>
#include <QDir>
#include <QProcess>
#include <QCoreApplication>


PluginManager::PluginManager() :
	m_PluginList()
{
	init();
}

PluginManager::~PluginManager()
{
}

void PluginManager::init()
{
	m_PluginDir = m_CoreInterface.getPluginDir();
	if (m_PluginDir.isEmpty()) {
		emit error(tr("Failed to get plugin directory."));
	}

	m_ProfileDir = m_CoreInterface.getProfileDir();
	if (m_ProfileDir.isEmpty()) {
		emit error(tr("Failed to get profile directory."));
	}

	m_InstalledDir = m_CoreInterface.getInstalledDir();
	if (m_InstalledDir.isEmpty()) {
		emit error(tr("Failed to get installed directory."));
	}
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
		QString srcDirName(m_InstalledDir.append(QDir::separator())
							.append(Plugin::getOsSpecificInstallerLocation()));

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
		for ( int i = 0 ; i < m_PluginList.size() ; i++ ) {
			// Get a file entry for the plugin using the full path
			QFile file(srcDirName + QDir::separator() + m_PluginList.at(i));

			// construct the destination file name
			QString newName(destDirName + QDir::separator() + m_PluginList.at(i));

			// Check to see if the plugin already exists
			QFile newFile(newName);
			if(newFile.exists()) {
				// If it does, delete it. TODO: Check to see if same and leave
				bool result = newFile.remove();
				if( !result ) {
					emit error(
							tr(	"Unable to delete plugin:\n%1\n"
								"Please stop synergy and run the wizard again.")
							.arg(newName));
					return;
				}
			}
			// make a copy of the plugin in the new location
			#if defined(Q_OS_WIN)
			bool result = file.copy(newName);
			#else
			bool result = file.link(newName);
			#endif
			if ( !result ) {
					emit error(
							tr("Failed to copy plugin '%1' to: %2\n%3\n"
							   "Please stop synergy and run the wizard again.")
							.arg(m_PluginList.at(i))
							.arg(newName)
							.arg(file.errorString()));
					return;
			}
			else {
				emit info(
					tr("Copying '%1' plugin (%2/%3)...")
					.arg(m_PluginList.at(i))
					.arg(i+1)
					.arg(m_PluginList.size()));
			}
		}
	}
	catch (std::exception& e)
	{
		emit error(tr(	"An error occurred while trying to copy the "
						"plugin list. Please contact the help desk, and "
						"provide the following details.\n\n%1").arg(e.what()));
	}

	emit copyFinished();
	return;
}

void PluginManager::queryPluginList()
{
	try {
		setDone(false);
		QString extension = "*" + Plugin::getOsSpecificExt();
		QStringList nameFilter(extension);

		QString installDir(m_CoreInterface.getInstalledDir()
							.append(QDir::separator())
							.append(Plugin::getOsSpecificInstallerLocation()));

		QString searchDirectory(installDir);
		QDir directory(searchDirectory);
		m_PluginList = directory.entryList(nameFilter);
		setDone(true);
	}
	catch (std::exception& e)
	{
		setDone(true);
		emit error(tr(	"An error occurred while trying to load the "
						"plugin list. Please contact the help desk, and "
						"provide the following details.\n\n%1").arg(e.what()));
	}
	emit queryPluginDone();
	return;
}
