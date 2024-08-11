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

#pragma once

#include "gui/config/IAppConfig.h"

#include "TlsCertificate.h"
#include "license/ILicense.h"

#include <QObject>

namespace synergy::gui {

class TlsUtility : public QObject {
  using ILicense = synergy::license::ILicense;
  Q_OBJECT

public:
  explicit TlsUtility(const IAppConfig &appConfig, const ILicense &license);

  bool generateCertificate();
  bool persistCertificate();

  /**
   * @brief Combines the availability and the enabled status of TLS.
   *
   * @return Given that the app setting for TLS is enabled:
   * If licensing is enabled, it checks whether the product has TLS
   * available, and if licensing is not enabled, true is returned.
   */
  bool isAvailableAndEnabled() const;

  /**
   * @return true if TLS is available, regardless of whether it is enabled.
   */
  bool isAvailable() const;

private:
  const IAppConfig &m_appConfig;
  const ILicense &m_license;
  TlsCertificate m_certificate;
};

} // namespace synergy::gui
