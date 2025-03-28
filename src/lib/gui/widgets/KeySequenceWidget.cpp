/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "KeySequenceWidget.h"

#include <QMouseEvent>
#include <iostream>

KeySequenceWidget::KeySequenceWidget(QWidget *parent, const KeySequence &seq)
    : QPushButton(parent),
      m_KeySequence(seq),
      m_BackupSequence(seq),
      m_Status(Stopped),
      m_MousePrefix("mousebutton("),
      m_MousePostfix(")"),
      m_KeyPrefix("keystroke("),
      m_KeyPostfix(")")
{
  setFocusPolicy(Qt::NoFocus);
  updateOutput();
}

void KeySequenceWidget::setKeySequence(const KeySequence &seq)
{
  m_KeySequence = seq;
  m_BackupSequence = seq;

  setStatus(Stopped);
  updateOutput();
}

void KeySequenceWidget::mousePressEvent(QMouseEvent *event)
{
  event->accept();

  if (status() == Stopped) {
    startRecording();
    return;
  }

  if (m_KeySequence.appendMouseButton(event->button()))
    stopRecording();

  updateOutput();
}

void KeySequenceWidget::startRecording()
{
  m_KeySequence = KeySequence();
  setDown(true);
  setFocus();
  grabKeyboard();
  setStatus(Recording);
}

void KeySequenceWidget::stopRecording()
{
  if (!keySequence().valid()) {
    m_KeySequence = backupSequence();
    updateOutput();
  }

  setDown(false);
  focusNextChild();
  releaseKeyboard();
  setStatus(Stopped);
  Q_EMIT keySequenceChanged();
}

bool KeySequenceWidget::event(QEvent *event)
{
  if (status() == Recording) {
    switch (event->type()) {
    case QEvent::KeyPress:
      keyPressEvent(static_cast<QKeyEvent *>(event));
      return true;

    case QEvent::MouseButtonRelease:
      event->accept();
      return true;

    case QEvent::ShortcutOverride:
      event->accept();
      return true;

    case QEvent::FocusOut:
      stopRecording();
      if (!valid()) {
        m_KeySequence = backupSequence();
        updateOutput();
      }
      break;

    default:
      break;
    }
  }

  return QPushButton::event(event);
}

void KeySequenceWidget::keyPressEvent(QKeyEvent *event)
{
  event->accept();

  if (status() == Stopped)
    return;

  if (m_KeySequence.appendKey(event->key(), event->modifiers()))
    stopRecording();

  updateOutput();
}

void KeySequenceWidget::updateOutput()
{
  QString s;

  if (m_KeySequence.isMouseButton())
    s = mousePrefix() + m_KeySequence.toString() + mousePostfix();
  else
    s = keyPrefix() + m_KeySequence.toString() + keyPostfix();

  setText(s);
}
