/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once
#include <QDialog>

#include "gui/config/AppConfig.h"
#include "gui/config/IServerConfig.h"
#include "gui/core/CoreProcess.h"
#include "gui/tls/TlsUtility.h"
#include "gui/validators/ValidationError.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
  using IServerConfig = deskflow::gui::IServerConfig;
  using CoreProcess = deskflow::gui::CoreProcess;

  Q_OBJECT

public:
  void extracted();
  SettingsDialog(
      QWidget *parent, AppConfig &appConfig, const IServerConfig &serverConfig, const CoreProcess &coreProcess
  );
  ~SettingsDialog() override;

signals:
  void shown();

private:
  void initConnections();
  void regenCertificates();
  void browseCertificatePath();
  void browseLogPath();
  void setLogToFile(bool logToFile);
  void setSystemScope(bool systemScope);
  void accept() override;
  void reject() override;
  void showEvent(QShowEvent *event) override;
  bool isClientMode() const;
  void updateTlsControls();
  void updateTlsControlsEnabled();
  void showReadOnlyMessage();

  /// @brief Load all settings.
  void loadFromConfig();

  /// @brief Enables or disables the TLS regenerate button.
  void updateTlsRegenerateButton();

  /// @brief Updates the key length value based on the loaded file.
  void updateKeyLengthOnFile(const QString &path);

  /// @brief Enables controls when they should be.
  void updateControls();

  /// @brief Stores settings scope at start of settings dialog
  /// This is necessary to restore state if user changes
  /// the scope and doesn't save changes
  bool m_wasOriginallySystemScope = false;

  std::unique_ptr<Ui::SettingsDialog> ui;
  AppConfig &m_appConfig;
  const IServerConfig &m_serverConfig;
  const CoreProcess &m_coreProcess;
  deskflow::gui::TlsUtility m_tlsUtility;
};
