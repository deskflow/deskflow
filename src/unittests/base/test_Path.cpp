/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "test_Path.h"

#include "base/Path.h"
#include <fstream>

void Path_Test::initTestCase()
{
  QDir dir;
  QVERIFY(dir.mkpath("tmp/test"));
}

void Path_Test::testOpen()
{
  const QString filePath = QStringLiteral("tmp/test/тіás.txt");
  QFile file(filePath);
  file.open(QIODevice::WriteOnly);
  file.write("test", 4);
  file.close();

  std::ifstream inFile(deskflow::filesystem::path(filePath.toStdString()));
  QVERIFY(inFile.is_open());
}

QTEST_MAIN(Path_Test)
