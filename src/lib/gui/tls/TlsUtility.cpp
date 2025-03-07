/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "TlsUtility.h"

#include "TlsCertificate.h"
#include "common/DeskflowSettings.h"
#include <QFile>
#include <QString>

namespace deskflow::gui {

TlsUtility::TlsUtility(QObject *parent) : QObject(parent)
{
}

bool TlsUtility::isEnabled() const
{
  return DeskflowSettings::value(Settings::Security::TlsEnabled).toBool();
}

bool TlsUtility::generateCertificate()
{
  qDebug("generating tls certificate, "
         "all clients must trust the new fingerprint");

  if (!isEnabled()) {
    qCritical("unable to generate tls certificate, "
              "tls is either not available or not enabled");
    return false;
  }

  auto length = DeskflowSettings::value(Settings::Security::KeySize).toInt();
  const auto certificate = DeskflowSettings::value(Settings::Security::Certificate).toString();

  return m_certificate.generateCertificate(certificate, length);
}

bool TlsUtility::persistCertificate()
{
  qDebug("persisting tls certificate");

  if (QFile::exists(DeskflowSettings::value(Settings::Security::Certificate).toString())) {
    qDebug("tls certificate already exists");
    return true;
  }

  return generateCertificate();
}

} // namespace deskflow::gui
