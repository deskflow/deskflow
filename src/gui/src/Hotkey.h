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

#if !defined(HOTKEY_H)

#define HOTKEY_H

#include <QString>
#include <QList>
#include <QTextStream>

#include "Action.h"
#include "KeySequence.h"

#include <vector>

class HotkeyDialog;
class ServerConfigDialog;
class QSettings;

class Hotkey
{
    public:
        Hotkey();

        QString text() const;
        const KeySequence& keySequence() const { return m_KeySequence; }
        void setKeySequence(const KeySequence& seq) { m_KeySequence = seq; }

        const std::vector<Action>& actions() const { return m_Actions; }
        void appendAction(const Action& action) { m_Actions.push_back(action); }
        void setAction(int index, const Action& action) { m_Actions[index] = action; }
        void removeAction(int index) { m_Actions.erase(m_Actions.begin() + index); }

        void loadSettings(QSettings& settings);
        void saveSettings(QSettings& settings) const;


    private:
        KeySequence m_KeySequence;
        std::vector<Action> m_Actions;
};

QTextStream& operator<<(QTextStream& outStream, const Hotkey& hotkey);

#endif
