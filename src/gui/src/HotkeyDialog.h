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

#if !defined(HOTKEYDIALOG_H)

#define HOTKEYDIALOG_H

#include "ui_HotkeyDialogBase.h"
#include "Hotkey.h"

#include <QDialog>

class HotkeyDialog : public QDialog, public Ui::HotkeyDialogBase {
    Q_OBJECT

public:
    HotkeyDialog (QWidget* parent, Hotkey& hotkey);

public:
    const Hotkey&
    hotkey () const {
        return m_Hotkey;
    }

protected slots:
    void accept ();

protected:
    const KeySequenceWidget*
    sequenceWidget () const {
        return m_pKeySequenceWidgetHotkey;
    }
    Hotkey&
    hotkey () {
        return m_Hotkey;
    }

private:
    Hotkey& m_Hotkey;
};

#endif
