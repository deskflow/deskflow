/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ConfigTests.h"

#include "../../lib/deskflow/Config.h"

using namespace deskflow;
const auto kTestFilename = "tmp/test/test.toml";

void ConfigTests::initTestCase()
{
  QDir dir;
  QVERIFY(dir.mkpath("tmp/test"));

  m_arch.init();
  m_log.setFilter(kDEBUG2);
}

void ConfigTests::loadFile()
{
  QFile testFile(kTestFilename);
  QVERIFY(testFile.open(QIODevice::WriteOnly | QIODevice::Text));

  QTextStream out(&testFile);
  out << "[test.args]\n"
         R"(test-arg = "test opt")";

  testFile.close();
  QVERIFY(!testFile.isOpen());

  Config config(kTestFilename, "test");

  const auto result = config.load("test");
  QVERIFY(result);
  QCOMPARE(config.argc(), 3);
  QCOMPARE(config.argv()[0], "test");
  QCOMPARE(config.argv()[1], "--test-arg");
  QCOMPARE(config.argv()[2], "test opt");
}

void ConfigTests::load_EmptyFile()
{
  Config config("", "test");
  QVERIFY_THROWS_EXCEPTION(Config::NoConfigFilenameError, config.load("test"));
}

void ConfigTests::load_NonExsitingFile()
{
  Config config("nonexistent.toml", "test");

  const auto result = config.load("test");
  QVERIFY(!result);
}

void ConfigTests::load_InvalidConfig()
{
  QFile testFile(kTestFilename);
  QVERIFY(testFile.open(QIODevice::WriteOnly));

  QVERIFY(testFile.write("foobar"));

  testFile.close();
  QVERIFY(!testFile.isOpen());

  Config config(kTestFilename, "test");
  QVERIFY_THROWS_EXCEPTION(Config::ParseError, config.load("test"));
}

void ConfigTests::load_missingSections()
{
  QFile testFile(kTestFilename);
  QVERIFY(testFile.open(QIODevice::WriteOnly));

  testFile.close();
  QVERIFY(!testFile.isOpen());

  Config config(kTestFilename, "missing");
  const auto result = config.load("test");
  QVERIFY(!result);
}

void ConfigTests::load_badTable()
{
  QFile testFile(kTestFilename);
  QVERIFY(testFile.open(QIODevice::WriteOnly));

  QVERIFY(testFile.write("[test]"));

  testFile.close();
  QVERIFY(!testFile.isOpen());

  Config config(kTestFilename, "test");

  const auto result = config.load("test");
  QVERIFY(!result);
}

void ConfigTests::load_lastArg()
{
  QFile testFile(kTestFilename);
  QVERIFY(testFile.open(QIODevice::WriteOnly));

  QVERIFY(testFile.write(
      "[test.args]\n"
      R"(_last = "test last")"
      "\n"
      R"(test-second = true)"
  ));

  testFile.close();
  QVERIFY(!testFile.isOpen());

  deskflow::Config config(kTestFilename, "test");

  const auto result = config.load("test-first");
  QVERIFY(result);
  QCOMPARE(config.argc(), 3);
  QCOMPARE(config.argv()[0], "test-first");
  QCOMPARE(config.argv()[1], "--test-second");
  QCOMPARE(config.argv()[2], "test last");
}

void ConfigTests::load_noArgs()
{
  QFile testFile(kTestFilename);
  QVERIFY(testFile.open(QIODevice::WriteOnly));

  QVERIFY(testFile.write("[test.args]"));

  testFile.close();
  QVERIFY(!testFile.isOpen());

  deskflow::Config config(kTestFilename, "test");

  const auto result = config.load("");
  QVERIFY(!result);
}

QTEST_MAIN(ConfigTests)
