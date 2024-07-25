/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2016 Symless Ltd.
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

#include "TlsCertificate.h"

#include "TlsFingerprint.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcess>

// RSA Bit length (e.g. 1024/2048/4096)
static const char *const kCertificateKeyLength = "rsa:";

// fingerprint hashing algorithm
static const char *const kCertificateHashAlgorithm = "-sha256";

static const char *const kCertificateLifetime = "365";
static const char *const kCertificateSubjectInfo = "/CN=Synergy";
static const char *const kCertificateFilename = "Synergy.pem";
static const char *const kSslDir = "SSL";

#if defined(Q_OS_WIN)
static const char *const kWinOpenSslDir = "OpenSSL";
static const char *const kWinOpenSslBinary = "openssl.exe";
static const char *const kConfigFile = "synergy.conf";
#elif defined(Q_OS_UNIX)
static const char *const kUnixOpenSslCommand = "openssl";
#endif

namespace synergy::gui {
#if defined(Q_OS_WIN)

QString openSslWindowsDir() {

  auto appDir = QDir(QCoreApplication::applicationDirPath());
  auto openSslDir = QDir(appDir.filePath(kWinOpenSslDir));

  // in production, openssl is deployed with the app.
  // in development, we can use the openssl path available at compile-time.
  if (!openSslDir.exists()) {
    openSslDir = QDir(OPENSSL_PATH);
  }

  // if the path still isn't found, something is seriously wrong.
  if (!openSslDir.exists()) {
    qFatal() << "OpenSSL dir not found: " << openSslDir;
  }

  return QDir::cleanPath(openSslDir.absolutePath());
}

QString openSslWindowsBinary() {
  auto dir = QDir(openSslWindowsDir());
  auto path = dir.filePath(kWinOpenSslBinary);

  // when installed, there is no openssl bin dir; it's installed at the base.
  // in development, we use the standard dir structure for openssl (bin dir).
  if (!QFile::exists(path)) {
    auto binDir = QDir(dir.filePath("bin"));
    path = binDir.filePath(kWinOpenSslBinary);
  }

  // if the path still isn't found, something is seriously wrong.
  if (!QFile::exists(path)) {
    qFatal() << "OpenSSL binary not found: " << path;
  }

  return path;
}

#endif

} // namespace synergy::gui

using namespace synergy::gui;

TlsCertificate::TlsCertificate(QObject *parent) : QObject(parent) {
  m_ProfileDir = m_CoreInterface.getProfileDir();
  if (m_ProfileDir.isEmpty()) {
    emit error(tr("Failed to get profile directory."));
  }
}

bool TlsCertificate::runTool(const QStringList &args) {
  QString program;
#if defined(Q_OS_WIN)
  program = openSslWindowsBinary();
#else
  program = kUnixOpenSslCommand;
#endif

  QStringList environment;
#if defined(Q_OS_WIN)
  auto openSslDir = QDir(openSslWindowsDir());
  auto config = QDir::cleanPath(openSslDir.filePath(kConfigFile));
  environment << QString("OPENSSL_CONF=%1").arg(config);
#endif

  QProcess process;
  process.setEnvironment(environment);
  process.start(program, args);

  bool success = process.waitForStarted();

  QString standardError;
  if (success && process.waitForFinished()) {
    m_ToolOutput = process.readAllStandardOutput().trimmed();
    standardError = process.readAllStandardError().trimmed();
  }

  if (int code = process.exitCode(); !success || code != 0) {
    emit error(QString("SSL tool failed: %1\n\nCode: %2\nError: %3")
                   .arg(program)
                   .arg(process.exitCode())
                   .arg(standardError.isEmpty() ? "Unknown" : standardError));
    return false;
  }

  return true;
}

void TlsCertificate::generateCertificate(
    const QString &path, const QString &keyLength, bool forceGen) {
  QString sslDirPath =
      QString("%1%2%3").arg(m_ProfileDir).arg(QDir::separator()).arg(kSslDir);

  QString filename = QString("%1%2%3")
                         .arg(sslDirPath)
                         .arg(QDir::separator())
                         .arg(kCertificateFilename);

  QString keySize = kCertificateKeyLength + keyLength;

  const QString pathToUse = path.isEmpty() ? filename : path;

  if (QFile file(pathToUse); !file.exists() || forceGen) {
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
    arguments.append(keySize);

    if (QDir sslDir(sslDirPath); !sslDir.exists()) {
      sslDir.mkpath(".");
    }

    // key output filename
    arguments.append("-keyout");
    arguments.append(pathToUse);

    // certificate output filename
    arguments.append("-out");
    arguments.append(pathToUse);

    if (!runTool(arguments)) {
      return;
    }

    generateFingerprint(pathToUse);
    emit info(tr("SSL certificate generated."));
  }

  emit generateFinished();
}

void TlsCertificate::generateFingerprint(const QString &certificateFilename) {
  QStringList arguments;
  arguments.append("x509");
  arguments.append("-fingerprint");
  arguments.append(kCertificateHashAlgorithm);
  arguments.append("-noout");
  arguments.append("-in");
  arguments.append(certificateFilename);

  if (!runTool(arguments)) {
    return;
  }

  // find the fingerprint from the tool output
  auto i = m_ToolOutput.indexOf("=");
  if (i != -1) {
    i++;
    QString fingerprint = m_ToolOutput.mid(i, m_ToolOutput.size() - i);

    TlsFingerprint::local().trust(fingerprint, false);
    emit info(tr("SSL fingerprint generated."));
  } else {
    emit error(tr("Failed to find SSL fingerprint."));
  }
}

QString TlsCertificate::getCertKeyLength(const QString &path) {

  QStringList arguments;
  arguments.append("rsa");
  arguments.append("-in");
  arguments.append(path);
  arguments.append("-text");
  arguments.append("-noout");

  if (!runTool(arguments)) {
    return QString();
  }
  const QString searchStart("Private-Key: (");
  const QString searchEnd(" bit");

  // Get the line that contains the key length from the output
  const auto indexStart = m_ToolOutput.indexOf(searchStart);
  const auto indexEnd = m_ToolOutput.indexOf(searchEnd, indexStart);
  const auto start = indexStart + searchStart.length();
  const auto end = indexEnd - (indexStart + searchStart.length());
  auto keyLength = m_ToolOutput.mid(start, end);

  return keyLength;
}
