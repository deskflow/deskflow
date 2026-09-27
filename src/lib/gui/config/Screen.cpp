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

using enum KeyboardModifier;
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

  setModifier(Alt, modifierValueFromString(Settings::value(Settings::Computer::ModifierAlt.arg(name)).toString()));
  setModifier(AltGr, modifierValueFromString(Settings::value(Settings::Computer::ModifierAltGr.arg(name)).toString()));
  setModifier(Ctrl, modifierValueFromString(Settings::value(Settings::Computer::ModifierCtrl.arg(name)).toString()));
  setModifier(Meta, modifierValueFromString(Settings::value(Settings::Computer::ModifierMeta.arg(name)).toString()));
  setModifier(Shift, modifierValueFromString(Settings::value(Settings::Computer::ModifierShift.arg(name)).toString()));
  setModifier(Super, modifierValueFromString(Settings::value(Settings::Computer::ModifierSuper.arg(name)).toString()));

  setSwitchCornerSize(Settings::value(Settings::Computer::SwitchCornerSize.arg(name)).toInt());

  m_SwitchCorners[static_cast<int>(TopLeft)] =
      Settings::value(Settings::Computer::SwitchCornerTopLeft.arg(name)).toBool();
  m_SwitchCorners[static_cast<int>(TopRight)] =
      Settings::value(Settings::Computer::SwitchCornerTopRight.arg(name)).toBool();
  m_SwitchCorners[static_cast<int>(BottomLeft)] =
      Settings::value(Settings::Computer::SwitchCornerBottomLeft.arg(name)).toBool();
  m_SwitchCorners[static_cast<int>(BottomRight)] =
      Settings::value(Settings::Computer::SwitchCornerBottomRight.arg(name)).toBool();

  m_Fixes[static_cast<int>(CapsLock)] = Settings::value(Settings::Computer::HalfDuplexCapsLock.arg(name)).toBool();
  m_Fixes[static_cast<int>(NumLock)] = Settings::value(Settings::Computer::HalfDuplexNumLock.arg(name)).toBool();
  m_Fixes[static_cast<int>(ScrollLock)] = Settings::value(Settings::Computer::HalfDuplexScrollLock.arg(name)).toBool();
  m_Fixes[static_cast<int>(XTest)] = Settings::value(Settings::Computer::XtestIsXineramaUnaware.arg(name)).toBool();

  m_Aliases = Settings::value(Settings::Computer::Aliases.arg(name)).toStringList();
}

void Screen::saveSettings(QSettingsProxy &settings) const
{
  const auto screenName = name();
  settings.setValue("name", screenName);

  if (screenName.isEmpty())
    return;

  Settings::setValue(Settings::Computer::Name.arg(screenName), screenName);
  Settings::setValue(Settings::Computer::Aliases.arg(screenName), m_Aliases);
  Settings::setValue(Settings::Computer::HalfDuplexCapsLock.arg(screenName), m_Fixes[static_cast<int>(CapsLock)]);
  Settings::setValue(Settings::Computer::HalfDuplexNumLock.arg(screenName), m_Fixes[static_cast<int>(NumLock)]);
  Settings::setValue(Settings::Computer::HalfDuplexScrollLock.arg(screenName), m_Fixes[static_cast<int>(ScrollLock)]);
  Settings::setValue(Settings::Computer::XtestIsXineramaUnaware.arg(screenName), m_Fixes[static_cast<int>(XTest)]);
  Settings::setValue(Settings::Computer::SwitchCornerSize.arg(screenName), switchCornerSize());
  Settings::setValue(
      Settings::Computer::SwitchCornerTopLeft.arg(screenName), m_SwitchCorners[static_cast<int>(TopLeft)]
  );
  Settings::setValue(
      Settings::Computer::SwitchCornerTopRight.arg(screenName), m_SwitchCorners[static_cast<int>(TopRight)]
  );
  Settings::setValue(
      Settings::Computer::SwitchCornerBottomLeft.arg(screenName), m_SwitchCorners[static_cast<int>(BottomLeft)]
  );
  Settings::setValue(
      Settings::Computer::SwitchCornerBottomRight.arg(screenName), m_SwitchCorners[static_cast<int>(BottomRight)]
  );

  Settings::setValue(
      Settings::Computer::ModifierAlt.arg(screenName),
      valueToKeyboardModifierOption(m_Modifiers.at(static_cast<qsizetype>(KeyboardModifier::Alt)))
  );
  Settings::setValue(
      Settings::Computer::ModifierAltGr.arg(screenName),
      valueToKeyboardModifierOption(m_Modifiers.at(static_cast<qsizetype>(KeyboardModifier::AltGr)))
  );
  Settings::setValue(
      Settings::Computer::ModifierCtrl.arg(screenName),
      valueToKeyboardModifierOption(m_Modifiers.at(static_cast<qsizetype>(KeyboardModifier::Ctrl)))
  );
  Settings::setValue(
      Settings::Computer::ModifierMeta.arg(screenName),
      valueToKeyboardModifierOption(m_Modifiers.at(static_cast<qsizetype>(KeyboardModifier::Meta)))
  );
  Settings::setValue(
      Settings::Computer::ModifierShift.arg(screenName),
      valueToKeyboardModifierOption(m_Modifiers.at(static_cast<qsizetype>(KeyboardModifier::Shift)))
  );
  Settings::setValue(
      Settings::Computer::ModifierSuper.arg(screenName),
      valueToKeyboardModifierOption(m_Modifiers.at(static_cast<qsizetype>(KeyboardModifier::Super)))
  );
}

QString Screen::screensSection() const
{
  return QStringLiteral("\t%1:\n").arg(name());
}

bool Screen::operator==(const Screen &screen) const
{
  return m_Name == screen.m_Name && m_Aliases == screen.m_Aliases && m_Modifiers == screen.m_Modifiers &&
         m_SwitchCorners == screen.m_SwitchCorners && m_SwitchCornerSize == screen.m_SwitchCornerSize &&
         m_Fixes == screen.m_Fixes && m_Swapped == screen.m_Swapped && m_isServer == screen.m_isServer;
}
