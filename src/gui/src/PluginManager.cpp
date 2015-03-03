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

#include "CoreInterface.h"
#include "CommandProcess.h"
#include "DataDownloader.h"
#include "QUtility.h"
#include "ProcessorArch.h"

#include <QFile>
#include <QDir>
#include <QProcess>

static QString kBaseUrl = "http://synergy-project.org/files";
static const char kWinProcessorArch32[] = "Windows-x86";
static const char kWinProcessorArch64[] = "Windows-x64";
static const char kMacProcessorArch[] = "MacOSX-i386";
static const char kLinuxProcessorArchDeb32[] = "Linux-i686-deb";
static const char kLinuxProcessorArchDeb64[] = "Linux-x86_64-deb";
static const char kLinuxProcessorArchRpm32[] = "Linux-i686-rpm";
static const char kLinuxProcessorArchRpm64[] = "Linux-x86_64-rpm";
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

void PluginManager::downloadPlugins()
{
	if (m_DataDownloader.isFinished()) {
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

		connect(&m_DataDownloader, SIGNAL(isComplete()), this, SLOT(downloadPlugins()));

		m_DataDownloader.download(url);
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
			tr("Failed to save certificate tool to: %1")
			.arg(m_ProfileDir));
		return;
	}

	file.write(m_DataDownloader.data());
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
				tr("Failed to download '%1' plugin to: %2")
				.arg(m_PluginList.at(m_DownloadIndex))
				.arg(m_PluginDir));

		return;
	}

	file.write(m_DataDownloader.data());
	file.close();
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
	result.append(getPluginOSSpecificName(pluginName));

	return result;
}

QString PluginManager::getOpenSSLBinaryUrl()
{
	QString result;

#if defined(Q_OS_WIN)
	result = kBaseUrl;
	result.append("/tools/");
	result.append(kWinOpenSSLBinary);
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
	// assume OpenSSL is unavailable on Windows,
	// but always available on both Mac and Linux
#if defined(Q_OS_WIN)
	return false;
#else
	return true;
#endif
}

void PluginManager::downloadOpenSSLBinary()
{
	if (checkOpenSSLBinary()) {
		emit openSSLBinaryReady();
		return;
	}

	QUrl url;
	QString openSslUrl = getOpenSSLBinaryUrl();
	url.setUrl(openSslUrl);

	disconnect(
		&m_DataDownloader,
		SIGNAL(isComplete()),
		this,
		SLOT(downloadPlugins()));

	connect(
		&m_DataDownloader,
		SIGNAL(isComplete()),
		this,
		SLOT(saveOpenSSLBinary()));

	m_DataDownloader.download(url);
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

	QProcess process;
	process.start(openSSLFilename, arguments);
	bool success = process.waitForStarted();

	QString standardOutput, standardError;
	if (success && process.waitForFinished())
	{
		standardOutput = process.readAllStandardOutput().trimmed();
		standardError = process.readAllStandardError().trimmed();
	}

	int code = process.exitCode();
	if (!standardError.isEmpty() || !success || code != 0)
	{
		emit error(
			QString("Failed to generate certificate.\n\nCode: %1\nError: %2")
				.arg(process.exitCode())
				.arg(standardError.isEmpty() ? "Unknown" : standardError));
		return;
	}

	emit generateCertificateFinished();
}
