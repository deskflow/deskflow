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

#if !defined(SCREEN__H)

#define SCREEN__H

#include <QPixmap>
#include <QString>
#include <QList>
#include <QStringList>

#include "BaseConfig.h"

class QSettings;
class QTextStream;

class ScreenSettingsDialog;

class Screen : public BaseConfig {
    friend QDataStream&
    operator<< (QDataStream& outStream, const Screen& screen);
    friend QDataStream& operator>> (QDataStream& inStream, Screen& screen);
    friend class ScreenSettingsDialog;
    friend class ScreenSetupModel;
    friend class ScreenSetupView;

public:
    Screen ();
    Screen (const QString& name);

public:
    const QPixmap*
    pixmap () const {
        return &m_Pixmap;
    }
    const QString&
    name () const {
        return m_Name;
    }
    const QStringList&
    aliases () const {
        return m_Aliases;
    }

    bool
    isNull () const {
        return m_Name.isEmpty ();
    }
    int
    modifier (int m) const {
        return m_Modifiers[m] == DefaultMod ? m : m_Modifiers[m];
    }
    const QList<int>&
    modifiers () const {
        return m_Modifiers;
    }
    bool
    switchCorner (int c) const {
        return m_SwitchCorners[c];
    }
    const QList<bool>&
    switchCorners () const {
        return m_SwitchCorners;
    }
    int
    switchCornerSize () const {
        return m_SwitchCornerSize;
    }
    bool
    fix (Fix f) const {
        return m_Fixes[f];
    }
    const QList<bool>&
    fixes () const {
        return m_Fixes;
    }

    void loadSettings (QSettings& settings);
    void saveSettings (QSettings& settings) const;
    QTextStream& writeScreensSection (QTextStream& outStream) const;
    QTextStream& writeAliasesSection (QTextStream& outStream) const;

    bool
    swapped () const {
        return m_Swapped;
    }
    QString&
    name () {
        return m_Name;
    }
    void
    setName (const QString& name) {
        m_Name = name;
    }

protected:
    void init ();
    QPixmap*
    pixmap () {
        return &m_Pixmap;
    }

    void
    setPixmap (const QPixmap& pixmap) {
        m_Pixmap = pixmap;
    }
    QStringList&
    aliases () {
        return m_Aliases;
    }
    void
    setModifier (int m, int n) {
        m_Modifiers[m] = n;
    }
    QList<int>&
    modifiers () {
        return m_Modifiers;
    }
    void
    addAlias (const QString& alias) {
        m_Aliases.append (alias);
    }
    void
    setSwitchCorner (int c, bool on) {
        m_SwitchCorners[c] = on;
    }
    QList<bool>&
    switchCorners () {
        return m_SwitchCorners;
    }
    void
    setSwitchCornerSize (int val) {
        m_SwitchCornerSize = val;
    }
    void
    setFix (int f, bool on) {
        m_Fixes[f] = on;
    }
    QList<bool>&
    fixes () {
        return m_Fixes;
    }
    void
    setSwapped (bool on) {
        m_Swapped = on;
    }

private:
    QPixmap m_Pixmap;
    QString m_Name;

    QStringList m_Aliases;
    QList<int> m_Modifiers;
    QList<bool> m_SwitchCorners;
    int m_SwitchCornerSize;
    QList<bool> m_Fixes;

    bool m_Swapped;
};

typedef QList<Screen> ScreenList;

QDataStream& operator<< (QDataStream& outStream, const Screen& screen);
QDataStream& operator>> (QDataStream& inStream, Screen& screen);

#endif
