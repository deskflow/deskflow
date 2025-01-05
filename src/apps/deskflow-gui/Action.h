/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "KeySequence.h"

#include <QList>
#include <QString>
#include <QStringList>

class ActionDialog;
class QSettings;
class QTextStream;

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

  static const char *m_actionTypeNames[];
  static const char *m_switchDirectionNames[];
  static const char *m_lockCursorModeNames[];
};

using ActionList = QList<Action>;

QTextStream &operator<<(QTextStream &outStream, const Action &action);
