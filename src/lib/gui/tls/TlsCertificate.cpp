/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "TlsCertificate.h"

#include "TlsFingerprint.h"

#include "common/constants.h"
#include "net/SecureUtils.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcess>

static const char *const kCertificateKeyLength = "rsa:";
static const char *const kCertificateHashAlgorithm = "-sha256";

TlsCertificate::TlsCertificate(QObject *parent) : QObject(parent)
{
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

  try {
    deskflow::generatePemSelfSignedCert(path.toStdString());
  } catch (const std::exception &e) {
    qCritical() << "SSL Error: " << e.what();
    return false;
  }
  qDebug("tls certificate generated");
  return generateFingerprint(path);
}

bool TlsCertificate::generateFingerprint(const QString &certificateFilename)
{
  qDebug("generating tls fingerprint");
  try {
    auto fingerprint =
        deskflow::pemFileCertFingerprint(certificateFilename.toStdString(), deskflow::FingerprintType::SHA1);
    TlsFingerprint::local().trust(QString::fromStdString(deskflow::formatSSLFingerprint(fingerprint)), false);
    qDebug("tls fingerprint generated");
    return true;
  } catch (const std::exception &e) {
    qCritical() << "failed to find tls fingerprint" << e.what();
    return false;
  }
}

int TlsCertificate::getCertKeyLength(const QString &path)
{
  // TODO FIX THIS
  return 2048;
  // QStringList arguments;
  // arguments.append("rsa");
  // arguments.append("-in");
  // arguments.append(path);
  // arguments.append("-text");
  // arguments.append("-noout");

  // if (!runTool(arguments)) {
  //   qFatal("failed to get key length from certificate");
  //   return 0;
  // }

  // const QString searchStart("Private-Key: (");
  // const QString searchEnd(" bit");

  // // Get the line that contains the key length from the output
  // const auto indexStart = m_toolStdout.indexOf(searchStart);
  // const auto indexEnd = m_toolStdout.indexOf(searchEnd, indexStart);
  // const auto start = indexStart + searchStart.length();
  // const auto end = indexEnd - (indexStart + searchStart.length());
  // auto keyLength = m_toolStdout.mid(start, end);

  // return keyLength.toInt();
}
