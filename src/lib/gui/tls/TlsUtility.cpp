/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "TlsUtility.h"

#include "TlsCertificate.h"

#include <QFile>
#include <QString>

namespace deskflow::gui {

TlsUtility::TlsUtility(const IAppConfig &appConfig) : m_appConfig(appConfig)
{
}

bool TlsUtility::isEnabled() const
{
  const auto &config = m_appConfig;
  return config.tlsEnabled();
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

  auto length = m_appConfig.tlsKeyLength();

  return m_certificate.generateCertificate(m_appConfig.tlsCertPath(), length);
}

bool TlsUtility::persistCertificate()
{
  qDebug("persisting tls certificate");

  if (QFile::exists(m_appConfig.tlsCertPath())) {
    qDebug("tls certificate already exists");
    return true;
  }

  return generateCertificate();
}

} // namespace deskflow::gui
