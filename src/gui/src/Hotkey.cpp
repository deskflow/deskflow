/*
 * barrier -- mouse and keyboard sharing utility
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

Hotkey::Hotkey() :
    m_KeySequence(),
    m_Actions()
{
}

QString Hotkey::text() const
{
    QString text = m_KeySequence.toString();

    if (m_KeySequence.isMouseButton())
        return "mousebutton(" + text + ")";

    return "keystroke(" + text + ")";
}

void Hotkey::loadSettings(QSettings& settings)
{
    m_KeySequence.loadSettings(settings);

    m_Actions.clear();
    int num = settings.beginReadArray("actions");
    for (int i = 0; i < num; i++)
    {
        settings.setArrayIndex(i);
        Action a;
        a.loadSettings(settings);
        m_Actions.push_back(a);
    }

    settings.endArray();
}

void Hotkey::saveSettings(QSettings& settings) const
{
    m_KeySequence.saveSettings(settings);

    settings.beginWriteArray("actions");
    for (int i = 0; i < m_Actions.size(); i++)
    {
        settings.setArrayIndex(i);
        m_Actions[i].saveSettings(settings);
    }
    settings.endArray();
}

QTextStream& operator<<(QTextStream& outStream, const Hotkey& hotkey)
{
    // Don't write config if there are no actions
    if (hotkey.actions().size() == 0) {
        return outStream;
    }

    outStream << "\t" << hotkey.text() << " = ";
    for (int i = 0; i < hotkey.actions().size(); i++) {
        outStream << hotkey.actions()[i];
        if (i != hotkey.actions().size() - 1) {
            outStream << ", ";
        }
    }

    outStream << "\n";

    return outStream;
}
