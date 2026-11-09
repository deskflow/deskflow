/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once
#include <QDialog>
#include <memory>

#include "gui/config/ServerConfig.h"

class SettingsDialogButtonBox;
class QLabel;
class QListWidget;
class QToolButton;
class QHBoxLayout;
class QPushButton;
namespace deskflow::gui::startup {
class Reader;
}

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
  Q_OBJECT

public:
  void extracted();
  SettingsDialog(
      QWidget *parent, const ServerConfig &serverConfig,
      std::shared_ptr<deskflow::gui::startup::Reader> startupReader = {}
  );
  ~SettingsDialog() override;

Q_SIGNALS:
  void requestRemoveAllSettings();
  void requestStartupQuit();
  void requestConnectionSetup();

protected:
  void changeEvent(QEvent *e) override;

private:
  QLabel *m_identityStatus = nullptr;
  QHBoxLayout *m_identityAction = nullptr;
  QLabel *m_identityHeading = nullptr;
  QLabel *m_pageTitle = nullptr;
  QLabel *m_pageDescription = nullptr;
  QListWidget *m_navigation = nullptr;
  QToolButton *m_connectionDetails = nullptr;
  QWidget *m_connectionDetailsBody = nullptr;
  bool m_styling = false;
  QPushButton *m_startupRecovery = nullptr;
  std::shared_ptr<deskflow::gui::startup::Reader> m_startupReader;
  void setupPresentation();
  void updatePageHeading();
  void updateAppearance();
  void initConnections() const;
  void regenCertificates();
  void browseCertificatePath();
  void browseLogPath();
  void setLogToFile(bool logToFile);
  void accept() override;
  bool isClientMode() const;
  void updateTlsControls();
  void updateTlsControlsEnabled();
  void resetAllSettings();
  void updateText();

  /// @brief Load all settings.
  void loadFromConfig();

  /// @brief Updates the key length value based on the loaded file.
  void updateKeyLengthOnFile(const QString &path);

  /// @brief Enables controls when they should be.
  void updateControls();
  void enforceReviewBoundary();

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

  bool m_interfaceSetOnLoad = false;
  std::unique_ptr<Ui::SettingsDialog> ui;
  const ServerConfig &m_serverConfig;
  SettingsDialogButtonBox *m_buttonBox = nullptr;
};
