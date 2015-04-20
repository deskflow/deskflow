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

#include "SslCertificate.h"

#include "Fingerprint.h"

#include <QProcess>
#include <QDir>
#include <QCoreApplication>

static const char kCertificateLifetime[] = "365";
static const char kCertificateSubjectInfo[] = "/CN=Synergy";
static const char kCertificateFilename[] = "Synergy.pem";
static const char kSslDir[] = "SSL";
static const char kUnixOpenSslCommand[] = "openssl";

#if defined(Q_OS_WIN)
static const char kWinOpenSslBinary[] = "OpenSSL\\openssl.exe";
#endif

SslCertificate::SslCertificate(QObject *parent) :
	QObject(parent)
{
	m_ProfileDir = m_CoreInterface.getProfileDir();
	if (m_ProfileDir.isEmpty()) {
	  emit error(tr("Failed to get profile directory."));
	}
}

bool SslCertificate::checkOpenSslBinary()
{
  // assume OpenSsl is unavailable on Windows,
  // but always available on both Mac and Linux
#if defined(Q_OS_WIN)
  return false;
#else
  return true;
#endif
}

bool SslCertificate::runProgram(
  const QString& program,
  const QStringList& args,
  const QStringList& env)
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

void SslCertificate::generateCertificate()
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

  QString sslDirPath = QString("%1%2%3")
	.arg(m_ProfileDir)
	.arg(QDir::separator())
	.arg(kSslDir);

  QDir sslDir(sslDirPath);
  if (!sslDir.exists()) {
	sslDir.mkdir(".");
  }

  QString filename = QString("%1%2%3")
	.arg(sslDirPath)
	.arg(QDir::separator())
	.arg(kCertificateFilename);

  // key output filename
  arguments.append("-keyout");
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
