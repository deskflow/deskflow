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

#include "Action.h"

#include <QSettings>
#include <QTextStream>

const char *Action::m_actionTypeNames[] = {
    "keyDown",       "keyUp",     "keystroke", "switchToScreen", "switchInDirection", "lockCursorToScreen",
    "restartServer", "mouseDown", "mouseUp",   "mousebutton"
};

const char *Action::m_switchDirectionNames[] = {"left", "right", "up", "down"};
const char *Action::m_lockCursorModeNames[] = {"toggle", "on", "off"};

QString Action::text() const
{
  QString text = QString(m_actionTypeNames[keySequence().isMouseButton() ? type() + 6 : type()]) + "(";

  switch (type()) {
  case keyDown:
  case keyUp:
  case keystroke: {
    text += keySequence().toString();

    if (!keySequence().isMouseButton()) {
      const QStringList &screens = typeScreenNames();
      if (haveScreens() && !screens.isEmpty()) {
        text += ",";

        for (int i = 0; i < screens.size(); i++) {
          text += screens[i];
          if (i != screens.size() - 1)
            text += ":";
        }
      } else
        text += ",*";
    }
  } break;

  case switchToScreen:
    text += switchScreenName();
    break;

  case switchInDirection:
    text += m_switchDirectionNames[m_switchDirection];
    break;

  case lockCursorToScreen:
    text += m_lockCursorModeNames[m_lockCursorMode];
    break;

  case restartAllConnections:
    text += "restart";
    break;
  default:
    Q_ASSERT(0);
    break;
  }

  text += ")";

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
