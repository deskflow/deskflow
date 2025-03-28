/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QDialog>

class Hotkey;
class Action;
class ServerConfig;
class KeySequenceWidget;

namespace Ui {
class ActionDialog;
}

class ActionDialog : public QDialog
{
  Q_OBJECT

public:
  enum ActionTypes
  {
    PressKey,
    ReleaseKey,
    ToggleKey,
    SwitchTo,
    SwitchInDirection,
    ModifyCursorLock,
    RestartServer
  };

  ActionDialog(QWidget *parent, const ServerConfig &config, Hotkey &hotkey, Action &action);
  ~ActionDialog() override;

protected slots:
  void accept() override;

private:
  void updateSize();
  void keySequenceChanged();
  void actionTypeChanged(int index);
  bool isKeyAction(int index);
  bool canSave();

  std::unique_ptr<Ui::ActionDialog> ui;
  Hotkey &m_hotkey;
  Action &m_action;
};
