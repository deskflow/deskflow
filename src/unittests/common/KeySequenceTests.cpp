/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "KeySequenceTests.h"

#include "common/KeySequence.h"

void KeySequenceTests::toString_controlShiftPlus_usesNamedPlus()
{
  KeySequence sequence;

  sequence.appendKey(Qt::Key_Control, Qt::ControlModifier);
  sequence.appendKey(Qt::Key_Shift, Qt::ControlModifier | Qt::ShiftModifier);
  QVERIFY(sequence.appendKey(Qt::Key_Plus, Qt::ControlModifier | Qt::ShiftModifier));

  QCOMPARE(sequence.toString(), QStringLiteral("Control+Shift+Plus"));
}

void KeySequenceTests::toString_configSyntaxChars_areEscaped()
{
  // '#' starts a comment and ')' ends the argument list in the server config
  const QList<QPair<int, QString>> cases = {
      {Qt::Key_NumberSign, QStringLiteral("Shift+\\u0023")},
      {Qt::Key_ParenLeft, QStringLiteral("Shift+\\u0028")},
      {Qt::Key_ParenRight, QStringLiteral("Shift+\\u0029")},
  };

  for (const auto &[key, expected] : cases) {
    KeySequence sequence;
    sequence.appendKey(Qt::Key_Shift, Qt::ShiftModifier);
    QVERIFY(sequence.appendKey(key, Qt::ShiftModifier));
    QCOMPARE(sequence.toString(), expected);
  }
}

QTEST_MAIN(KeySequenceTests)
