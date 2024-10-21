/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
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

#include "Screen.h"
#include "config/ScreenConfig.h"

using namespace deskflow::gui::proxy;
using enum ScreenConfig::Modifier;
using enum ScreenConfig::SwitchCorner;
using enum ScreenConfig::Fix;

Screen::Screen()
{
  init();
}

Screen::Screen(const QString &name)
{
  init();
  setName(name);
}

void Screen::init()
{
  name().clear();
  aliases().clear();
  modifiers().clear();
  switchCorners().clear();
  fixes().clear();
  setSwitchCornerSize(0);

  // m_Modifiers, m_SwitchCorners and m_Fixes are QLists we use like fixed-size
  // arrays, thus we need to make sure to fill them with the required number of
  // elements.
  for (int i = 0; i < static_cast<int>(NumModifiers); i++)
    modifiers() << i;

  for (int i = 0; i < static_cast<int>(NumSwitchCorners); i++)
    switchCorners() << false;

  for (int i = 0; i < static_cast<int>(NumFixes); i++)
    fixes() << false;
}

void Screen::loadSettings(QSettingsProxy &settings)
{
  setName(settings.value("name").toString());

  if (name().isEmpty())
    return;

  setSwitchCornerSize(settings.value("switchCornerSize").toInt());

  readSettings(settings, aliases(), "alias", QString(""));
  readSettings(settings, modifiers(), "modifier", static_cast<int>(DefaultMod), static_cast<int>(NumModifiers));
  readSettings(settings, switchCorners(), "switchCorner", 0, static_cast<int>(NumSwitchCorners));
  readSettings(settings, fixes(), "fix", 0, static_cast<int>(NumFixes));
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
}

QTextStream &Screen::writeScreensSection(QTextStream &outStream) const
{
  outStream << "\t" << name() << ":" << Qt::endl;

  for (int i = 0; i < modifiers().size(); i++)
    if (modifier(i) != i)
      outStream << "\t\t" << modifierName(i) << " = " << modifierName(modifier(i)) << Qt::endl;

  for (int i = 0; i < fixes().size(); i++)
    outStream << "\t\t" << fixName(i) << " = " << (fixes()[i] ? "true" : "false") << Qt::endl;

  outStream << "\t\t"
            << "switchCorners = none ";
  for (int i = 0; i < switchCorners().size(); i++)
    if (switchCorners()[i])
      outStream << "+" << switchCornerName(i) << " ";
  outStream << Qt::endl;

  outStream << "\t\t"
            << "switchCornerSize = " << switchCornerSize() << Qt::endl;

  return outStream;
}

QTextStream &Screen::writeAliasesSection(QTextStream &outStream) const
{
  if (!aliases().isEmpty()) {
    outStream << "\t" << name() << ":" << Qt::endl;

    for (const QString &alias : aliases())
      outStream << "\t\t" << alias << Qt::endl;
  }

  return outStream;
}

bool Screen::operator==(const Screen &screen) const
{
  return m_Name == screen.m_Name && m_Aliases == screen.m_Aliases && m_Modifiers == screen.m_Modifiers &&
         m_SwitchCorners == screen.m_SwitchCorners && m_SwitchCornerSize == screen.m_SwitchCornerSize &&
         m_Fixes == screen.m_Fixes && m_Swapped == screen.m_Swapped && m_isServer == screen.m_isServer;
}
