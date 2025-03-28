/*
 * Deskflow -- mouse and keyboard sharing utility
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

namespace SettingsKeys {
inline static const QString ActionType = QStringLiteral("type");
inline static const QString ScreenNames = QStringLiteral("typeScreenNames");
inline static const QString ScreenName = QStringLiteral("typeScreenName");
inline static const QString SwitchToScreen = QStringLiteral("switchScreenName");
inline static const QString SwitchDirection = QStringLiteral("switchInDirection");
inline static const QString LockToScreen = QStringLiteral("lockCursorToScreen");
inline static const QString ActiveOnRelease = QStringLiteral("activeOnRelease");
inline static const QString HasScreens = QStringLiteral("hasScreens");
inline static const QString RestartServer = QStringLiteral("restartServer");
}; // namespace SettingsKeys

class Action
{
  friend class ActionDialog;
  friend QTextStream &operator<<(QTextStream &outStream, const Action &action);

public:
  enum ActionType
  {
    keyDown,
    keyUp,
    keystroke,
    switchToScreen,
    switchInDirection,
    lockCursorToScreen,
    restartAllConnections,
    mouseDown,
    mouseUp,
    mousebutton,
  };
  enum SwitchDirection
  {
    switchLeft,
    switchRight,
    switchUp,
    switchDown
  };
  enum LockCursorMode
  {
    lockCursorToggle,
    lockCursonOn,
    lockCursorOff
  };

public:
  Action() = default;

public:
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

  bool operator==(const Action &a) const;

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
  int m_type = keystroke;
  QStringList m_typeScreenNames = QStringList();
  QString m_switchScreenName = QString();
  int m_switchDirection = switchLeft;
  int m_lockCursorMode = lockCursorToggle;
  bool m_activeOnRelease = false;
  bool m_hasScreens = false;
  bool m_restartServer;

  inline static const QString m_commandTemplate = QStringLiteral("(%1)");
  inline static const QStringList m_actionTypeNames{
      QStringLiteral("keyDown"),           QStringLiteral("keyUp"),
      QStringLiteral("keystroke"),         QStringLiteral("switchToScreen"),
      QStringLiteral("switchInDirection"), QStringLiteral("lockCursorToScreen"),
      QStringLiteral("restartServer"),     QStringLiteral("mouseDown"),
      QStringLiteral("mouseUp"),           QStringLiteral("mousebutton")
  };

  inline static const QStringList m_switchDirectionNames{
      QStringLiteral("left"), QStringLiteral("right"), QStringLiteral("up"), QStringLiteral("down")
  };

  inline static const QStringList m_lockCursorModeNames{
      QStringLiteral("toggle"), QStringLiteral("on"), QStringLiteral("off")
  };
};

using ActionList = QList<Action>;

QTextStream &operator<<(QTextStream &outStream, const Action &action);
