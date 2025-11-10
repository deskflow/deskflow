/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "KeySequence.h"

#include <QList>
#include <QString>
#include <QStringList>

class ActionDialog;
class QSettings;
class QTextStream;

using namespace Qt::StringLiterals;

namespace SettingsKeys {
inline static const QString ActionType = u"type"_s;
inline static const QString ScreenNames = u"typeScreenNames"_s;
inline static const QString ScreenName = u"typeScreenName"_s;
inline static const QString SwitchToScreen = u"switchScreenName"_s;
inline static const QString SwitchDirection = u"switchInDirection"_s;
inline static const QString LockToScreen = u"lockCursorToScreen"_s;
inline static const QString ActiveOnRelease = u"activeOnRelease"_s;
inline static const QString HasScreens = u"hasScreens"_s;
inline static const QString RestartServer = u"restartServer"_s;
} // namespace SettingsKeys

class Action
{
  friend class ActionDialog;
  friend QTextStream &operator<<(QTextStream &outStream, const Action &action);

public:
  enum class Type
  {
    keyDown,
    keyUp,
    keystroke,
    switchToScreen,
    switchInDirection,
    switchToNextScreen,
    lockCursorToScreen,
    restartAllConnections,
    mouseDown,
    mouseUp,
    mousebutton,
  };
  enum class SwitchDirection
  {
    left,
    right,
    up,
    down
  };
  enum class LockCursorMode
  {
    toggle,
    on,
    off
  };

public:
  Action() = default;

  QString text() const;
  const KeySequence &keySequence() const
  {
    return m_keySequence;
  }
  void loadSettings(QSettings &settings);
  void saveSettings(QSettings &settings) const;
  int type() const
  {
    return m_type;
  }
  const QStringList &typeScreenNames() const
  {
    return m_typeScreenNames;
  }
  const QString &switchScreenName() const
  {
    return m_switchScreenName;
  }
  int switchDirection() const
  {
    return m_switchDirection;
  }
  int lockCursorMode() const
  {
    return m_lockCursorMode;
  }
  bool activeOnRelease() const
  {
    return m_activeOnRelease;
  }
  bool haveScreens() const
  {
    return m_hasScreens;
  }
  bool restartServer() const
  {
    return m_restartServer;
  }

  bool operator==(const Action &a) const = default;

protected:
  KeySequence &keySequence()
  {
    return m_keySequence;
  }
  void setKeySequence(const KeySequence &seq)
  {
    m_keySequence = seq;
  }
  void setType(int t)
  {
    m_type = t;
  }
  QStringList &typeScreenNames()
  {
    return m_typeScreenNames;
  }
  void setSwitchScreenName(const QString &n)
  {
    m_switchScreenName = n;
  }
  void setSwitchDirection(int d)
  {
    m_switchDirection = d;
  }
  void setLockCursorMode(int m)
  {
    m_lockCursorMode = m;
  }
  void setActiveOnRelease(bool b)
  {
    m_activeOnRelease = b;
  }
  void setHaveScreens(bool b)
  {
    m_hasScreens = b;
  }
  void setRestartServer(bool b)
  {
    m_restartServer = b;
  }

private:
  KeySequence m_keySequence;
  int m_type = static_cast<int>(Type::keystroke);
  QStringList m_typeScreenNames = QStringList();
  QString m_switchScreenName = QString();
  int m_switchDirection = static_cast<int>(SwitchDirection::left);
  int m_lockCursorMode = static_cast<int>(LockCursorMode::toggle);
  bool m_activeOnRelease = false;
  bool m_hasScreens = false;
  bool m_restartServer;

  inline static const QString m_commandTemplate = u"(%1)"_s;
  inline static const QStringList m_actionTypeNames{
      u"keyDown"_s,
      u"keyUp"_s,
      u"keystroke"_s,
      u"switchToScreen"_s,
      u"switchInDirection"_s,
      u"switchToNextScreen"_s,
      u"lockCursorToScreen"_s,
      u"restartServer"_s,
      u"mouseDown"_s,
      u"mouseUp"_s,
      u"mousebutton"_s
  };

  inline static const QStringList m_switchDirectionNames{u"left"_s, u"right"_s, u"up"_s, u"down"_s};

  inline static const QStringList m_lockCursorModeNames{u"toggle"_s, u"on"_s, u"off"_s};
};

using ActionList = QList<Action>;

QTextStream &operator<<(QTextStream &outStream, const Action &action);
