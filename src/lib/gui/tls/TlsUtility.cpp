/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "TlsUtility.h"

#include "TlsCertificate.h"
#include "common/Settings.h"
#include <QFile>
#include <QString>

namespace deskflow::gui {

TlsUtility::TlsUtility(QObject *parent) : QObject(parent)
{
  // do nothing
}

bool TlsUtility::isEnabled() const
{
  return Settings::value(Settings::Security::TlsEnabled).toBool();
}

bool TlsUtility::generateCertificate() const
{
  qDebug(
      "generating tls certificate, "
      "all clients must trust the new fingerprint"
  );

  if (!isEnabled()) {
    qCritical(
        "unable to generate tls certificate, "
        "tls is either not available or not enabled"
    );
    return false;
  }

  auto length = Settings::value(Settings::Security::KeySize).toInt();

  if (length < 2048) {
    length = 2048;
    qDebug("selected size too small setting certificate size to 2048");
    Settings::setValue(Settings::Security::KeySize, 2048);
  }

  const auto certificate = Settings::value(Settings::Security::Certificate).toString();
  return m_certificate.generateCertificate(certificate, length);
}

bool TlsUtility::persistCertificate() const
{
  qDebug("persisting tls certificate");

  if (QFile::exists(Settings::value(Settings::Security::Certificate).toString())) {
    qDebug("tls certificate already exists");
    return true;
  }

  return generateCertificate();
}

} // namespace deskflow::gui
