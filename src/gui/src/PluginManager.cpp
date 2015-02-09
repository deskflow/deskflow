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

#include "PluginManager.h"

#include "DataDownloader.h"
#include "QUtility.h"
#include "ProcessorArch.h"

#include <QCoreApplication>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QMessageBox>

static const char kGetPluginDirArg[] = "--get-plugin-dir";

static QString kPluginsBaseUrl = "http://synergy-project.org/files/plugins/";
static const char kWinProcessorArch32[] = "Windows-x86";
static const char kWinProcessorArch64[] = "Windows-x64";
static const char kMacProcessorArch[] = "MacOSX-i386";
static const char kLinuxProcessorArch32[] = "Linux-i686";
static const char kLinuxProcessorArch64[] = "Linux-x86_64";
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
	m_DownloadIndex(-1),
	m_pPluginDownloader(NULL)
{
}

PluginManager::~PluginManager()
{
	if (m_pPluginDownloader != NULL) {
		delete m_pPluginDownloader;
	}
}

void PluginManager::downloadPlugins()
{
	if (m_pPluginDownloader != NULL) {
		savePlugin();
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
		url.setUrl(pluginUrl);

		if (m_pPluginDownloader == NULL) {
			m_pPluginDownloader = new DataDownloader();
			connect(m_pPluginDownloader, SIGNAL(isComplete()), this, SLOT(downloadPlugins()));
		}
		m_pPluginDownloader->download(url);
	}
}

void PluginManager::savePlugin()
{
	QString pluginDir = getPluginDir();
	if (pluginDir.isEmpty()) {
		return;
	}

	QString filename = pluginDir;
	QString pluginName = m_PluginList.at(m_DownloadIndex);
	pluginName = getPluginOSSpecificName(pluginName);
	filename.append(QDir::separator()).append(pluginName);

	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly)) {
		QMessageBox::warning(
			(QWidget*)parent(), "Synergy",
			tr("Failed to download plugin %1 to location: %2")
			.arg(m_PluginList.at(m_DownloadIndex))
			.arg(pluginDir));
		return;
	}

	file.write(m_pPluginDownloader->data());
	file.close();
}


QString PluginManager::getPluginDir()
{
	QString program(QCoreApplication::applicationDirPath() + "/syntool");

	QProcess process;
	process.setReadChannel(QProcess::StandardOutput);
	QStringList args(kGetPluginDirArg);
	process.start(program, args);
	bool success = process.waitForStarted();

	QString out, error;
	if (success)
	{
		if (process.waitForFinished()) {
			out = process.readAllStandardOutput();
			error = process.readAllStandardError();
		}
	}

	out = out.trimmed();
	error = error.trimmed();

	if (out.isEmpty() ||
		!error.isEmpty() ||
		!success ||
		process.exitCode() != 0)
	{
		QMessageBox::critical(
			(QWidget*)parent(), tr("Synergy"),
			tr("An error occured while trying to get "
			"plugin directory from syntool. Code: %1\nError: %2")
			.arg(process.exitCode())
			.arg(error.isEmpty() ? "Unknown" : error));
		return "";
	}

	// create the path if not exist
	// TODO: synergy folder should be hidden
	QDir dir(out);
	if (!dir.exists()) {
		dir.mkpath(".");
	}

	return out;
}

QString PluginManager::getPluginUrl(const QString& pluginName)
{
	QString result;
	result = kPluginsBaseUrl.append(pluginName).append("/1.0/");

	int arch = checkProcessorArch();
	if (arch == Win_x86) {
		result.append(kWinProcessorArch32);
	}
	else if (arch == Win_x64) {
		result.append(kWinProcessorArch64);
	}
	else if (arch == Mac_i386) {
		result.append(kMacProcessorArch);
	}
	else if (arch == Linux_i686) {
		result.append(kLinuxProcessorArch32);
	}
	else if (arch == Linux_x86_64) {
		result.append(kLinuxProcessorArch64);
	}
	else {
		QMessageBox::critical(
			(QWidget*)parent(), tr("Synergy"),
			tr("Failed to detect system architecture."));
		return "";
	}
	result.append("/");
	result.append(getPluginOSSpecificName(pluginName));

	return result;
}

QString PluginManager::getPluginOSSpecificName(const QString& pluginName)
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
