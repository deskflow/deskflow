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

#if !defined(KEYSEQUENCE_H)

#define KEYSEQUENCE_H

#include <QList>
#include <QString>

class QSettings;

class KeySequence {
public:
    KeySequence ();

public:
    QString toString () const;
    bool appendKey (int modifiers, int key);
    bool appendMouseButton (int button);
    bool isMouseButton () const;
    bool
    valid () const {
        return m_IsValid;
    }
    int
    modifiers () const {
        return m_Modifiers;
    }
    void saveSettings (QSettings& settings) const;
    void loadSettings (QSettings& settings);
    const QList<int>&
    sequence () const {
        return m_Sequence;
    }

private:
    void
    setValid (bool b) {
        m_IsValid = b;
    }
    void
    setModifiers (int i) {
        m_Modifiers = i;
    }
    QList<int>&
    sequence () {
        return m_Sequence;
    }

private:
    QList<int> m_Sequence;
    int m_Modifiers;
    bool m_IsValid;

    static QString keyToString (int key);
};

#endif
