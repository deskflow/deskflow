/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QList>
#include <QString>
#include <QTextStream>

#include "common/Action.h"
#include "common/KeySequence.h"

class HotkeyDialog;
class ServerConfigDialog;
class QSettings;

class Hotkey
{
public:
  Hotkey() = default;

  QString text() const;
  const KeySequence &keySequence() const
  {
    return m_keySequence;
  }
  void setKeySequence(const KeySequence &seq)
  {
    m_keySequence = seq;
  }
  //! When false the key still reaches apps on the server, see \c disableGlobalHotkeyRegister
  bool registerWithOS() const
  {
    return m_registerWithOS;
  }
  void setRegisterWithOS(bool registerWithOS)
  {
    m_registerWithOS = registerWithOS;
  }
  const ActionList &actions() const
  {
    return m_actions;
  }

  Action &actionAt(int index);
  void addAction(const Action &action);
  void removeActionAt(int index);

  void loadSettings(QSettings &settings);
  void saveSettings(QSettings &settings) const;

  bool operator==(const Hotkey &hk) const;

private:
  KeySequence m_keySequence = {};
  ActionList m_actions = {};
  bool m_registerWithOS = true;
  inline static const QString kSectionActions = QStringLiteral("actions");
  inline static const QString kDisableGlobalHotkeyRegister = QStringLiteral("disableGlobalHotkeyRegister");
  inline static const QString kMousebutton = QStringLiteral("mousebutton(%1)");
  inline static const QString kKeystroke = QStringLiteral("keystroke(%1)");
  inline static const QString kKeystrokeNoRegister = QStringLiteral("keystroke(%1,disableGlobalHotkeyRegister)");

  friend QTextStream &operator<<(QTextStream &outStream, const Hotkey &hotkey);
};

using HotkeyList = QList<Hotkey>;

QTextStream &operator<<(QTextStream &outStream, const Hotkey &hotkey);
