/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <common/Settings.h>

#include <QObject>

namespace deskflow::gui {

class TlsUtility : public QObject
{
  Q_OBJECT

public:
  explicit TlsUtility(QObject *parent = nullptr);

  static bool generateCertificate();

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

  /**
   * @brief Get the lenght of a key
   * @param certPath path of the file to check, when unset will use the value of Settings::Security::Certificate
   * @return the bitsize of the key
   */
  static int getCertKeyLength(const QString &certPath = Settings::value(Settings::Security::Certificate).toString());

  /**
   * @brief get the SHA256 fingerprint of a certificatefile.
   * @param certPath path of the file to fingerprint, when not set uses the vaule of Settings::Security::Certificate
   * @return A QByteArray of the SHA256 fingerprint of the file or empty array if invalid file is passed
   */
  // clang-format off
  static QByteArray certFingerprint(const QString &certPath = Settings::value(Settings::Security::Certificate).toString());
  // clang-format on
};

} // namespace deskflow::gui
