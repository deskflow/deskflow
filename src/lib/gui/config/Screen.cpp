/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Chris Rizzitello <sithlord48@gmail.com>
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
  const auto name = settings.value("name").toString();
  setName(name);

  if (name.isEmpty())
    return;

  setSwitchCornerSize(Settings::value(Settings::Screen::SwitchCornerSize.arg(name)).toInt());

  readSettings(settings, modifiers(), "modifier", static_cast<int>(DefaultMod), static_cast<int>(NumModifiers));

  m_SwitchCorners[static_cast<int>(TopLeft)] =
      Settings::value(Settings::Screen::SwitchCornerTopLeft.arg(name)).toBool();
  m_SwitchCorners[static_cast<int>(TopRight)] =
      Settings::value(Settings::Screen::SwitchCornerTopRight.arg(name)).toBool();
  m_SwitchCorners[static_cast<int>(BottomLeft)] =
      Settings::value(Settings::Screen::SwitchCornerBottomLeft.arg(name)).toBool();
  m_SwitchCorners[static_cast<int>(BottomRight)] =
      Settings::value(Settings::Screen::SwitchCornerBottomRight.arg(name)).toBool();

  m_Fixes[static_cast<int>(CapsLock)] = Settings::value(Settings::Screen::HalfDuplexCapsLock.arg(name)).toBool();
  m_Fixes[static_cast<int>(NumLock)] = Settings::value(Settings::Screen::HalfDuplexNumLock.arg(name)).toBool();
  m_Fixes[static_cast<int>(ScrollLock)] = Settings::value(Settings::Screen::HalfDuplexScrollLock.arg(name)).toBool();
  m_Fixes[static_cast<int>(XTest)] = Settings::value(Settings::Screen::XtestIsXineramaUnaware.arg(name)).toBool();

  m_Aliases = Settings::value(Settings::Screen::Aliases.arg(name)).toStringList();
}

void Screen::saveSettings(QSettingsProxy &settings) const
{
  const auto screenName = name();
  settings.setValue("name", screenName);

  if (screenName.isEmpty())
    return;

  Settings::setValue(Settings::Screen::Name.arg(screenName), screenName);
  Settings::setValue(Settings::Screen::Aliases.arg(screenName), m_Aliases);
  Settings::setValue(Settings::Screen::HalfDuplexCapsLock.arg(screenName), m_Fixes[static_cast<int>(CapsLock)]);
  Settings::setValue(Settings::Screen::HalfDuplexNumLock.arg(screenName), m_Fixes[static_cast<int>(NumLock)]);
  Settings::setValue(Settings::Screen::HalfDuplexScrollLock.arg(screenName), m_Fixes[static_cast<int>(ScrollLock)]);
  Settings::setValue(Settings::Screen::XtestIsXineramaUnaware.arg(screenName), m_Fixes[static_cast<int>(XTest)]);
  Settings::setValue(Settings::Screen::SwitchCornerSize.arg(screenName), switchCornerSize());
  Settings::setValue(Settings::Screen::SwitchCornerTopLeft.arg(screenName), m_SwitchCorners[static_cast<int>(TopLeft)]);
  Settings::setValue(
      Settings::Screen::SwitchCornerTopRight.arg(screenName), m_SwitchCorners[static_cast<int>(TopRight)]
  );
  Settings::setValue(
      Settings::Screen::SwitchCornerBottomLeft.arg(screenName), m_SwitchCorners[static_cast<int>(BottomLeft)]
  );
  Settings::setValue(
      Settings::Screen::SwitchCornerBottomRight.arg(screenName), m_SwitchCorners[static_cast<int>(BottomRight)]
  );

  writeSettings(settings, modifiers(), "modifier");
}

QString Screen::screensSection() const
{
  const auto lineTemplate = QStringLiteral("\t\t%1 = %2\n");

  QString out = QStringLiteral("\t%1:\n").arg(name());
  for (int i = 0; i < modifiers().size(); i++) {
    if (modifier(i) != i)
      out.append(lineTemplate.arg(modifierName(i), modifierName(modifier(i))));
  }
  return out;
}

bool Screen::operator==(const Screen &screen) const
{
  return m_Name == screen.m_Name && m_Aliases == screen.m_Aliases && m_Modifiers == screen.m_Modifiers &&
         m_SwitchCorners == screen.m_SwitchCorners && m_SwitchCornerSize == screen.m_SwitchCornerSize &&
         m_Fixes == screen.m_Fixes && m_Swapped == screen.m_Swapped && m_isServer == screen.m_isServer;
}
