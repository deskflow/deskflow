/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2015 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "TlsCertificate.h"

#include "common/Settings.h"
#include "net/Fingerprint.h"
#include "net/FingerprintDatabase.h"
#include "net/SecureUtils.h"

#include <QDir>

TlsCertificate::TlsCertificate(QObject *parent) : QObject(parent)
{
  // do nothing
}

bool TlsCertificate::generateCertificate(const QString &path, int keyLength) const
{
  qDebug("generating tls certificate: %s", qUtf8Printable(path));

  QFileInfo info(path);
  if (QDir dir(info.absolutePath()); !dir.exists() && !dir.mkpath(".")) {
    qCritical("failed to create directory for tls certificate");
    return false;
  }

  try {
    deskflow::generatePemSelfSignedCert(path.toStdString(), keyLength);
  } catch (const std::exception &e) {
    qCritical() << "failed to generate self-signed pem cert: " << e.what();
    return false;
  }
  qDebug("tls certificate generated");
  return generateFingerprint(path);
}

bool TlsCertificate::generateFingerprint(const QString &certificateFilename) const
{
  qDebug("generating tls fingerprint");
  const std::string certPath = certificateFilename.toStdString();
  FingerprintDatabase db;
  db.addTrusted(deskflow::pemFileCertFingerprint(certPath, Fingerprint::Type::SHA256));
  return db.write(Settings::tlsLocalDb());
}

int TlsCertificate::getCertKeyLength(const QString &path) const
{
  return deskflow::getCertLength(path.toStdString());
}
