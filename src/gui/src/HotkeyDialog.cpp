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

#include "HotkeyDialog.h"

#include <QtCore>
#include <QtGui>

HotkeyDialog::HotkeyDialog (QWidget* parent, Hotkey& hotkey)
    : QDialog (parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      Ui::HotkeyDialogBase (),
      m_Hotkey (hotkey) {
    setupUi (this);

    m_pKeySequenceWidgetHotkey->setText (m_Hotkey.text ());
}

void
HotkeyDialog::accept () {
    if (!sequenceWidget ()->valid ())
        return;

    hotkey ().setKeySequence (sequenceWidget ()->keySequence ());
    QDialog::accept ();
}
