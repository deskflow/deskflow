/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "DotEnvTests.h"

#include "../../lib/gui/DotEnv.h"

void DotEnvTests::initTestCase()
{
  QDir dir;
  QVERIFY(dir.mkpath("tmp/test"));
}

void DotEnvTests::invalidFile()
{
  deskflow::gui::dotenv(m_envFile);

  const QString actualValue = qEnvironmentVariable("TEST_ENV_VAR");

  QVERIFY(actualValue.isEmpty());
  QCOMPARE(actualValue, "");
}

void DotEnvTests::validFile()
{
  QFile file(m_envFile);
  QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));

  const QString key = "TEST_ENV_VAR";
  const QString value = R"("test value")";
  const QString entry = key + " = " + value;

  QTextStream out(&file);
  out << " # Comment" << Qt::endl;
  out << "FOOBAR" << Qt::endl;
  out << entry << Qt::endl;
  file.close();

  deskflow::gui::dotenv(m_envFile);

  const QString actualValue = qEnvironmentVariable(qPrintable(key));

  QCOMPARE(actualValue.toStdString(), "test value");

  QVERIFY(QFile::remove(m_envFile));
}

QTEST_MAIN(DotEnvTests)
