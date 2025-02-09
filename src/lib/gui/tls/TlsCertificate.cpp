/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2015 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "TlsCertificate.h"

#include "base/finally.h"
#include "common/constants.h"
#include "gui/core/CoreTool.h"
#include "net/FingerprintData.h"
#include "net/FingerprintDatabase.h"
#include "net/SecureUtils.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcess>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

TlsCertificate::TlsCertificate(QObject *parent) : QObject(parent)
{
  CoreTool coreTool;
  m_profileDir = coreTool.getProfileDir();
  if (m_profileDir.isEmpty())
    qCritical() << "unable to get profile dir";
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

  try {
    deskflow::generatePemSelfSignedCert(path.toStdString(), keyLength);
  } catch (const std::exception &e) {
    qCritical() << "failed to generate self-signed pem cert: " << e.what();
    return false;
  }
  qDebug("tls certificate generated");
  return generateFingerprint(path);
}

bool TlsCertificate::generateFingerprint(const QString &certificateFilename)
{
  qDebug("generating tls fingerprint");
  const std::string certPath = certificateFilename.toStdString();
  try {
    deskflow::FingerprintDatabase db;
    db.addTrusted(deskflow::pemFileCertFingerprint(certPath, deskflow::FingerprintType::SHA1));
    db.addTrusted(deskflow::pemFileCertFingerprint(certPath, deskflow::FingerprintType::SHA256));
    db.write(QStringLiteral("%1/%2").arg(getTlsDir(), kFingerprintLocalFilename).toStdString());

    qDebug("tls fingerprint generated");
    return true;
  } catch (const std::exception &e) {
    qCritical() << "failed to find tls fingerprint: " << e.what();
    return false;
  }
}

int TlsCertificate::getCertKeyLength(const QString &path)
{
  return deskflow::getCertLength(path.toStdString());
}

QString TlsCertificate::getCertificatePath() const
{
  return QStringLiteral("%1/%2/%3").arg(m_profileDir, kSslDir, kCertificateFilename);
}

QString TlsCertificate::getTlsDir() const
{
  return QStringLiteral("%1/%2").arg(m_profileDir, kSslDir);
}

bool TlsCertificate::isCertificateValid(const QString &path)
{
  OpenSSL_add_all_algorithms();
  ERR_load_crypto_strings();

  auto fp = deskflow::fopenUtf8Path(path.toStdString(), "r");
  if (!fp) {
    qWarning() << tr("could not read from default certificate file");
    return false;
  }
  auto fileClose = deskflow::finally([fp]() { std::fclose(fp); });

  auto *cert = PEM_read_X509(fp, nullptr, nullptr, nullptr);
  if (!cert) {
    qWarning() << tr("could not load default certificate file to memory");
    return false;
  }
  auto certFree = deskflow::finally([cert]() { X509_free(cert); });

  auto *pubkey = X509_get_pubkey(cert);
  if (!pubkey) {
    qWarning() << tr("default certificate key file does not contain valid public key");
    return false;
  }
  auto pubkeyFree = deskflow::finally([pubkey]() { EVP_PKEY_free(pubkey); });

  auto type = EVP_PKEY_type(EVP_PKEY_id(pubkey));
  if (type != EVP_PKEY_RSA && type != EVP_PKEY_DSA) {
    qWarning() << tr("public key in default certificate key file is not RSA or DSA");
    return false;
  }

  auto bits = EVP_PKEY_bits(pubkey);
  if (bits < 2048) {
    // We could have small keys in old barrier installations
    qWarning() << tr("public key in default certificate key file is too small");
    return false;
  }

  return true;
}
