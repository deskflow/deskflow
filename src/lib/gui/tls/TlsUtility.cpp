/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "TlsUtility.h"

#include "TlsCertificate.h"
#include "common/Settings.h"

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

bool TlsUtility::generateCertificate() const
{
  qDebug(
      "generating tls certificate, "
      "all clients must trust the new fingerprint"
  );

  auto length = Settings::value(Settings::Security::KeySize).toInt();

  if (length < 2048) {
    length = 2048;
    qDebug("selected size too small setting certificate size to 2048");
    Settings::setValue(Settings::Security::KeySize, 2048);
  }

  const auto certificate = Settings::value(Settings::Security::Certificate).toString();
  return m_certificate.generateCertificate(certificate, length);
}

} // namespace deskflow::gui
