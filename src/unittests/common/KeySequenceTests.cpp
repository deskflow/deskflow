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

void KeySequenceTests::fromString_controlShiftPlus_roundTrips()
{
  const auto sequence = KeySequence::fromString(QStringLiteral("Control+Shift+Plus"));

  QVERIFY(sequence.valid());
  QCOMPARE(sequence.toString(), QStringLiteral("Control+Shift+Plus"));
}

void KeySequenceTests::fromString_invalidSequence_returnsInvalid()
{
  QVERIFY(!KeySequence::fromString(QStringLiteral("Control+NotAKey")).valid());
}

QTEST_MAIN(KeySequenceTests)
