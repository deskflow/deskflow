/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Symless Ltd.
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

static const char *const kCertificateKeyLength = "rsa:";
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

#if defined(Q_OS_WIN)

namespace synergy::gui {

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
    qFatal() << "openssl dir not found: " << openSslDir;
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
    qFatal() << "openssl binary not found: " << path;
  }

  return path;
}

} // namespace synergy::gui

using namespace synergy::gui;

#endif

TlsCertificate::TlsCertificate(QObject *parent) : QObject(parent) {
  m_profileDir = m_coreInterface.getProfileDir();
  if (m_profileDir.isEmpty()) {
    qCritical("empty profile directory result");
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
  if (!QFile::exists(config)) {
    qDebug("openssl config file not found: %s", qUtf8Printable(config));

    // if the expected production file location doesn't exist, try the dev path.
    config = QDir::cleanPath(QString("res/openssl/%1").arg(kConfigFile));

    // if it still isn't there, then there's something seriously wrong.
    if (!QFile::exists(config)) {
      qFatal() << "openssl config file not found: " << config;
    }
  }

  environment << QString("OPENSSL_CONF=%1").arg(config);
#endif

  qDebug(
      "running: %s %s", qUtf8Printable(program),
      qUtf8Printable(args.join(" ")));

  QProcess process;

  for (const auto &envVar : environment) {
    qDebug("setting env var %s", qUtf8Printable(envVar));
  }

  process.setEnvironment(environment);

  process.start(program, args);

  bool success = process.waitForStarted();

  QString stderrOutput;
  if (success && process.waitForFinished()) {
    m_toolStdout = process.readAllStandardOutput().trimmed();
    stderrOutput = process.readAllStandardError().trimmed();
  }

  if (int code = process.exitCode(); !success || code != 0) {
    qDebug(
        "openssl failed with code %d: %s", code, qUtf8Printable(stderrOutput));

    qCritical(
        "failed to generate TLS certificate:\n\n%s",
        qUtf8Printable(stderrOutput));
    return false;
  }

  return true;
}

void TlsCertificate::generateCertificate(
    const QString &path, const QString &keyLength, bool createFile) {
  QString sslDirPath =
      QString("%1%2%3").arg(m_profileDir).arg(QDir::separator()).arg(kSslDir);

  QString defaultPath = QString("%1%2%3")
                            .arg(sslDirPath)
                            .arg(QDir::separator())
                            .arg(kCertificateFilename);

  QString keySize = kCertificateKeyLength + keyLength;

  const QString pathToUse =
      QDir::cleanPath(path.isEmpty() ? defaultPath : path);

  qDebug("generating tls certificate: %s", qUtf8Printable(pathToUse));

  QFile file(pathToUse);
  if (!file.exists() && !createFile) {
    qCritical(
        "tls certificate file does not exist: %s", qUtf8Printable(pathToUse));
    return;
  }

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

  if (runTool(arguments)) {
    generateFingerprint(pathToUse);
    qDebug("tls certificate generated");
  } else {
    // error dialog already shown
    qDebug("skipping fingerprint generation, as generate failed");
    return;
  }
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
  auto i = m_toolStdout.indexOf("=");
  if (i != -1) {
    i++;
    QString fingerprint = m_toolStdout.mid(i, m_toolStdout.size() - i);

    TlsFingerprint::local().trust(fingerprint, false);
    qDebug("tls fingerprint generated");
  } else {
    qCritical("failed to find tls fingerprint in tls tool output");
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
  const auto indexStart = m_toolStdout.indexOf(searchStart);
  const auto indexEnd = m_toolStdout.indexOf(searchEnd, indexStart);
  const auto start = indexStart + searchStart.length();
  const auto end = indexEnd - (indexStart + searchStart.length());
  auto keyLength = m_toolStdout.mid(start, end);

  return keyLength;
}
