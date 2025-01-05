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

#include "Hotkey.h"

#include <QSettings>

Hotkey::Hotkey() : m_keySequence{}, m_Actions()
{
}

QString Hotkey::text() const
{
  return m_keySequence.isMouseButton() ? kMousebutton.arg(m_keySequence.toString())
                                       : kKeystroke.arg(m_keySequence.toString());
}

void Hotkey::loadSettings(QSettings &settings)
{
  m_keySequence.loadSettings(settings);

  actions().clear();
  int num = settings.beginReadArray(kSectionActions);
  for (int i = 0; i < num; i++) {
    settings.setArrayIndex(i);
    Action a;
    a.loadSettings(settings);
    actions().append(a);
  }

  settings.endArray();
}

void Hotkey::saveSettings(QSettings &settings) const
{
  m_keySequence.saveSettings(settings);

  settings.beginWriteArray(kSectionActions);
  for (int i = 0; i < actions().size(); i++) {
    settings.setArrayIndex(i);
    actions()[i].saveSettings(settings);
  }
  settings.endArray();
}

bool Hotkey::operator==(const Hotkey &hk) const
{
  return m_keySequence == hk.keySequence() && m_Actions == hk.m_Actions;
}

QTextStream &operator<<(QTextStream &outStream, const Hotkey &hotkey)
{
  // Don't write a config if there is no actions
  if (hotkey.actions().size() == 0)
    return outStream;

  QString outText = QStringLiteral("\t%1 = ").arg(hotkey.text());
  for (int i = 0; i < hotkey.actions().size(); i++) {
    outText.append(hotkey.actions().at(i).text());
    if (i != hotkey.actions().size() - 1) {
      outText.append(QStringLiteral(", "));
    }
  }
  outText.append(QStringLiteral("\n"));

  outStream << outText;
  return outStream;
}
