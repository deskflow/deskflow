/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
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
  struct ActionTypes
  {
    inline static const auto PressKey = 0;
    inline static const auto ReleaseKey = 1;
    inline static const auto ToggleKey = 2;
    inline static const auto SwitchTo = 3;
    inline static const auto SwitchInDirection = 4;
    inline static const auto SwitchToNextScreen = 5;
    inline static const auto ModifyCursorLock = 6;
    inline static const auto RestartServer = 7;
  };

  ActionDialog(QWidget *parent, const ServerConfig &config, Hotkey &hotkey, Action &action);
  ~ActionDialog() override;

protected Q_SLOTS:
  void accept() override;

private:
  void updateSize();
  void keySequenceChanged();
  void itemToggled() const;
  void actionTypeChanged(int index);
  bool isKeyAction(int index) const;
  bool canSave() const;

  std::unique_ptr<Ui::ActionDialog> ui;
  Hotkey &m_hotkey;
  Action &m_action;
};
