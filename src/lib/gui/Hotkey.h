/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QList>
#include <QString>
#include <QTextStream>

#include "Action.h"
#include "KeySequence.h"

class HotkeyDialog;
class ServerConfigDialog;
class QSettings;

class Hotkey
{
  friend class HotkeyDialog;
  friend class ServerConfigDialog;
  friend QTextStream &operator<<(QTextStream &outStream, const Hotkey &hotkey);

public:
  Hotkey();

public:
  QString text() const;
  const KeySequence &keySequence() const
  {
    return m_keySequence;
  }
  const ActionList &actions() const
  {
    return m_actions;
  }

  void loadSettings(QSettings &settings);
  void saveSettings(QSettings &settings) const;

  bool operator==(const Hotkey &hk) const;

protected:
  KeySequence &keySequence()
  {
    return m_keySequence;
  }
  void setKeySequence(const KeySequence &seq)
  {
    m_keySequence = seq;
  }
  ActionList &actions()
  {
    return m_actions;
  }

private:
  KeySequence m_keySequence;
  ActionList m_actions;
  inline static const QString kSectionActions = QStringLiteral("actions");
  inline static const QString kMousebutton = QStringLiteral("mousebutton(%1)");
  inline static const QString kKeystroke = QStringLiteral("keystroke(%1)");
};

using HotkeyList = QList<Hotkey>;

QTextStream &operator<<(QTextStream &outStream, const Hotkey &hotkey);
