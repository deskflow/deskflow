/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "ScreenSetupModel.h"
#include "ServerConfig.h"
#include "gui/config/AppConfig.h"

#include <QDialog>

class QItemSelection;

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
  void onScreenRemoved();

protected:
  void addClient();
  bool addComputer(const QString &clientName, bool doSilent);

  void addHotkey();
  void editHotkey();
  void removeHotkey();
  void listHotkeysSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

  void addAction();
  void editAction();
  void removeAction();
  void listActionsSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

  void toggleSwitchDoubleTap(bool enable);
  void setSwitchDoubleTap(int within);

  void toggleSwitchDelay(bool enable);
  void setSwitchDelay(int delay);

  void toggleLockToScreen(bool disabled);
  void toggleWin32Foreground(bool enabled);

  void toggleClipboard(bool enabled);
  void setClipboardLimit(int limit);

  void toggleHeartbeat(bool enabled);
  void setHeartbeat(int rate);

  void toggleRelativeMouseMoves(bool enabled);
  void toggleProtocol();

  void setSwitchCornerSize(int size);
  void toggleCornerBottomLeft(bool enable);
  void toggleCornerTopLeft(bool enable);
  void toggleCornerBottomRight(bool enable);
  void toggleCornerTopRight(bool enable);

  void toggleExternalConfig(bool enable = false);
  bool browseConfigFile();

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
