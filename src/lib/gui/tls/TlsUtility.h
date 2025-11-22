/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "TlsCertificate.h"

#include <QObject>

namespace deskflow::gui {

class TlsUtility : public QObject
{
  Q_OBJECT

public:
  explicit TlsUtility(QObject *parent = nullptr);

  bool generateCertificate() const;
  bool persistCertificate() const;

  /**
   * @brief Checks the settings values Settings::Security::TlsEnabled
   * @return true when tls is enabled
   */
  static bool isEnabled();

private:
  TlsCertificate m_certificate;
};

} // namespace deskflow::gui
