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
      const QStringList &computers = typeComputerNames();
      if (haveComputers() && !computers.isEmpty()) {
        QString computerList;
        for (int i = 0; i < computers.size(); i++) {
          computerList.append(computers[i]);
          if (i != computers.size() - 1)
            computerList.append(QStringLiteral(":"));
        }
        commandArgs.append(QStringLiteral(",%1").arg(computerList));
      } else
        commandArgs.append(QStringLiteral(",*"));
    }
    text.append(m_commandTemplate.arg(commandArgs));
  } break;

  case Type::switchToComputer:
    text.append(m_commandTemplate.arg(m_switchComputerName));
    break;

  case Type::switchInDirection:
    text.append(m_commandTemplate.arg(m_switchDirectionNames.at(m_switchDirection)));
    break;

  case Type::lockCursorToComputer:
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
  m_keySequence.loadSettings(settings);
  setType(settings.value(SettingsKeys::ActionType, static_cast<int>(Type::keyDown)).toInt());

  m_typeComputerNames.clear();
  int numTypeComputers = settings.beginReadArray(SettingsKeys::ComputerNames);
  for (int i = 0; i < numTypeComputers; i++) {
    settings.setArrayIndex(i);
    m_typeComputerNames.append(settings.value(SettingsKeys::ComputerName).toString());
  }
  settings.endArray();

  setSwitchComputerName(settings.value(SettingsKeys::SwitchToComputer).toString());
  setSwitchDirection(settings.value(SettingsKeys::SwitchDirection, static_cast<int>(SwitchDirection::left)).toInt());
  setLockCursorMode(settings.value(SettingsKeys::LockToComputer, static_cast<int>(LockCursorMode::toggle)).toInt());
  setActiveOnRelease(settings.value(SettingsKeys::ActiveOnRelease, false).toBool());
  setHaveComputers(settings.value(SettingsKeys::HasComputers, false).toBool());
  setRestartServer(settings.value(SettingsKeys::RestartServer, false).toBool());
}

void Action::saveSettings(QSettings &settings) const
{
  keySequence().saveSettings(settings);
  settings.setValue(SettingsKeys::ActionType, type());

  settings.beginWriteArray(SettingsKeys::ComputerNames);
  for (int i = 0; i < m_typeComputerNames.size(); i++) {
    settings.setArrayIndex(i);
    settings.setValue(SettingsKeys::ComputerName, m_typeComputerNames[i]);
  }
  settings.endArray();

  settings.setValue(SettingsKeys::SwitchToComputer, m_switchComputerName);
  settings.setValue(SettingsKeys::SwitchDirection, m_switchDirection);
  settings.setValue(SettingsKeys::LockToComputer, m_lockCursorMode);
  settings.setValue(SettingsKeys::ActiveOnRelease, m_activeOnRelease);
  settings.setValue(SettingsKeys::HasComputers, m_hasComputers);
  settings.setValue(SettingsKeys::RestartServer, m_restartServer);
}

int Action::type() const
{
  return m_type;
}

QStringList Action::typeComputerNames() const
{
  return m_typeComputerNames;
}

void Action::clearComputers()
{
  m_typeComputerNames.clear();
}

void Action::addComputer(const QString &computer)
{
  if (m_typeComputerNames.contains(computer))
    return;
  m_typeComputerNames.append(computer);
}

void Action::removeComputer(const QString &computer)
{
  if (!m_typeComputerNames.contains(computer))
    return;
  m_typeComputerNames.removeAll(computer);
}

const QString &Action::switchComputerName() const
{
  return m_switchComputerName;
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

bool Action::haveComputers() const
{
  return m_hasComputers;
}

bool Action::restartServer() const
{
  return m_restartServer;
}

void Action::setKeySequence(const KeySequence &seq)
{
  m_keySequence = seq;
}

void Action::setType(int t)
{
  m_type = t;
}

void Action::setSwitchComputerName(const QString &n)
{
  m_switchComputerName = n;
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

void Action::setHaveComputers(bool b)
{
  m_hasComputers = b;
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
