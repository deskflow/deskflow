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
static QString kCertificateLifetime = "365";
static QString kCertificateSubjectInfo = "/CN=Synergy";
static QString kCertificateFilename = "Synergy.pem";
static QString kUnixOpenSslCommand = "openssl";

#if defined(Q_OS_WIN)
static const char kWinPluginExt[] = ".dll";
static const char kWinOpenSslSetup[] = "openssl-1.0.2-Windows-x86.exe";
static const char kWinOpenSslBinary[] = "OpenSSL\\openssl.exe";

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

void PluginManager::saveOpenSslSetup()
{
	QDir dir(m_ProfileDir);
	if (!dir.exists()) {
		dir.mkpath(".");
	}

#if defined(Q_OS_WIN)

	QString filename =
		QString("%1\\%2")
		.arg(m_ProfileDir)
		.arg(kWinOpenSslSetup);

	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly)) {
		emit error(
			tr("Failed to save certificate tool to: %1")
			.arg(m_ProfileDir));
		return;
	}

	file.write(m_DataDownloader.data());
	file.close();

	QStringList installArgs;
	installArgs.append("-s");
	installArgs.append("-y");

	if (!runProgram(filename, installArgs, QStringList())) {
		return;
	}

	// openssl installer no longer needed
	QFile::remove(filename);

	emit info(tr("SSL tools ready"));
#endif

	emit openSslBinaryReady();
}

void PluginManager::generateCertificate()
{
	QString openSslProgramFile;

#if defined(Q_OS_WIN)
	openSslProgramFile = QCoreApplication::applicationDirPath();
	openSslProgramFile.append("\\").append(kWinOpenSslBinary);
#else
	openSslProgramFile = kUnixOpenSslCommand;
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

	QString subInfo(kCertificateSubjectInfo);
	arguments.append(subInfo);

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

	QStringList environment;

#if defined(Q_OS_WIN)
	environment << QString("OPENSSL_CONF=%1\\OpenSSL\\synergy.conf")
		.arg(QCoreApplication::applicationDirPath());
#endif

	if (!runProgram(openSslProgramFile, arguments, environment)) {
		return;
	}

	emit info(tr("SSL certificate generated"));

	// generate fingerprint
	arguments.clear();
	arguments.append("x509");
	arguments.append("-fingerprint");
	arguments.append("-sha1");
	arguments.append("-noout");
	arguments.append("-in");
	arguments.append(filename);

	if (!runProgram(openSslProgramFile, arguments, environment)) {
		return;
	}

	// write the standard output into file
	filename.clear();
	filename.append(Fingerprint::local().filePath());

	// only write the fingerprint part
	int i = m_standardOutput.indexOf("=");
	if (i != -1) {
		i++;
		QString fingerprint = m_standardOutput.mid(i, m_standardOutput.size() - i);
		
		Fingerprint::local().trust(fingerprint, false);
		emit info(tr("SSL fingerprint generated"));
	}

	emit generateCertificateFinished();
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
	pluginName = getPluginOsSpecificName(pluginName);
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
	result.append(getPluginOsSpecificName(pluginName));

	return result;
}

QString PluginManager::getOpenSslSetupUrl()
{
	QString result;

#if defined(Q_OS_WIN)
	result = kBaseUrl;
	result.append("/tools/");
	result.append(kWinOpenSslSetup);
#endif

	return result;
}

bool PluginManager::checkOpenSslBinary()
{
	// assume OpenSsl is unavailable on Windows,
	// but always available on both Mac and Linux
#if defined(Q_OS_WIN)
	return false;
#else
	return true;
#endif
}

bool PluginManager::runProgram(
	const QString& program, const QStringList& args, const QStringList& env)
{
	QProcess process;
	process.setEnvironment(env);
	process.start(program, args);

	bool success = process.waitForStarted();

	QString standardError;
	if (success && process.waitForFinished())
	{
		m_standardOutput = process.readAllStandardOutput().trimmed();
		standardError = process.readAllStandardError().trimmed();
	}

	int code = process.exitCode();
	if (!success || code != 0)
	{
		emit error(
			QString("Program failed: %1\n\nCode: %2\nError: %3")
			.arg(program)
			.arg(process.exitCode())
			.arg(standardError.isEmpty() ? "Unknown" : standardError));
		return false;
	}

	return true;
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
