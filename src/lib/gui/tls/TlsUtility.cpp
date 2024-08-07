/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
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

#include "TlsUtility.h"

#include "TlsCertificate.h"
#include "constants.h"

#include <QString>

namespace synergy::gui {

TlsUtility::TlsUtility(
    const IAppConfig &appConfig, const license::License &license)
    : m_appConfig(appConfig),
      m_license(license) {}

bool TlsUtility::isAvailable() const {
  return !kEnableActivation || m_license.isTlsAvailable();
}

bool TlsUtility::isAvailableAndEnabled() const {
  const auto &config = m_appConfig;
  return isAvailable() && config.tlsEnabled();
}

bool TlsUtility::generateCertificate() {
  qDebug("generating tls certificate, "
         "all clients must trust the new fingerprint");

  if (!isAvailableAndEnabled()) {
    qCritical("unable to generate tls certificate, "
              "tls is either not available or not enabled");
    return false;
  }

  auto path = m_appConfig.tlsCertPath();
  auto length = m_appConfig.tlsKeyLength();

  return m_certificate.generateCertificate(path, length);
}

} // namespace synergy::gui
