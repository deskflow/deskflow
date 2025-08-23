/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoreArgParserTests.h"
#include "common/Settings.h"
#include "deskflow/CoreArgParser.h"

void CoreArgParserTests::initTestCase()
{
  QDir dir;
  QVERIFY(dir.mkpath(m_settingsPath));

  QFile oldSettings(m_settingsFile);
  if (oldSettings.exists())
    oldSettings.remove();

  Settings::setSettingFile(m_settingsFile);
}

void CoreArgParserTests::interfaceLong()
{
  QStringList args = {"stub", "client", "--interface", "mock_address"};

  CoreArgParser parser(args);
  parser.parse();

  QCOMPARE(Settings::value(Settings::Core::Interface).toString(), "mock_address");
}

void CoreArgParserTests::interfaceShort()
{
  QStringList args = {"stub", "client", "-i", "address"};

  CoreArgParser parser(args);
  parser.parse();

  QCOMPARE(Settings::value(Settings::Core::Interface).toString(), "address");
}

void CoreArgParserTests::portLong()
{
  QStringList args = {"stub", "client", "--port", "28771"};

  CoreArgParser parser(args);
  parser.parse();

  QCOMPARE(Settings::value(Settings::Core::Port).toInt(), 28771);
}

void CoreArgParserTests::portShort()
{
  QStringList args = {"stub", "client", "--p", "18768"};

  CoreArgParser parser(args);
  parser.parse();

  QCOMPARE(Settings::value(Settings::Core::Port).toInt(), 18768);
}

void CoreArgParserTests::nameLong()
{
  QStringList args = {"stub", "client", "--name", "FancyName"};

  CoreArgParser parser(args);
  parser.parse();

  QCOMPARE(Settings::value(Settings::Core::ScreenName).toString(), "FancyName");
}

void CoreArgParserTests::nameShort()
{
  QStringList args = {"stub", "client", "-n", "ShortName"};

  CoreArgParser parser(args);
  parser.parse();

  QCOMPARE(Settings::value(Settings::Core::ScreenName).toString(), "ShortName");
}

void CoreArgParserTests::logLevel()
{
  QStringList args = {"stub", "client", "--log-level", "DEBUG1"};

  CoreArgParser parser(args);
  parser.parse();

  QCOMPARE(Settings::value(Settings::Log::Level).toInt(), 6);
}

void CoreArgParserTests::logFile()
{
  QStringList args = {"stub", "client", "--log", "mock_filename"};

  CoreArgParser parser(args);
  parser.parse();

  QCOMPARE(Settings::value(Settings::Log::File).toString(), "mock_filename");
}

void CoreArgParserTests::logFileWithSpace()
{
  QStringList args = {"stub", "client", "--log", "mock filename"};

  CoreArgParser parser(args);
  parser.parse();

  QCOMPARE(Settings::value(Settings::Log::File).toString(), "mock filename");
}

QTEST_MAIN(CoreArgParserTests)
