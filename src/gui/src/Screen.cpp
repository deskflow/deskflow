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

#include "Screen.h"

#include <QtCore>
#include <QtGui>

Screen::Screen ()
    : m_Pixmap (QPixmap (":res/icons/64x64/video-display.png")),
      m_Swapped (false) {
    init ();
}

Screen::Screen (const QString& name)
    : m_Pixmap (QPixmap (":res/icons/64x64/video-display.png")),
      m_Swapped (false) {
    init ();
    setName (name);
}

void
Screen::init () {
    name ().clear ();
    aliases ().clear ();
    modifiers ().clear ();
    switchCorners ().clear ();
    fixes ().clear ();
    setSwitchCornerSize (0);

    // m_Modifiers, m_SwitchCorners and m_Fixes are QLists we use like
    // fixed-size arrays,
    // thus we need to make sure to fill them with the required number of
    // elements.
    for (int i = 0; i < NumModifiers; i++)
        modifiers () << i;

    for (int i = 0; i < NumSwitchCorners; i++)
        switchCorners () << false;

    for (int i = 0; i < NumFixes; i++)
        fixes () << false;
}

void
Screen::loadSettings (QSettings& settings) {
    setName (settings.value ("name").toString ());

    if (name ().isEmpty ())
        return;

    setSwitchCornerSize (settings.value ("switchCornerSize").toInt ());

    readSettings (settings, aliases (), "alias", QString (""));
    readSettings (settings,
                  modifiers (),
                  "modifier",
                  static_cast<int> (DefaultMod),
                  NumModifiers);
    readSettings (
        settings, switchCorners (), "switchCorner", false, NumSwitchCorners);
    readSettings (settings, fixes (), "fix", false, NumFixes);
}

void
Screen::saveSettings (QSettings& settings) const {
    settings.setValue ("name", name ());

    if (name ().isEmpty ())
        return;

    settings.setValue ("switchCornerSize", switchCornerSize ());

    writeSettings (settings, aliases (), "alias");
    writeSettings (settings, modifiers (), "modifier");
    writeSettings (settings, switchCorners (), "switchCorner");
    writeSettings (settings, fixes (), "fix");
}

QTextStream&
Screen::writeScreensSection (QTextStream& outStream) const {
    outStream << "\t" << name () << ":" << endl;

    for (int i = 0; i < modifiers ().size (); i++)
        if (modifier (i) != i)
            outStream << "\t\t" << modifierName (i) << " = "
                      << modifierName (modifier (i)) << endl;

    for (int i = 0; i < fixes ().size (); i++)
        outStream << "\t\t" << fixName (i) << " = "
                  << (fixes ()[i] ? "true" : "false") << endl;

    outStream << "\t\t"
              << "switchCorners = none ";
    for (int i = 0; i < switchCorners ().size (); i++)
        if (switchCorners ()[i])
            outStream << "+" << switchCornerName (i) << " ";
    outStream << endl;

    outStream << "\t\t"
              << "switchCornerSize = " << switchCornerSize () << endl;

    return outStream;
}

QTextStream&
Screen::writeAliasesSection (QTextStream& outStream) const {
    if (!aliases ().isEmpty ()) {
        outStream << "\t" << name () << ":" << endl;

        foreach (const QString& alias, aliases ())
            outStream << "\t\t" << alias << endl;
    }

    return outStream;
}

QDataStream&
operator<< (QDataStream& outStream, const Screen& screen) {
    return outStream << screen.name () << screen.switchCornerSize ()
                     << screen.aliases () << screen.modifiers ()
                     << screen.switchCorners () << screen.fixes ();
}

QDataStream&
operator>> (QDataStream& inStream, Screen& screen) {
    return inStream >> screen.m_Name >> screen.m_SwitchCornerSize >>
           screen.m_Aliases >> screen.m_Modifiers >> screen.m_SwitchCorners >>
           screen.m_Fixes;
}
