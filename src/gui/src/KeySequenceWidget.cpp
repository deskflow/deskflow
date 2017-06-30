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

#include "KeySequenceWidget.h"

#include <iostream>
#include <QMouseEvent>

KeySequenceWidget::KeySequenceWidget (QWidget* parent, const KeySequence& seq)
    : QPushButton (parent),
      m_KeySequence (seq),
      m_BackupSequence (seq),
      m_Status (Stopped),
      m_MousePrefix ("mousebutton("),
      m_MousePostfix (")"),
      m_KeyPrefix ("keystroke("),
      m_KeyPostfix (")") {
    setFocusPolicy (Qt::NoFocus);
    updateOutput ();
}

void
KeySequenceWidget::setKeySequence (const KeySequence& seq) {
    keySequence ()    = seq;
    backupSequence () = seq;

    setStatus (Stopped);
    updateOutput ();
}

void
KeySequenceWidget::mousePressEvent (QMouseEvent* event) {
    event->accept ();

    if (status () == Stopped) {
        startRecording ();
        return;
    }

    if (m_KeySequence.appendMouseButton (event->button ()))
        stopRecording ();

    updateOutput ();
}

void
KeySequenceWidget::startRecording () {
    keySequence () = KeySequence ();
    setDown (true);
    setFocus ();
    grabKeyboard ();
    setStatus (Recording);
}

void
KeySequenceWidget::stopRecording () {
    if (!keySequence ().valid ()) {
        keySequence () = backupSequence ();
        updateOutput ();
    }

    setDown (false);
    focusNextChild ();
    releaseKeyboard ();
    setStatus (Stopped);
    emit keySequenceChanged ();
}

bool
KeySequenceWidget::event (QEvent* event) {
    if (status () == Recording) {
        switch (event->type ()) {
            case QEvent::KeyPress:
                keyPressEvent (static_cast<QKeyEvent*> (event));
                return true;

            case QEvent::MouseButtonRelease:
                event->accept ();
                return true;

            case QEvent::ShortcutOverride:
                event->accept ();
                return true;

            case QEvent::FocusOut:
                stopRecording ();
                if (!valid ()) {
                    keySequence () = backupSequence ();
                    updateOutput ();
                }
                break;

            default:
                break;
        }
    }

    return QPushButton::event (event);
}

void
KeySequenceWidget::keyPressEvent (QKeyEvent* event) {
    event->accept ();

    if (status () == Stopped)
        return;

    if (m_KeySequence.appendKey (event->key (), event->modifiers ()))
        stopRecording ();

    updateOutput ();
}

void
KeySequenceWidget::updateOutput () {
    QString s;

    if (m_KeySequence.isMouseButton ())
        s = mousePrefix () + m_KeySequence.toString () + mousePostfix ();
    else
        s = keyPrefix () + m_KeySequence.toString () + keyPostfix ();

    setText (s);
}
