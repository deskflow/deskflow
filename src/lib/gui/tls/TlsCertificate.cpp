/*
 * Deskflow -- mouse and keyboard sharing utility
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
static const char *const kCertificateSubjectInfo = "/CN=Deskflow";

#if defined(Q_OS_WIN)
static const char *const kWinOpenSslDir = "OpenSSL";
static const char *const kWinOpenSslBinary = "openssl.exe";
static const char *const kConfigFile = "openssl.cnf";
#elif defined(Q_OS_UNIX)
static const char *const kUnixOpenSslCommand = "openssl";
#endif

#if defined(Q_OS_WIN)

namespace deskflow::gui {

QString openSslWindowsDir()
{

  auto appDir = QDir(QCoreApplication::applicationDirPath());
  auto openSslDir = QDir(appDir.filePath(kWinOpenSslDir));

  // in production, openssl is deployed with the app.
  // in development, we can use the openssl path available at compile-time.
  if (!openSslDir.exists()) {
    openSslDir = QDir(OPENSSL_EXE_DIR);
  }

  // if the path still isn't found, something is seriously wrong.
  const auto path = openSslDir.absolutePath();
  if (openSslDir.exists()) {
    qDebug("openssl dir: %s", qUtf8Printable(path));
  } else {
    qFatal("openssl dir not found: %s", qUtf8Printable(path));
  }

  return QDir::cleanPath(path);
}

QString openSslWindowsBinary()
{
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

} // namespace deskflow::gui

using namespace deskflow::gui;

#endif

TlsCertificate::TlsCertificate(QObject *parent) : QObject(parent)
{
}

bool TlsCertificate::runTool(const QStringList &args)
{
#if defined(Q_OS_WIN)
  const auto program = openSslWindowsBinary();
#else
  const auto program = kUnixOpenSslCommand;
#endif

  QStringList environment;

// Windows is special! :)
// For OpenSSL, it's very common to bundle the openssl.exe and openssl.cnf files
// with the application. This is made a little more complex in the Windows dev
// env, because vcpkg can't find the openssl.cnf file by default, so we need to
// give it a bit of guidance by setting the `OPENSSL_CONF` env var.
#if defined(Q_OS_WIN)
  const auto openSslDir = QDir(openSslWindowsDir());
  const auto config = QDir::cleanPath(openSslDir.filePath(kConfigFile));
  environment << QString("OPENSSL_CONF=%1").arg(config);
#endif

  QProcess process;
  process.setEnvironment(environment);
  for (const auto &envVar : std::as_const(environment)) {
    qDebug("set env var: %s", qUtf8Printable(envVar));
  }

  qDebug("running: %s %s", qUtf8Printable(program), qUtf8Printable(args.join(" ")));
  process.start(program, args);
  bool success = process.waitForStarted();

  QString toolStderr;
  if (success && process.waitForFinished()) {
    m_toolStdout = process.readAllStandardOutput().trimmed();
    toolStderr = process.readAllStandardError().trimmed();
  }

  if (int code = process.exitCode(); !success || code != 0) {
    qDebug("openssl failed with code %d: %s", code, qUtf8Printable(toolStderr));

    qCritical("failed to generate tls certificate:\n\n%s", qUtf8Printable(toolStderr));
    return false;
  }

  return true;
}

bool TlsCertificate::generateCertificate(const QString &path, int keyLength)
{
  qDebug("generating tls certificate: %s", qUtf8Printable(path));

  QFileInfo info(path);
  QDir dir(info.absolutePath());
  if (!dir.exists() && !dir.mkpath(".")) {
    qCritical("failed to create directory for tls certificate");
    return false;
  }

  QString keySize = kCertificateKeyLength + QString::number(keyLength);

  QStringList arguments;

  // self signed certificate
  arguments.append("req");
  arguments.append("-x509");
  arguments.append("-nodes");

  // valid duration
  arguments.append("-days");
  arguments.append(kCertificateLifetime);

  // subject information
  arguments.append("-subj");

  QString subInfo(kCertificateSubjectInfo);
  arguments.append(subInfo);

  // private key
  arguments.append("-newkey");
  arguments.append(keySize);

  // key output filename
  arguments.append("-keyout");
  arguments.append(path);

  // certificate output filename
  arguments.append("-out");
  arguments.append(path);

  if (runTool(arguments)) {
    qDebug("tls certificate generated");

    return generateFingerprint(path);
  } else {
    qCritical("failed to generate tls certificate");
    return false;
  }
}

bool TlsCertificate::generateFingerprint(const QString &certificateFilename)
{
  qDebug("generating tls fingerprint");

  QStringList arguments;
  arguments.append("x509");
  arguments.append("-fingerprint");
  arguments.append(kCertificateHashAlgorithm);
  arguments.append("-noout");
  arguments.append("-in");
  arguments.append(certificateFilename);

  if (!runTool(arguments)) {
    qCritical("failed to generate tls fingerprint");
    return false;
  }

  // find the fingerprint from the tool output
  auto i = m_toolStdout.indexOf("=");
  if (i != -1) {
    i++;
    QString fingerprint = m_toolStdout.mid(i, m_toolStdout.size() - i);

    TlsFingerprint::local().trust(fingerprint, false);
    qDebug("tls fingerprint generated");
    return true;
  } else {
    qCritical("failed to find tls fingerprint in tls tool output");
    return false;
  }
}

int TlsCertificate::getCertKeyLength(const QString &path)
{

  QStringList arguments;
  arguments.append("rsa");
  arguments.append("-in");
  arguments.append(path);
  arguments.append("-text");
  arguments.append("-noout");

  if (!runTool(arguments)) {
    qFatal("failed to get key length from certificate");
    return 0;
  }

  const QString searchStart("Private-Key: (");
  const QString searchEnd(" bit");

  // Get the line that contains the key length from the output
  const auto indexStart = m_toolStdout.indexOf(searchStart);
  const auto indexEnd = m_toolStdout.indexOf(searchEnd, indexStart);
  const auto start = indexStart + searchStart.length();
  const auto end = indexEnd - (indexStart + searchStart.length());
  auto keyLength = m_toolStdout.mid(start, end);

  return keyLength.toInt();
}
