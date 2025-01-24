/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QPushButton>

#include "KeySequence.h"

class KeySequenceWidget : public QPushButton
{
  Q_OBJECT

public:
  KeySequenceWidget(QWidget *parent, const KeySequence &seq = KeySequence());

signals:
  void keySequenceChanged();

public:
  const QString &mousePrefix() const
  {
    return m_MousePrefix;
  }
  const QString &mousePostfix() const
  {
    return m_MousePostfix;
  }
  const QString &keyPrefix() const
  {
    return m_KeyPrefix;
  }
  const QString &keyPostfix() const
  {
    return m_KeyPostfix;
  }

  void setMousePrefix(const QString &s)
  {
    m_MousePrefix = s;
  }
  void setMousePostfix(const QString &s)
  {
    m_MousePostfix = s;
  }
  void setKeyPrefix(const QString &s)
  {
    m_KeyPrefix = s;
  }
  void setKeyPostfix(const QString &s)
  {
    m_KeyPostfix = s;
  }

  const KeySequence &keySequence() const
  {
    return m_KeySequence;
  }
  const KeySequence &backupSequence() const
  {
    return m_BackupSequence;
  }
  void setKeySequence(const KeySequence &seq);

  bool valid() const
  {
    return keySequence().valid();
  }

protected:
  void mousePressEvent(QMouseEvent *);
  void keyPressEvent(QKeyEvent *);
  bool event(QEvent *event);
  void appendToSequence(int key);
  void updateOutput();
  void startRecording();
  void stopRecording();

private:
  enum Status
  {
    Stopped,
    Recording
  };
  void setStatus(Status s)
  {
    m_Status = s;
  }
  Status status() const
  {
    return m_Status;
  }

private:
  KeySequence m_KeySequence;
  KeySequence m_BackupSequence;
  Status m_Status;
  QString m_MousePrefix;
  QString m_MousePostfix;
  QString m_KeyPrefix;
  QString m_KeyPostfix;
};
