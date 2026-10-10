/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoreProcessTests.h"

#include "gui/core/CoreProcess.h"

using deskflow::gui::CoreProcess;

void CoreProcessTests::parseComputerShape_data()
{
  QTest::addColumn<QString>("args");
  QTest::addColumn<bool>("valid");
  QTest::addColumn<QString>("name");
  QTest::addColumn<QSize>("size");

  QTest::newRow("simple") << "laptop,1920,1080" << true << "laptop" << QSize(1920, 1080);
  QTest::newRow("comma in name") << "a,b,800,600" << true << "a,b" << QSize(800, 600);
  QTest::newRow("zero width") << "laptop,0,1080" << false << "" << QSize();
  QTest::newRow("not a number") << "laptop,wide,1080" << false << "" << QSize();
  QTest::newRow("missing size") << "laptop" << false << "" << QSize();
  QTest::newRow("empty name") << ",1920,1080" << false << "" << QSize();
}

void CoreProcessTests::parseComputerShape()
{
  QFETCH(QString, args);
  QFETCH(bool, valid);
  QFETCH(QString, name);
  QFETCH(QSize, size);

  QString parsedName;
  QSize parsedSize;
  QCOMPARE(CoreProcess::parseComputerShape(args, parsedName, parsedSize), valid);
  if (valid) {
    QCOMPARE(parsedName, name);
    QCOMPARE(parsedSize, size);
  }
}

QTEST_MAIN(CoreProcessTests)
