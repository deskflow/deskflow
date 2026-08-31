/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "KeySequenceWidgetTests.h"

#include "gui/widgets/KeySequenceWidget.h"

void KeySequenceWidgetTests::focusPolicy_default_strongFocus()
{
  KeySequenceWidget widget(nullptr);

  QCOMPARE(widget.focusPolicy(), Qt::StrongFocus);
}

void KeySequenceWidgetTests::keyClick_stoppedWidget_recordsKey()
{
  KeySequenceWidget widget(nullptr);

  QTest::keyClick(&widget, Qt::Key_A);

  QCOMPARE(widget.keySequence().toString(), QStringLiteral("a"));
}

QTEST_MAIN(KeySequenceWidgetTests)
