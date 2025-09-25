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

void CoreArgParserTests::secure_false()
{
  QStringList args = {"stub", "client", "--secure", "false"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(!Settings::value(Settings::Security::TlsEnabled).toBool());
}

void CoreArgParserTests::secure_true()
{
  QStringList args = {"stub", "client", "--secure", "true"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(Settings::value(Settings::Security::TlsEnabled).toBool());
}

void CoreArgParserTests::secure_0()
{
  QStringList args = {"stub", "client", "--secure", "0"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(!Settings::value(Settings::Security::TlsEnabled).toBool());
}

void CoreArgParserTests::secure_1()
{
  QStringList args = {"stub", "client", "--secure", "1"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(Settings::value(Settings::Security::TlsEnabled).toBool());
}

void CoreArgParserTests::tlsCert()
{
  QStringList args = {"stub", "client", "--tls-cert", "certFile"};

  CoreArgParser parser(args);
  parser.parse();

  QCOMPARE(Settings::value(Settings::Security::Certificate).toString(), "certFile");
}

void CoreArgParserTests::preventSleep_false()
{
  QStringList args = {"stub", "client", "--prevent-sleep", "false"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(!Settings::value(Settings::Core::PreventSleep).toBool());
}

void CoreArgParserTests::preventSleep_1()
{
  QStringList args = {"stub", "client", "--prevent-sleep", "1"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(Settings::value(Settings::Core::PreventSleep).toBool());
}

void CoreArgParserTests::preventSleep_0()
{
  QStringList args = {"stub", "client", "--prevent-sleep", "0"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(!Settings::value(Settings::Core::PreventSleep).toBool());
}

void CoreArgParserTests::restartLongOption_0()
{
  QStringList args = {"stub", "client", "--restartOnFailure", "0"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(!Settings::value(Settings::Core::RestartOnFailure).toBool());
}

void CoreArgParserTests::restartLongOption_1()
{
  QStringList args = {"stub", "client", "--restartOnFailure", "1"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(Settings::value(Settings::Core::RestartOnFailure).toBool());
}

void CoreArgParserTests::restartLongOption_false()
{
  QStringList args = {"stub", "client", "--restartOnFailure", "false"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(!Settings::value(Settings::Core::RestartOnFailure).toBool());
}

void CoreArgParserTests::restartLongOption_true()
{
  QStringList args = {"stub", "client", "--restartOnFailure", "true"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(Settings::value(Settings::Core::RestartOnFailure).toBool());
}

void CoreArgParserTests::restartShortOption_0()
{
  QStringList args = {"stub", "client", "-r", "0"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(!Settings::value(Settings::Core::RestartOnFailure).toBool());
}

void CoreArgParserTests::restartShortOption_1()
{
  QStringList args = {"stub", "client", "-r", "1"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(Settings::value(Settings::Core::RestartOnFailure).toBool());
}

void CoreArgParserTests::restartShortOption_false()
{
  QStringList args = {"stub", "client", "-r", "false"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(!Settings::value(Settings::Core::RestartOnFailure).toBool());
}

void CoreArgParserTests::restartShortOption_true()
{
  QStringList args = {"stub", "client", "-r", "true"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(Settings::value(Settings::Core::RestartOnFailure).toBool());
}

void CoreArgParserTests::hookOptions_false()
{
  QStringList args = {"stub", "client", "--useHooks", "false"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(!Settings::value(Settings::Core::UseHooks).toBool());
}

void CoreArgParserTests::hookOptions_true()
{
  QStringList args = {"stub", "client", "--useHooks", "true"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(Settings::value(Settings::Core::UseHooks).toBool());
}

void CoreArgParserTests::server_peerCheck_false()
{
  QStringList args = {"stub", "server", "--peerCertCheck", "false"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(!Settings::value(Settings::Security::CheckPeers).toBool());
}

void CoreArgParserTests::server_peerCheck_true()
{
  QStringList args = {"stub", "server", "--peerCertCheck", "true"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(Settings::value(Settings::Security::CheckPeers).toBool());
}

void CoreArgParserTests::server_setConfig()
{
  QStringList args = {"stub", "server", "--serverConfig", "afile.conf"};

  CoreArgParser parser(args);
  parser.parse();

  QCOMPARE("afile.conf", Settings::value(Settings::Server::ExternalConfigFile).toString());
}

void CoreArgParserTests::client_yscroll()
{
  QStringList args = {"stub", "client", "--yscroll", "15"};

  CoreArgParser parser(args);
  parser.parse();

  QCOMPARE(15, Settings::value(Settings::Client::ScrollSpeed).toInt());
}

void CoreArgParserTests::preventSleep_true()
{
  QStringList args = {"stub", "client", "--prevent-sleep", "true"};

  CoreArgParser parser(args);
  parser.parse();

  QVERIFY(Settings::value(Settings::Core::PreventSleep).toBool());
}

QTEST_MAIN(CoreArgParserTests)
