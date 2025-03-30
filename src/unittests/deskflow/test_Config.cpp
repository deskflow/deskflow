/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QtTest>

#include "test_Config.h"

#include "base/Log.h"

#include <fstream>

using namespace deskflow;
const auto kTestFilename = "tmp/test/test.toml";

void Config_Test::initTestCase()
{
  QDir dir;
  QVERIFY(dir.mkpath("tmp/test"));
  m_arch.init();
  m_log.setFilter(kDEBUG2);
}

void Config_Test::loadFile()
{
  QFile testFile(kTestFilename);
  QVERIFY(testFile.open(QIODevice::WriteOnly | QIODevice::Text));
  QTextStream out(&testFile);
  out << "[test.args]\n"
         R"(test-arg = "test opt")";
  testFile.close();

  Config config(kTestFilename, "test");

  const auto result = config.load("test");

  QVERIFY(result);
  QCOMPARE(config.argc(), 3);
  QCOMPARE(config.argv()[0], "test");
  QCOMPARE(config.argv()[1], "--test-arg");
  QCOMPARE(config.argv()[2], "test opt");
}

void Config_Test::load_EmptyFile()
{
  Config config("", "test");
  QVERIFY_THROWS_EXCEPTION(Config::NoConfigFilenameError, config.load("test"));
}

void Config_Test::load_NonExsitingFile()
{
  Config config("nonexistent.toml", "test");
  const auto result = config.load("test");
  QVERIFY(!result);
}

void Config_Test::load_InvalidConfig()
{
  std::ofstream testFile(kTestFilename);
  testFile << "foobar";
  testFile.close();
  Config config(kTestFilename, "test");
  QVERIFY_THROWS_EXCEPTION(Config::ParseError, config.load("test"));
}

void Config_Test::load_missingSections()
{
  std::ofstream testFile(kTestFilename);
  testFile.close();
  Config config(kTestFilename, "missing");
  const auto result = config.load("test");
  QVERIFY(!result);
}

void Config_Test::load_badTable()
{
  std::ofstream testFile(kTestFilename);
  testFile << "[test]";
  testFile.close();
  Config config(kTestFilename, "test");
  const auto result = config.load("test");
  QVERIFY(!result);
}

void Config_Test::load_lastArg()
{
  std::ofstream testFile(kTestFilename);
  testFile << "[test.args]\n"
              R"(_last = "test last")"
              "\n"
              R"(test-second = true)";
  testFile.close();
  deskflow::Config config(kTestFilename, "test");

  const auto result = config.load("test-first");

  QVERIFY(result);
  QCOMPARE(config.argc(), 3);
  QCOMPARE(config.argv()[0], "test-first");
  QCOMPARE(config.argv()[1], "--test-second");
  QCOMPARE(config.argv()[2], "test last");
}

void Config_Test::load_noArgs()
{
  std::ofstream testFile(kTestFilename);
  testFile << "[test.args]";
  testFile.close();
  deskflow::Config config(kTestFilename, "test");
  const auto result = config.load("");
  QVERIFY(!result);
}

QTEST_MAIN(Config_Test)
