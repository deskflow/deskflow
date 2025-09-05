/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "ScreenSetupModel.h"
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
  void listHotkeysSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected [[maybe_unused]]);

  void addAction();
  void editAction();
  void removeAction();
  void listActionsSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected [[maybe_unused]]);

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
  std::unique_ptr<Ui::ServerConfigDialog> ui;
  QString m_message = "";
  ServerConfig &m_originalServerConfig;
  bool m_originalServerConfigIsExternal;
  QString m_originalServerConfigUsesExternalFile;
  ScreenSetupModel m_screenSetupModel;
  ServerConfig m_serverConfig;

private Q_SLOTS:
  void onChange();
};
