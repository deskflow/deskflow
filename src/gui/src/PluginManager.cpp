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

#include "DirectoryManager.h"
#include "CommandProcess.h"
#include "DataDownloader.h"
#include "QUtility.h"
#include "ProcessorArch.h"

#include <QFile>
#include <QDir>

static QString kPluginsBaseUrl = "http://synergy-project.org/files/plugins/";
static const char kWinProcessorArch32[] = "Windows-x86";
static const char kWinProcessorArch64[] = "Windows-x64";
static const char kMacProcessorArch[] = "MacOSX-i386";
static const char kLinuxProcessorArchDeb32[] = "Linux-i686-deb";
static const char kLinuxProcessorArchDeb64[] = "Linux-x86_64-deb";
static const char kLinuxProcessorArchRpm32[] = "Linux-i686-rpm";
static const char kLinuxProcessorArchRpm64[] = "Linux-x86_64-rpm";
static QString kOpenSSLBaseUrl = "http://synergy-foss.org/files/tools/";
static QString kCertificateLifetime = "365";
static QString kCertificateSubjectInfo = "/CN=Synergy";
static QString kCertificateFilename = "Synergy.pem";
static QString kUnixOpenSSLCommand = "openssl";

#if defined(Q_OS_WIN)
static const char kWinPluginExt[] = ".dll";
static const char kWinOpenSSLBinary[] = "openssl.exe";

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
	m_PluginDir = DirectoryManager::getPluginDir();
	if (m_PluginDir.isEmpty()) {
		emit error(tr("Failed to get plugin directory."));
	}

	m_ProfileDir = DirectoryManager::getProfileDir();
	if (m_ProfileDir.isEmpty()) {
		emit error(tr("Failed to get profile directory."));
	}
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
		if (pluginUrl.isEmpty()) {
			return;
		}
		url.setUrl(pluginUrl);

		if (m_pPluginDownloader == NULL) {
			m_pPluginDownloader = new DataDownloader();
			connect(m_pPluginDownloader, SIGNAL(isComplete()), this, SLOT(downloadPlugins()));
		}
		m_pPluginDownloader->download(url);
	}
}

void PluginManager::saveOpenSSLBinary()
{
	QDir dir(m_ProfileDir);
	if (!dir.exists()) {
		dir.mkpath(".");
	}

	QString filename = m_ProfileDir;
#if defined(Q_OS_WIN)
	filename.append("\\").append(kWinOpenSSLBinary);
#endif


	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly)) {
		emit error(
				tr("Failed to download OpenSSl to location: %1")
				.arg(m_ProfileDir));
		return;
	}

	file.write(m_pPluginDownloader->data());
	file.close();

	emit openSSLBinaryReady();
}

void PluginManager::generateCertificate()
{
	connect(
		this,
		SIGNAL(openSSLBinaryReady()),
		this,
		SLOT(doGenerateCertificate()));

	downloadOpenSSLBinary();
}

void PluginManager::savePlugin()
{
	// create the path if not exist
	QDir dir(m_PluginDir);
	if (!dir.exists()) {
		dir.mkpath(".");
	}

	QString filename = m_PluginDir;
	QString pluginName = m_PluginList.at(m_DownloadIndex);
	pluginName = getPluginOSSpecificName(pluginName);
	filename.append(QDir::separator()).append(pluginName);

	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly)) {
		emit error(
				tr("Failed to download plugin %1 to location: %2")
				.arg(m_PluginList.at(m_DownloadIndex))
				.arg(m_PluginDir));

		return;
	}

	file.write(m_pPluginDownloader->data());
	file.close();
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
	else if (arch == Linux_rpm_i686) {
		result.append(kLinuxProcessorArchRpm32);
	}
	else if (arch == Linux_rpm_x86_64) {
		result.append(kLinuxProcessorArchRpm64);
	}
	else if (arch == Linux_deb_i686) {
		result.append(kLinuxProcessorArchDeb32);
	}
	else if (arch == Linux_deb_x86_64) {
		result.append(kLinuxProcessorArchDeb64);
	}
	else {
		emit error(
			tr("Failed to get the url of plugin %1 .")
			.arg(pluginName));
		return "";
	}
	result.append("/");
	result.append(getPluginOSSpecificName(pluginName));

	return result;
}

QString PluginManager::getOpenSSLBinaryUrl()
{
	QString result;
#if defined(Q_OS_WIN)
	result = kOpenSSLBaseUrl.append(kWinOpenSSLBinary);
#endif

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

bool PluginManager::checkOpenSSLBinary()
{
	bool exist = false;
#if defined(Q_OS_WIN)
	QString openSSLFilename = m_ProfileDir;
	openSSLFilename.append("\\").append(kWinOpenSSLBinary);
	QDir dir(openSSLFilename);
	if (dir.exists()) {
		exist = true;
	}
#else
	// assume OpenSSL is always installed on both Mac and Linux
	exist = true;
#endif

	return exist;
}

void PluginManager::downloadOpenSSLBinary()
{
	if (checkOpenSSLBinary()) {
		emit openSSLBinaryReady();
		return;
	}

	QUrl url;
	QString pluginUrl = getOpenSSLBinaryUrl();
	url.setUrl(pluginUrl);

	disconnect(
		m_pPluginDownloader,
		SIGNAL(isComplete()),
		this,
		SLOT(downloadPlugins()));
	connect(
		m_pPluginDownloader,
		SIGNAL(isComplete()),
		this,
		SLOT(saveOpenSSLBinary()));

	m_pPluginDownloader->download(url);
}

void PluginManager::doGenerateCertificate()
{
	QString openSSLFilename = m_ProfileDir;
#if defined(Q_OS_WIN)
	openSSLFilename.append("\\").append(kWinOpenSSLBinary);
#else
	openSSLFilename = kUnixOpenSSLCommand;
#endif

	QStringList arguments;

	// self signed certificate
	arguments.append("req");
	arguments.append("-x509");
	arguments.append("-nodes");

	// valide duration
	arguments.append("-days");
	arguments.append(kCertificateLifetime);

	// subject information
	arguments.append("-subj");

	QString info(kCertificateSubjectInfo);
	arguments.append(info);

	// private key
	arguments.append("-newkey");
	arguments.append("rsa:1024");

	// key output filename
	arguments.append("-keyout");
	QString filename = m_ProfileDir;
	filename.append(QDir::separator()).append(kCertificateFilename);
	arguments.append(filename);

	// certificate output filename
	arguments.append("-out");
	arguments.append(filename);

	// update command and arguments
	CommandProcess commandProcess(openSSLFilename, arguments);
	commandProcess.run();

	emit generateCertificateFinished();
}
