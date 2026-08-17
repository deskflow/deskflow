/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Action.h"

#include <QSettings>
#include <QTextStream>

QString Action::text() const
{
  auto text = QString(m_actionTypeNames.at(type()));

  switch (static_cast<Action::Type>(type())) {
    using enum Type;
  case keyDown:
  case keyUp:
  case keystroke: {
    QString commandArgs = keySequence().toString();

    if (!keySequence().isMouseButton()) {
      const QStringList &screens = typeScreenNames();
      if (haveScreens() && !screens.isEmpty()) {
        QString screenList;
        for (int i = 0; i < screens.size(); i++) {
          screenList.append(screens[i]);
          if (i != screens.size() - 1)
            screenList.append(QStringLiteral(":"));
        }
        commandArgs.append(QStringLiteral(",%1").arg(screenList));
      } else
        commandArgs.append(QStringLiteral(",*"));
    }
    text.append(m_commandTemplate.arg(commandArgs));
  } break;

  case Type::switchToScreen:
    text.append(m_commandTemplate.arg(m_switchScreenName));
    break;

  case Type::switchInDirection:
    text.append(m_commandTemplate.arg(m_switchDirectionNames.at(m_switchDirection)));
    break;

  case Type::lockCursorToScreen:
    text.append(m_commandTemplate.arg(m_lockCursorModeNames.at(m_lockCursorMode)));
    break;

  default:
    break;
  }

  return text;
}

const KeySequence &Action::keySequence() const
{
  return m_keySequence;
}

void Action::loadSettings(QSettings &settings)
{
  keySequence().loadSettings(settings);
  setType(settings.value(SettingsKeys::ActionType, static_cast<int>(Type::keyDown)).toInt());

  typeScreenNames().clear();
  int numTypeScreens = settings.beginReadArray(SettingsKeys::ScreenNames);
  for (int i = 0; i < numTypeScreens; i++) {
    settings.setArrayIndex(i);
    typeScreenNames().append(settings.value(SettingsKeys::ScreenName).toString());
  }
  settings.endArray();

  setSwitchScreenName(settings.value(SettingsKeys::SwitchToScreen).toString());
  setSwitchDirection(settings.value(SettingsKeys::SwitchDirection, static_cast<int>(SwitchDirection::left)).toInt());
  setLockCursorMode(settings.value(SettingsKeys::LockToScreen, static_cast<int>(LockCursorMode::toggle)).toInt());
  setActiveOnRelease(settings.value(SettingsKeys::ActiveOnRelease, false).toBool());
  setHaveScreens(settings.value(SettingsKeys::HasScreens, false).toBool());
  setRestartServer(settings.value(SettingsKeys::RestartServer, false).toBool());
}

void Action::saveSettings(QSettings &settings) const
{
  keySequence().saveSettings(settings);
  settings.setValue(SettingsKeys::ActionType, type());

  settings.beginWriteArray(SettingsKeys::ScreenNames);
  for (int i = 0; i < typeScreenNames().size(); i++) {
    settings.setArrayIndex(i);
    settings.setValue(SettingsKeys::ScreenName, typeScreenNames()[i]);
  }
  settings.endArray();

  settings.setValue(SettingsKeys::SwitchToScreen, switchScreenName());
  settings.setValue(SettingsKeys::SwitchDirection, switchDirection());
  settings.setValue(SettingsKeys::LockToScreen, lockCursorMode());
  settings.setValue(SettingsKeys::ActiveOnRelease, activeOnRelease());
  settings.setValue(SettingsKeys::HasScreens, haveScreens());
  settings.setValue(SettingsKeys::RestartServer, restartServer());
}

int Action::type() const
{
  return m_type;
}

const QStringList &Action::typeScreenNames() const
{
  return m_typeScreenNames;
}

const QString &Action::switchScreenName() const
{
  return m_switchScreenName;
}

int Action::switchDirection() const
{
  return m_switchDirection;
}

int Action::lockCursorMode() const
{
  return m_lockCursorMode;
}

bool Action::activeOnRelease() const
{
  return m_activeOnRelease;
}

bool Action::haveScreens() const
{
  return m_hasScreens;
}

bool Action::restartServer() const
{
  return m_restartServer;
}

KeySequence &Action::keySequence()
{
  return m_keySequence;
}

void Action::setKeySequence(const KeySequence &seq)
{
  m_keySequence = seq;
}

void Action::setType(int t)
{
  m_type = t;
}

QStringList &Action::typeScreenNames()
{
  return m_typeScreenNames;
}

void Action::setSwitchScreenName(const QString &n)
{
  m_switchScreenName = n;
}

void Action::setSwitchDirection(int d)
{
  m_switchDirection = d;
}

void Action::setLockCursorMode(int m)
{
  m_lockCursorMode = m;
}

void Action::setActiveOnRelease(bool b)
{
  m_activeOnRelease = b;
}

void Action::setHaveScreens(bool b)
{
  m_hasScreens = b;
}

void Action::setRestartServer(bool b)
{
  m_restartServer = b;
}

QTextStream &operator<<(QTextStream &outStream, const Action &action)
{
  if (action.activeOnRelease())
    outStream << ";";

  outStream << action.text();

  return outStream;
}
