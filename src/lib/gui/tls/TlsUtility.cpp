/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "TlsUtility.h"

#include "common/Settings.h"
#include "net/SecureUtils.h"

#include <QFile>
#include <QSslCertificate>
#include <QSslKey>
#include <QString>

namespace deskflow::gui {

TlsUtility::TlsUtility(QObject *parent) : QObject(parent)
{
  // do nothing
}

bool TlsUtility::isEnabled()
{
  return Settings::value(Settings::Security::TlsEnabled).toBool();
}

bool TlsUtility::isCertValid(const QString &certPath)
{
  const auto certs = QSslCertificate::fromPath(certPath);
  if (certs.isEmpty()) {
    //: %1 will be replaced by the certificate path
    qDebug() << tr("failed to read key from certificate file: %1").arg(certPath);
    return false;
  }

  const auto cert = certs.first();
  if (cert.isNull()) {
    //: %1 will be replaced by the certificate path
    qDebug() << tr("failed to parse certificate file: %1").arg(certPath);
    return false;
  }

  const auto key = cert.publicKey();
  if (key.isNull()) {
    //: %1 will be replaced by the certificate path
    qDebug() << tr("failed to read key from certificate file: %1").arg(certPath);
    return false;
  }

  if (key.length() != Settings::value(Settings::Security::KeySize).toInt()) {
    qDebug() << tr("key detected is the incorrect size");
    return false;
  }

  if (const auto type = key.algorithm(); (type != QSsl::Dsa || type != QSsl::Rsa)) {
    //: %1 will be replaced by the certificate path
    qDebug() << tr("failed to read RSA or DSA key from certificate file: %1").arg(certPath);
    return false;
  }

  return true;
}

int TlsUtility::getCertKeyLength(const QString &certPath)
{
  return deskflow::getCertLength(certPath.toStdString());
}

QByteArray TlsUtility::certFingerprint(const QString &certPath)
{
  QByteArray fingerprint;

  const auto certs = QSslCertificate::fromPath(certPath);
  if (certs.isEmpty()) {
    //: %1 will be replaced by the certificate path
    qDebug() << tr("failed to read key from certificate file: %1").arg(certPath);
    return fingerprint;
  }

  const auto cert = certs.first();
  if (cert.isNull()) {
    //: %1 will be replaced by the certificate path
    qWarning() << tr("failed to parse certificate file: %1").arg(certPath);
    return fingerprint;
  }

  return cert.digest(QCryptographicHash::Sha256);
}

bool TlsUtility::generateCertificate()
{
  qDebug(
      "generating tls certificate, "
      "all clients must trust the new fingerprint"
  );

  const auto keyLength = std::max(2048, Settings::value(Settings::Security::KeySize).toInt());
  const auto certPath = Settings::value(Settings::Security::Certificate).toString();

  QFileInfo info(certPath);
  if (QDir dir(info.absolutePath()); !dir.exists() && !dir.mkpath(".")) {
    qCritical("failed to create directory for tls certificate");
    return false;
  }

  try {
    deskflow::generatePemSelfSignedCert(certPath.toStdString(), keyLength);
  } catch (const std::exception &e) {
    qCritical() << "failed to generate self-signed pem cert: " << e.what();
    return false;
  }
  qDebug("tls certificate generated");
  return true;
}

} // namespace deskflow::gui
