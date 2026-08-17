/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/KeySequence.h"

#include <QList>
#include <QString>
#include <QStringList>

class ActionDialog;
class QSettings;
class QTextStream;

struct SettingsKeys
{
  inline static const QString ActionType = QStringLiteral("type");
  inline static const QString ScreenNames = QStringLiteral("typeScreenNames");
  inline static const QString ScreenName = QStringLiteral("typeScreenName");
  inline static const QString SwitchToScreen = QStringLiteral("switchScreenName");
  inline static const QString SwitchDirection = QStringLiteral("switchInDirection");
  inline static const QString LockToScreen = QStringLiteral("lockCursorToScreen");
  inline static const QString ActiveOnRelease = QStringLiteral("activeOnRelease");
  inline static const QString HasScreens = QStringLiteral("hasScreens");
  inline static const QString RestartServer = QStringLiteral("restartServer");
};

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
  const KeySequence &keySequence() const;
  void loadSettings(QSettings &settings);
  void saveSettings(QSettings &settings) const;
  int type() const;
  const QStringList &typeScreenNames() const;
  const QString &switchScreenName() const;
  int switchDirection() const;
  int lockCursorMode() const;
  bool activeOnRelease() const;
  bool haveScreens() const;
  bool restartServer() const;

  bool operator==(const Action &a) const = default;

protected:
  KeySequence &keySequence();
  void setKeySequence(const KeySequence &seq);
  void setType(int t);
  QStringList &typeScreenNames();
  void setSwitchScreenName(const QString &n);
  void setSwitchDirection(int d);
  void setLockCursorMode(int m);
  void setActiveOnRelease(bool b);
  void setHaveScreens(bool b);
  void setRestartServer(bool b);

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

  inline static const QString m_commandTemplate = QStringLiteral("(%1)");
  inline static const QStringList m_actionTypeNames{
      QStringLiteral("keyDown"),
      QStringLiteral("keyUp"),
      QStringLiteral("keystroke"),
      QStringLiteral("switchToScreen"),
      QStringLiteral("switchInDirection"),
      QStringLiteral("switchToNextScreen"),
      QStringLiteral("lockCursorToScreen"),
      QStringLiteral("restartServer"),
      QStringLiteral("mouseDown"),
      QStringLiteral("mouseUp"),
      QStringLiteral("mousebutton")
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
