/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once
#include <QDialog>

#include "gui/config/IServerConfig.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
  using IServerConfig = deskflow::gui::IServerConfig;

  Q_OBJECT

public:
  void extracted();
  SettingsDialog(QWidget *parent, const IServerConfig &serverConfig);
  ~SettingsDialog() override;

Q_SIGNALS:
  void shown();

protected:
  void changeEvent(QEvent *e) override;

private:
  void initConnections() const;
  void regenCertificates();
  void browseCertificatePath();
  void browseLogPath();
  void setLogToFile(bool logToFile);
  void accept() override;
  void showEvent(QShowEvent *event) override;
  bool isClientMode() const;
  void updateTlsControls();
  void updateTlsControlsEnabled();
  void showReadOnlyMessage();
  void updateText();

  /// @brief Load all settings.
  void loadFromConfig();

  /// @brief Updates the key length value based on the loaded file.
  void updateKeyLengthOnFile(const QString &path);

  /// @brief Enables controls when they should be.
  void updateControls();

  /// @brief updates the setting vaule for key size.
  void updateRequestedKeySize() const;

  /// @brief update if the log level warning is shown
  void logLevelChanged();

  /**
   * @brief isModified
   * @return true when any client settings in the gui do not match the stored settings values.
   */
  bool isModified() const;

  /**
   * @brief isDefault
   * @return true if all client settings match the default values
   */
  bool isDefault() const;

  /**
   * @brief Set the gui values to the defalut values for all client settings
   */
  void resetToDefault();

  /**
   * @brief setButtonBoxEnabledButtons
   * Enable / Disable the button box buttons based on the state of the gui
   */
  void setButtonBoxEnabledButtons() const;

  std::unique_ptr<Ui::SettingsDialog> ui;
  const IServerConfig &m_serverConfig;
};
