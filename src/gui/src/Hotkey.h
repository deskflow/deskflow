/*
 * synergy -- mouse and keyboard sharing utility
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

class HotkeyDialog;
class ServerConfigDialog;
class QSettings;

class Hotkey {
    friend class HotkeyDialog;
    friend class ServerConfigDialog;
    friend QTextStream&
    operator<< (QTextStream& outStream, const Hotkey& hotkey);

public:
    Hotkey ();

public:
    QString text () const;
    const KeySequence&
    keySequence () const {
        return m_KeySequence;
    }
    const ActionList&
    actions () const {
        return m_Actions;
    }

    void loadSettings (QSettings& settings);
    void saveSettings (QSettings& settings) const;

protected:
    KeySequence&
    keySequence () {
        return m_KeySequence;
    }
    void
    setKeySequence (const KeySequence& seq) {
        m_KeySequence = seq;
    }
    ActionList&
    actions () {
        return m_Actions;
    }


private:
    KeySequence m_KeySequence;
    ActionList m_Actions;
};

typedef QList<Hotkey> HotkeyList;

QTextStream& operator<< (QTextStream& outStream, const Hotkey& hotkey);

#endif
