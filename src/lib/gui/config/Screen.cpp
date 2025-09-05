/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Screen.h"
#include "config/ScreenConfig.h"

using enum ScreenConfig::Modifier;
using enum ScreenConfig::SwitchCorner;
using enum ScreenConfig::Fix;

Screen::Screen(const QString &name)
{
  setName(name);
}

void Screen::loadSettings(QSettingsProxy &settings)
{
  setName(settings.value("name").toString());

  if (name().isEmpty())
    return;

  setSwitchCornerSize(settings.value("switchCornerSize").toInt());

  readSettings(settings, aliases(), "alias", QString(""));
  readSettings(settings, modifiers(), "modifier", static_cast<int>(DefaultMod), static_cast<int>(NumModifiers));
  readSettings(settings, switchCorners(), "switchCorner", false, static_cast<int>(NumSwitchCorners));
  readSettings(settings, fixes(), "fix", 0, static_cast<int>(NumFixes));

  auto enterCommand = settings.value("EnterScreenCommand").toString();
  if (enterCommand.isEmpty()) {
    m_enterScreenCommand = std::nullopt;
  } else {
    m_enterScreenCommand = enterCommand;
  }

  auto exitCommand = settings.value("ExitScreenCommand").toString();
  if (exitCommand.isEmpty()) {
    m_exitScreenCommand = std::nullopt;
  } else {
    m_exitScreenCommand = exitCommand;
  }
}

void Screen::saveSettings(QSettingsProxy &settings) const
{
  settings.setValue("name", name());

  if (name().isEmpty())
    return;

  settings.setValue("switchCornerSize", switchCornerSize());

  writeSettings(settings, aliases(), "alias");
  writeSettings(settings, modifiers(), "modifier");
  writeSettings(settings, switchCorners(), "switchCorner");
  writeSettings(settings, fixes(), "fix");

  if (m_enterScreenCommand.has_value()) {
    settings.setValue("EnterScreenCommand", m_enterScreenCommand.value());
  } else {
    settings.remove("EnterScreenCommand");
  }

  if (m_exitScreenCommand.has_value()) {
    settings.setValue("ExitScreenCommand", m_exitScreenCommand.value());
  } else {
    settings.remove("ExitScreenCommand");
  }
}

QString Screen::screensSection() const
{
  const auto lineTemplate = QStringLiteral("\t\t%1 = %2\n");

  QString out = QStringLiteral("\t%1:\n").arg(name());
  for (int i = 0; i < modifiers().size(); i++) {
    if (modifier(i) != i)
      out.append(lineTemplate.arg(modifierName(i), modifierName(modifier(i))));
  }

  for (int i = 0; i < fixes().size(); i++)
    out.append(lineTemplate.arg(fixName(i), fixes().at(i) ? QStringLiteral("true") : QStringLiteral("false")));

  auto corners = QStringLiteral("none");
  for (int i = 0; i < switchCorners().size(); i++) {
    if (switchCorners()[i])
      corners.append(QStringLiteral(" +%1 ").arg(switchCornerName(i)));
  }
  out.append(lineTemplate.arg(QStringLiteral("switchCorners"), corners));

  out.append(lineTemplate.arg(QStringLiteral("switchCornerSize"), QString::number(switchCornerSize())));

  return out;
}

QString Screen::aliasesSection() const
{
  QString out;
  if (!aliases().isEmpty()) {
    out = QStringLiteral("\t%1:\n").arg(name());

    for (const QString &alias : aliases())
      out.append(QStringLiteral("\t\t%1\n").arg(alias));
  }
  return out;
}

bool Screen::operator==(const Screen &screen) const
{
  return m_Name == screen.m_Name && m_Aliases == screen.m_Aliases && m_Modifiers == screen.m_Modifiers &&
         m_SwitchCorners == screen.m_SwitchCorners && m_SwitchCornerSize == screen.m_SwitchCornerSize &&
         m_Fixes == screen.m_Fixes && m_Swapped == screen.m_Swapped && m_isServer == screen.m_isServer &&
         m_enterScreenCommand == screen.m_enterScreenCommand && m_exitScreenCommand == screen.m_exitScreenCommand;
}
