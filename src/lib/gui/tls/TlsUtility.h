/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "gui/config/IAppConfig.h"

#include "TlsCertificate.h"

#include <QObject>

namespace deskflow::gui {

class TlsUtility : public QObject
{
  Q_OBJECT

public:
  explicit TlsUtility(const IAppConfig &appConfig);

  bool generateCertificate();
  bool persistCertificate();

  /**
   * @brief Combines the availability and the enabled status of TLS.
   *
   * @return Given that the app setting for TLS is enabled:
   * If licensing is enabled, it checks whether the product has TLS
   * available, and if licensing is not enabled, true is returned.
   */
  bool isEnabled() const;

private:
  const IAppConfig &m_appConfig;
  TlsCertificate m_certificate;
};

} // namespace deskflow::gui
