/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "TlsCertificate.h"
#include <common/Settings.h>

#include <QObject>

namespace deskflow::gui {

class TlsUtility : public QObject
{
  Q_OBJECT

public:
  explicit TlsUtility(QObject *parent = nullptr);

  bool generateCertificate() const;

  /**
   * @brief Checks the settings values Settings::Security::TlsEnabled
   * @return true when tls is enabled
   */
  static bool isEnabled();

  /**
   * @brief isCertValid
   * @param certPath the path of the file to check, when not set uses Settings::Security::Certificate value
   * @return true if the certificate is valid
   */
  static bool isCertValid(const QString &certPath = Settings::value(Settings::Security::Certificate).toString());

private:
  TlsCertificate m_certificate;
};

} // namespace deskflow::gui
