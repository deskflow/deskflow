/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "PathTests.h"

#include "../../lib/base/Path.h"

#include <fstream>

void PathTests::initTestCase()
{
  QDir dir;
  QVERIFY(dir.mkpath("tmp/test"));
}

void PathTests::testOpen()
{
  const QString filePath = QStringLiteral("tmp/test/тіás.txt");
  QFile file(filePath);

  QVERIFY(file.open(QIODevice::WriteOnly));
  QVERIFY(file.write("test", 4));

  file.close();
  QVERIFY(!file.isOpen());

  std::ifstream inFile(deskflow::filesystem::path(filePath.toStdString()));
  QVERIFY(inFile.is_open());
}

QTEST_MAIN(PathTests)
