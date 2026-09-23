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

  setModifier(Alt, modifierValueFromString(Settings::value(Settings::Screen::ModifierAlt.arg(name)).toString()));
  setModifier(AltGr, modifierValueFromString(Settings::value(Settings::Screen::ModifierAltGr.arg(name)).toString()));
  setModifier(Ctrl, modifierValueFromString(Settings::value(Settings::Screen::ModifierCtrl.arg(name)).toString()));
  setModifier(Meta, modifierValueFromString(Settings::value(Settings::Screen::ModifierMeta.arg(name)).toString()));
  setModifier(Shift, modifierValueFromString(Settings::value(Settings::Screen::ModifierShift.arg(name)).toString()));
  setModifier(Super, modifierValueFromString(Settings::value(Settings::Screen::ModifierSuper.arg(name)).toString()));

  setSwitchCornerSize(Settings::value(Settings::Screen::SwitchCornerSize.arg(name)).toInt());

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

  Settings::setValue(
      Settings::Screen::ModifierAlt.arg(screenName),
      valueToKeyboardModifierOption(m_Modifiers.at(static_cast<qsizetype>(KeyboardModifier::Alt)))
  );
  Settings::setValue(
      Settings::Screen::ModifierAltGr.arg(screenName),
      valueToKeyboardModifierOption(m_Modifiers.at(static_cast<qsizetype>(KeyboardModifier::AltGr)))
  );
  Settings::setValue(
      Settings::Screen::ModifierCtrl.arg(screenName),
      valueToKeyboardModifierOption(m_Modifiers.at(static_cast<qsizetype>(KeyboardModifier::Ctrl)))
  );
  Settings::setValue(
      Settings::Screen::ModifierMeta.arg(screenName),
      valueToKeyboardModifierOption(m_Modifiers.at(static_cast<qsizetype>(KeyboardModifier::Meta)))
  );
  Settings::setValue(
      Settings::Screen::ModifierShift.arg(screenName),
      valueToKeyboardModifierOption(m_Modifiers.at(static_cast<qsizetype>(KeyboardModifier::Shift)))
  );
  Settings::setValue(
      Settings::Screen::ModifierSuper.arg(screenName),
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
