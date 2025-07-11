/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QList>
#include <QString>

class QSettings;

class KeySequence
{
public:
  KeySequence() = default;

  QString toString() const;
  bool appendKey(int modifiers, int key);
  bool appendMouseButton(int button);
  bool isMouseButton() const;
  bool valid() const
  {
    return m_IsValid;
  }
  int modifiers() const
  {
    return m_Modifiers;
  }
  void saveSettings(QSettings &settings) const;
  void loadSettings(QSettings &settings);
  const QList<int> &sequence() const
  {
    return m_Sequence;
  }

  bool operator==(const KeySequence &ks) const;

private:
  void setValid(bool b)
  {
    m_IsValid = b;
  }
  void setModifiers(int i)
  {
    m_Modifiers = i;
  }
  QList<int> &sequence()
  {
    return m_Sequence;
  }

  QList<int> m_Sequence = {};
  int m_Modifiers = 0;
  bool m_IsValid = false;

  inline static const int kStrSize = 4;
  inline static const int kBase = 16;
  inline static const QChar kFillChar = QChar('0');

  static QString keyToString(int key);
};
