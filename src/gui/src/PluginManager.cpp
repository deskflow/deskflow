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

#include <QFile>
#include <QDir>
#include <QProcess>
#include <QCoreApplication>

static QString kBaseUrl = "http://synergy-project.org/files";
static const char kWinProcessorArch32[] = "Windows-x86";
static const char kWinProcessorArch64[] = "Windows-x64";
static const char kMacProcessorArch[] = "MacOSX-i386";
static const char kLinuxProcessorArchDeb32[] = "Linux-i686-deb";
static const char kLinuxProcessorArchDeb64[] = "Linux-x86_64-deb";
static const char kLinuxProcessorArchRpm32[] = "Linux-i686-rpm";
static const char kLinuxProcessorArchRpm64[] = "Linux-x86_64-rpm";

#if defined(Q_OS_WIN)
static const char kWinPluginExt[] = ".dll";

#elif defined(Q_OS_MAC)
static const char kMacPluginPrefix[] = "lib";
static const char kMacPluginExt[] = ".dylib";
#else
static const char kLinuxPluginPrefix[] = "lib";
static const char kLinuxPluginExt[] = ".so";
#endif

PluginManager::PluginManager(QStringList pluginList) :
	m_PluginList(pluginList),
	m_DownloadIndex(-1)
{
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
	QString pluginName = getPluginOsSpecificName(name);
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

void PluginManager::downloadPlugins()
{
	if (m_DataDownloader.isFinished()) {
		if (!savePlugin()) {
			return;
		}

		if (m_DownloadIndex != m_PluginList.size() - 1) {
			emit downloadNext();
		}
		else {
			emit downloadFinished();
			return;
		}
	}

	m_DownloadIndex++;

	if (m_DownloadIndex < m_PluginList.size()) {
		QUrl url;
		QString pluginUrl = getPluginUrl(m_PluginList.at(m_DownloadIndex));
		if (pluginUrl.isEmpty()) {
			return;
		}
		url.setUrl(pluginUrl);

		connect(&m_DataDownloader, SIGNAL(isComplete()), this, SLOT(downloadPlugins()));

		m_DataDownloader.download(url);
	}
}

bool PluginManager::savePlugin()
{
	// create the path if not exist
	QDir dir(m_PluginDir);
	if (!dir.exists()) {
		dir.mkpath(".");
	}

	QString filename = m_PluginDir;
	QString pluginName = m_PluginList.at(m_DownloadIndex);
	pluginName = getPluginOsSpecificName(pluginName);
	filename.append(QDir::separator()).append(pluginName);

	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly)) {
		emit error(
				tr("Failed to download plugin '%1' to: %2\n%3")
				.arg(m_PluginList.at(m_DownloadIndex))
				.arg(m_PluginDir)
				.arg(file.errorString()));

		file.close();
		return false;
	}

	file.write(m_DataDownloader.data());
	file.close();

	return true;
}

QString PluginManager::getPluginUrl(const QString& pluginName)
{
	QString archName;

#if defined(Q_OS_WIN)

	try {
		QString coreArch = m_CoreInterface.getArch();
		if (coreArch.startsWith("x86")) {
			archName = kWinProcessorArch32;
		}
		else if (coreArch.startsWith("x64")) {
			archName = kWinProcessorArch64;
		}
	}
	catch (...) {
		emit error(tr("Could not get Windows architecture type."));
		return "";
	}

#elif defined(Q_OS_MAC)

	archName = kMacProcessorArch;

#else

	int arch = checkProcessorArch();
	if (arch == Linux_rpm_i686) {
		archName = kLinuxProcessorArchRpm32;
	}
	else if (arch == Linux_rpm_x86_64) {
		archName = kLinuxProcessorArchRpm64;
	}
	else if (arch == Linux_deb_i686) {
		archName = kLinuxProcessorArchDeb32;
	}
	else if (arch == Linux_deb_x86_64) {
		archName = kLinuxProcessorArchDeb64;
	}
	else {
		emit error(tr("Could not get Linux architecture type."));
		return "";
	}

#endif

	QString result = kBaseUrl;
	result.append("/plugins/");
	result.append(pluginName).append("/1.0/");
	result.append(archName);
	result.append("/");
	result.append(getPluginOsSpecificName(pluginName));

	return result;
}

QString PluginManager::getPluginOsSpecificName(const QString& pluginName)
{
	QString result = pluginName;
#if defined(Q_OS_WIN)
	result.append(kWinPluginExt);
#elif defined(Q_OS_MAC)
	result = kMacPluginPrefix + pluginName + kMacPluginExt;
#else
	result = kLinuxPluginPrefix + pluginName + kLinuxPluginExt;
#endif
	return result;
}
