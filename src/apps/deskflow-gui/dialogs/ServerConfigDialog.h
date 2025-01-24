/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "ScreenSetupModel.h"
#include "ServerConfig.h"
#include "gui/config/AppConfig.h"

#include <QDialog>

namespace Ui {
class ServerConfigDialog;
}

class ServerConfigDialog : public QDialog
{
  Q_OBJECT

public:
  ServerConfigDialog(QWidget *parent, ServerConfig &config, AppConfig &appConfig);
  ~ServerConfigDialog();
  bool addClient(const QString &clientName);

public slots:
  void accept() override;
  void reject() override;
  void message(const QString &message)
  {
    m_Message = message;
  }

protected slots:
  void on_m_pButtonNewHotkey_clicked();
  void on_m_pListHotkeys_itemSelectionChanged();
  void on_m_pButtonEditHotkey_clicked();
  void on_m_pButtonRemoveHotkey_clicked();

  void on_m_pButtonNewAction_clicked();
  void on_m_pListActions_itemSelectionChanged();
  void on_m_pButtonEditAction_clicked();
  void on_m_pButtonRemoveAction_clicked();
  void on_m_pCheckBoxEnableClipboard_stateChanged(int state);
  void on_m_pButtonAddComputer_clicked();
  void onScreenRemoved();
  void on_m_pCheckBoxUseExternalConfig_toggled(bool checked = false);
  bool on_m_pButtonBrowseConfigFile_clicked();

protected:
  bool addComputer(const QString &clientName, bool doSilent);
  ServerConfig &serverConfig()
  {
    return m_ServerConfig;
  }
  void setOriginalServerConfig(const ServerConfig &s)
  {
    m_OriginalServerConfig = s;
  }
  ScreenSetupModel &model()
  {
    return m_ScreenSetupModel;
  }
  AppConfig &appConfig()
  {
    return m_appConfig;
  }

private:
  std::unique_ptr<Ui::ServerConfigDialog> ui;
  ServerConfig &m_OriginalServerConfig;
  ServerConfig m_ServerConfig;
  bool m_OriginalServerConfigIsExternal;
  QString m_OriginalServerConfigUsesExternalFile;
  ScreenSetupModel m_ScreenSetupModel;
  QString m_Message;
  AppConfig &m_appConfig;

private slots:
  void onChange();
};
