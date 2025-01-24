/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Action.h"

#include <QSettings>
#include <QTextStream>

QString Action::text() const
{
  QString text = QString(m_actionTypeNames.at(keySequence().isMouseButton() ? type() + 6 : type()));

  switch (type()) {
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

  case switchToScreen:
    text.append(m_commandTemplate.arg(m_switchScreenName));
    break;

  case switchInDirection:
    text.append(m_commandTemplate.arg(m_switchDirectionNames.at(m_switchDirection)));
    break;

  case lockCursorToScreen:
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
  setType(settings.value(SettingsKeys::ActionType, keyDown).toInt());

  typeScreenNames().clear();
  int numTypeScreens = settings.beginReadArray(SettingsKeys::ScreenNames);
  for (int i = 0; i < numTypeScreens; i++) {
    settings.setArrayIndex(i);
    typeScreenNames().append(settings.value(SettingsKeys::ScreenName).toString());
  }
  settings.endArray();

  setSwitchScreenName(settings.value(SettingsKeys::SwitchToScreen).toString());
  setSwitchDirection(settings.value(SettingsKeys::SwitchDirection, switchLeft).toInt());
  setLockCursorMode(settings.value(SettingsKeys::LockToScreen, lockCursorToggle).toInt());
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

bool Action::operator==(const Action &a) const
{
  return m_keySequence == a.m_keySequence && m_type == a.m_type && m_typeScreenNames == a.m_typeScreenNames &&
         m_switchScreenName == a.m_switchScreenName && m_switchDirection == a.m_switchDirection &&
         m_lockCursorMode == a.m_lockCursorMode && m_activeOnRelease == a.m_activeOnRelease &&
         m_hasScreens == a.m_hasScreens && m_restartServer == a.m_restartServer;
}

QTextStream &operator<<(QTextStream &outStream, const Action &action)
{
  if (action.activeOnRelease())
    outStream << ";";

  outStream << action.text();

  return outStream;
}
