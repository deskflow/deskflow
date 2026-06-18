/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "ScreenSetupModel.h"
#include "common/NetworkProtocol.h"
#include "config/ServerConfig.h"

#include <QDialog>

class QItemSelection;

namespace Ui {
class ServerConfigDialog;
}

class ServerConfigDialog : public QDialog
{
  Q_OBJECT

public:
  ServerConfigDialog(QWidget *parent, ServerConfig &config);
  ~ServerConfigDialog() override;
  bool addClient(const QString &clientName);

public Q_SLOTS:
  void accept() override;
  void reject() override;
  void message(const QString &message)
  {
    m_message = message;
  }

protected Q_SLOTS:
  void onScreenRemoved();

protected:
  void addClient();
  bool addComputer(const QString &clientName, bool doSilent);

  void addHotkey();
  void editHotkey();
  void removeHotkey();
  void listHotkeysSelectionChanged(const QItemSelection &selected, [[maybe_unused]] const QItemSelection &deselected);

  void addAction();
  void editAction();
  void removeAction();
  void listActionsSelectionChanged(const QItemSelection &selected, [[maybe_unused]] const QItemSelection &deselected);

  void toggleSwitchDoubleTap(bool enable);
  void setSwitchDoubleTap(int within);

  void toggleSwitchDelay(bool enable);
  void setSwitchDelay(int delay);

  void toggleDefaultLockToComputerState(bool state);
  void toggleLockToComputer(bool disabled);
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
    return m_serverConfig;
  }
  void setOriginalServerConfig(const ServerConfig &s)
  {
    m_originalServerConfig = s;
  }
  ScreenSetupModel &model()
  {
    return m_screenSetupModel;
  }

private:
  void loadFromConfig();
  void initConnections() const;
  std::unique_ptr<Ui::ServerConfigDialog> ui;
  QString m_message = "";
  int m_columns;
  int m_rows;
  ServerConfig &m_originalServerConfig;
  NetworkProtocol m_protocol;
  bool m_enableClipboard;
  bool m_enableHeartbeat;
  int m_heartbeatRate;
  int m_switchDelay;
  int m_switchDoubleTap;
  uint m_clipboardSize;
  bool m_relativeMouseMoves;
  bool m_enableSwitchDelay;
  bool m_enableSwitchDoubleTap;
  bool m_originalServerConfigIsExternal;
  bool m_win32keepForeground;
  bool m_disableLockToComputer;
  bool m_defaultLockToComputerState;
  QString m_originalServerConfigUsesExternalFile;
  ServerConfig m_serverConfig;
  ScreenSetupModel m_screenSetupModel;

private Q_SLOTS:
  void onChange();
};
