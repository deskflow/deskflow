/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
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

QTextStream &operator<<(QTextStream &outStream, const Action &action)
{
  if (action.activeOnRelease())
    outStream << ";";

  outStream << action.text();

  return outStream;
}
