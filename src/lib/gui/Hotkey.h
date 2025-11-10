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

using namespace Qt::StringLiterals;

class HotkeyDialog;
class ServerConfigDialog;
class QSettings;

class Hotkey
{
  friend class HotkeyDialog;
  friend class ServerConfigDialog;
  friend QTextStream &operator<<(QTextStream &outStream, const Hotkey &hotkey);

public:
  Hotkey() = default;

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
  KeySequence m_keySequence = {};
  ActionList m_actions = {};
  inline static const QString kSectionActions = u"actions"_s;
  inline static const QString kMousebutton = u"mousebutton(%1)"_s;
  inline static const QString kKeystroke = u"keystroke(%1)"_s;
};

using HotkeyList = QList<Hotkey>;

QTextStream &operator<<(QTextStream &outStream, const Hotkey &hotkey);
