/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Screen.h"
#include "config/ScreenConfig.h"
#include <common/Settings.h>

using enum ScreenConfig::Modifier;
using enum ScreenConfig::SwitchCorner;
using enum ScreenConfig::Fix;

Screen::Screen(const QString &name)
{
  setName(name);
}

void Screen::loadSettings(QSettingsProxy &settings)
{
  const auto name = settings.value(Settings::Layout::ScreenName).toString();
  setName(name);

  if (name.isEmpty())
    return;

  setSwitchCornerSize(settings.value(Settings::Layout::ScreenSwitchCornerSize).toInt());

  readSettings(
      settings, modifiers(), Settings::Layout::ScreenModifierArray, Settings::Layout::ScreenModifier,
      static_cast<int>(DefaultMod), static_cast<int>(NumModifiers)
  );
  readSettings(
      settings, switchCorners(), Settings::Layout::ScreenSwitchCornerArray, Settings::Layout::ScreenSwitchCorner, false,
      static_cast<int>(NumSwitchCorners)
  );
  readSettings(
      settings, fixes(), Settings::Layout::ScreenFixArray, Settings::Layout::ScreenFix, 0, static_cast<int>(NumFixes)
  );

  m_Aliases = Settings::value(Settings::Screen::Aliases.arg(name)).toStringList();
}

void Screen::saveSettings(QSettingsProxy &settings) const
{

  const auto screenName = name();
  settings.setValue(Settings::Layout::ScreenName, screenName);

  if (screenName.isEmpty())
    return;

  Settings::setValue(Settings::Screen::Aliases.arg(screenName), m_Aliases);

  settings.setValue(Settings::Layout::ScreenSwitchCornerSize, switchCornerSize());

  writeSettings(settings, modifiers(), Settings::Layout::ScreenModifierArray, Settings::Layout::ScreenModifier);
  writeSettings(
      settings, switchCorners(), Settings::Layout::ScreenSwitchCornerArray, Settings::Layout::ScreenSwitchCorner
  );
  writeSettings(settings, fixes(), Settings::Layout::ScreenFixArray, Settings::Layout::ScreenFix);
}

bool Screen::operator==(const Screen &screen) const
{
  return m_Name == screen.m_Name && m_Aliases == screen.m_Aliases && m_Modifiers == screen.m_Modifiers &&
         m_SwitchCorners == screen.m_SwitchCorners && m_SwitchCornerSize == screen.m_SwitchCornerSize &&
         m_Fixes == screen.m_Fixes && m_Swapped == screen.m_Swapped && m_isServer == screen.m_isServer;
}
