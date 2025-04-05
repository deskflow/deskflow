/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ArgParserTests.h"

#include "../../lib/deskflow/ArgsBase.h"
#include "../../lib/deskflow/ClientArgs.h"
#include "../../lib/deskflow/ServerArgs.h"

// This file is generated at build time
#include <common/Constants.h>

void ArgParserTests::initTestCase()
{
  m_arch.init();
  m_log.setFilter(kDEBUG2);
  static deskflow::ArgsBase base;
  m_parser.setArgsBase(base);
}

void ArgParserTests::isArg()
{
  int i = 1;
  const int argc = 2;
  const char *argShort[argc] = {"stub", "-t"};
  QVERIFY(ArgParser::isArg(i, argc, argShort, "-t", NULL));

  const char *argLong[argc] = {"stub", "--test"};
  QVERIFY(ArgParser::isArg(i, argc, argLong, NULL, "--test"));
}

void ArgParserTests::missingArg()
{
  int i = 1;
  const int argc = 2;
  const char *argv[argc] = {"stub", "-t"};
  static deskflow::ArgsBase argsBase;

  m_parser.setArgsBase(argsBase);

  QVERIFY(!ArgParser::isArg(i, argc, argv, "-t", NULL, 1));
  QVERIFY(argsBase.m_shouldExitFail);
}

void ArgParserTests::withQuotes()
{
  std::string command("\"stub\"");
  size_t left = 0;
  size_t right = 0;

  QVERIFY(ArgParser::searchDoubleQuotes(command, left, right));
  QCOMPARE(left, 0);
  QCOMPARE(right, 5);

  command = "stub";
  left = 0;
  right = 0;

  QVERIFY(!ArgParser::searchDoubleQuotes(command, left, right));
  QCOMPARE(0, left);
  QCOMPARE(0, right);

  command = "\"stub";
  left = 0;
  right = 0;

  QVERIFY(!ArgParser::searchDoubleQuotes(command, left, right));
  QCOMPARE(left, 0);
  QCOMPARE(right, 0);
}

void ArgParserTests::splitCommand()
{
  std::string command("stub");
  std::vector<std::string> argv;
  ArgParser::splitCommandString(command, argv);

  QCOMPARE(argv.size(), 1);
  QCOMPARE(argv.at(0), "stub");

  command = "stub1 stub2";
  argv.resize(0);
  ArgParser::splitCommandString(command, argv);

  QCOMPARE(2, argv.size());
  QCOMPARE(argv.at(0), "stub1");
  QCOMPARE(argv.at(1), "stub2");

  command = "stub1 stub2 stub3";
  argv.resize(0);
  ArgParser::splitCommandString(command, argv);

  QCOMPARE(3, argv.size());
  QCOMPARE(argv.at(0), "stub1");
  QCOMPARE(argv.at(1), "stub2");
  QCOMPARE(argv.at(2), "stub3");

  command = "\"stub1\" stub2 \"stub3 space\"";
  argv.resize(0);
  ArgParser::splitCommandString(command, argv);

  QCOMPARE(3, argv.size());
  QCOMPARE(argv.at(0), "stub1");
  QCOMPARE(argv.at(1), "stub2");
  QCOMPARE(argv.at(2), "stub3 space");
}

void ArgParserTests::getArgv()
{
  std::vector<std::string> argArray;
  argArray.push_back("stub1");
  argArray.push_back("stub2");
  argArray.push_back("stub3 space");
  const char **argv = ArgParser::getArgv(argArray);

  std::string row1(argv[0]);
  std::string row2(argv[1]);
  std::string row3(argv[2]);

  QCOMPARE(row1, "stub1");
  QCOMPARE(row2, "stub2");
  QCOMPARE(row3, "stub3 space");

  delete[] argv;
}

void ArgParserTests::assembleCommand()
{
  std::vector<std::string> argArray;
  argArray.push_back("stub1");
  argArray.push_back("stub2");
  std::string command = ArgParser::assembleCommand(argArray);

  QCOMPARE(command, "stub1 stub2");

  argArray.resize(0);
  argArray.push_back("stub1");
  argArray.push_back("stub2");
  command = ArgParser::assembleCommand(argArray, "stub2");

  QCOMPARE(command, "stub1");

  argArray.resize(0);
  argArray.push_back("stub1");
  argArray.push_back("stub2");
  argArray.push_back("stub3");
  argArray.push_back("stub4");
  command = ArgParser::assembleCommand(argArray, "stub2", 1);

  QCOMPARE(command, "stub1 stub4");

  argArray.resize(0);
  argArray.push_back("stub1 space");
  argArray.push_back("stub2");
  argArray.push_back("stub3 space");
  command = ArgParser::assembleCommand(argArray);

  QCOMPARE(command, "\"stub1 space\" stub2 \"stub3 space\"");
}

void ArgParserTests::server_setAddress()
{
  deskflow::ServerArgs serverArgs;
  const int argc = 3;
  const char *kAddressCmd[argc] = {"stub", "--address", "mock_address"};

  m_parser.parseServerArgs(serverArgs, argc, kAddressCmd);

  QCOMPARE(serverArgs.m_deskflowAddress, "mock_address");
}

void ArgParserTests::server_setConfigFile()
{
  deskflow::ServerArgs serverArgs;
  const int argc = 3;
  const char *kConfigCmd[argc] = {"stub", "--config", "mock_configFile"};

  m_parser.parseServerArgs(serverArgs, argc, kConfigCmd);

  QCOMPARE(serverArgs.m_configFile, "mock_configFile");
}

void ArgParserTests::server_unexpectedParam()
{
  deskflow::ServerArgs serverArgs;
  const int argc = 2;
  std::array<const char *, argc> kUnknownCmd = {"stub", "--unknown"};

  QVERIFY(!m_parser.parseServerArgs(serverArgs, argc, kUnknownCmd.data()));
}

void ArgParserTests::serverArgs()
{
  deskflow::ServerArgs args;
  args.m_daemon = false;
  char const *argv[] = {
      "deskflow", "--help"
#if WINAPI_XWINDOWS
      ,
      "--no-xinitthreads"
#endif
      ,
      "--res-w", "888"
  };

  QVERIFY(m_parser.parseServerArgs(args, sizeof(argv) / sizeof(argv[0]), argv));
  QVERIFY(args.m_shouldExitOk);
}

void ArgParserTests::clientArgs()
{
  deskflow::ClientArgs args;
  args.m_daemon = false;
  char const *argv[] = {
      kAppId,
      "--help"
#if WINAPI_XWINDOWS
      ,
      "--no-xinitthreads"
#endif
      ,
      "--res-w",
      "888",
      "127.0.0.1"
  };

  QVERIFY(m_parser.parseClientArgs(args, sizeof(argv) / sizeof(argv[0]), argv));
  QVERIFY(args.m_shouldExitOk);
}

void ArgParserTests::client_yScroll()
{
  deskflow::ClientArgs clientArgs;
  const int argc = 3;
  const char *kYScrollCmd[argc] = {"stub", "--yscroll", "1"};

  m_parser.parseClientArgs(clientArgs, argc, kYScrollCmd);

  QCOMPARE(clientArgs.m_yscroll, 1);
}

void ArgParserTests::client_setLangSync()
{
  deskflow::ClientArgs clientArgs;
  clientArgs.m_enableLangSync = false;
  const int argc = 2;
  std::array<const char *, argc> kLangCmd = {"stub", "--sync-language"};

  m_parser.parseClientArgs(clientArgs, argc, kLangCmd.data());

  QVERIFY(clientArgs.m_enableLangSync);
}

void ArgParserTests::client_setInvertScroll()
{
  deskflow::ClientArgs clientArgs;
  const int argc = 2;
  std::array<const char *, argc> kLangCmd = {"stub", "--invert-scroll"};

  m_parser.parseClientArgs(clientArgs, argc, kLangCmd.data());

  QCOMPARE(clientArgs.m_clientScrollDirection, deskflow::ClientScrollDirection::INVERT_SERVER);
}

void ArgParserTests::client_commonArgs()
{
  deskflow::ClientArgs clientArgs;
  clientArgs.m_enableLangSync = false;
  const int argc = 5;
  std::array<const char *, argc> kLangCmd = {"stub", "--enable-crypto", "--tls-cert", "tlsCertPath", "--prevent-sleep"};

  m_parser.parseClientArgs(clientArgs, argc, kLangCmd.data());

  QVERIFY(clientArgs.m_enableCrypto);
  QVERIFY(clientArgs.m_preventSleep);
  QCOMPARE(clientArgs.m_tlsCertFile, "tlsCertPath");
}

void ArgParserTests::client_setAddress()
{
  deskflow::ClientArgs clientArgs;
  const int argc = 2;
  const char *kAddressCmd[argc] = {"stub", "mock_address"};

  QVERIFY(m_parser.parseClientArgs(clientArgs, argc, kAddressCmd));
  QCOMPARE(clientArgs.m_serverAddress, "mock_address");
}

void ArgParserTests::client_badArgs()
{
  deskflow::ClientArgs clientArgs;
  const int argc = 1;
  const char *kNoAddressCmd[argc] = {"stub"};
  QVERIFY(!m_parser.parseClientArgs(clientArgs, argc, kNoAddressCmd));

  const int argc2 = 3;
  const char *kUnrecognizedCmd[argc2] = {"stub", "mock_arg", "mock_address"};
  QVERIFY(!m_parser.parseClientArgs(clientArgs, argc2, kUnrecognizedCmd));
}

void ArgParserTests::deprecatedArg_crypoPass_true()
{
  int i = 1;
  const int argc = 3;
  const char *kCryptoPassCmd[argc] = {"stub", "--crypto-pass", "mock_pass"};

  QVERIFY(m_parser.parseDeprecatedArgs(argc, kCryptoPassCmd, i));
  QCOMPARE(i, 2);
}

void ArgParserTests::deprecatedArg_crypoPass_false()
{
  int i = 1;
  const int argc = 3;
  const char *kCryptoPassCmd[argc] = {"stub", "--mock-arg", "mock_value"};

  QVERIFY(!m_parser.parseDeprecatedArgs(argc, kCryptoPassCmd, i));
  QCOMPARE(i, 1);
}

void ArgParserTests::generic_logLevel()
{
  int i = 1;
  const int argc = 3;
  const char *kLogLevelCmd[argc] = {"stub", "--debug", "DEBUG"};

  m_parser.parseGenericArgs(argc, kLogLevelCmd, i);

  QCOMPARE(m_parser.argsBase().m_logFilter, "DEBUG");
  QCOMPARE(i, 2);
}

void ArgParserTests::generic_logFile()
{
  int i = 1;
  const int argc = 3;
  const char *kLogFileCmd[argc] = {"stub", "--log", "mock_filename"};

  m_parser.parseGenericArgs(argc, kLogFileCmd, i);

  QCOMPARE(m_parser.argsBase().m_logFile, "mock_filename");
  QCOMPARE(i, 2);
}

void ArgParserTests::generic_logFileWithSpace()
{
  int i = 1;
  const int argc = 3;
  const char *kLogFileCmdWithSpace[argc] = {"stub", "--log", "mo ck_filename"};

  m_parser.parseGenericArgs(argc, kLogFileCmdWithSpace, i);

  QCOMPARE(m_parser.argsBase().m_logFile, "mo ck_filename");
  QCOMPARE(i, 2);
}

void ArgParserTests::generic_foreground()
{
  int i = 1;
  const int argc = 2;
  const char *kNoDeamonCmd[argc] = {"stub", "-f"};

  m_parser.parseGenericArgs(argc, kNoDeamonCmd, i);

  QVERIFY(!m_parser.argsBase().m_daemon);
  QCOMPARE(i, 1);
}

void ArgParserTests::generic_daemon()
{
  int i = 1;
  const int argc = 2;
  const char *kDeamonCmd[argc] = {"stub", "--daemon"};

  m_parser.parseGenericArgs(argc, kDeamonCmd, i);

  QVERIFY(m_parser.argsBase().m_daemon);
  QCOMPARE(i, 1);
}

void ArgParserTests::generic_name()
{
  int i = 1;
  const int argc = 3;
  const char *kNameCmd[argc] = {"stub", "--name", "mock"};
  // Somehow cause a dump if not made here.
  ArgParser parser(nullptr);
  deskflow::ArgsBase base;

  parser.setArgsBase(base);
  parser.parseGenericArgs(argc, kNameCmd, i);

  QCOMPARE(base.m_name, "mock");
  QCOMPARE(i, 2);
}

void ArgParserTests::generic_noRestart()
{
  int i = 1;
  const int argc = 2;
  const char *kNoRestartCmd[argc] = {"stub", "--no-restart"};

  m_parser.parseGenericArgs(argc, kNoRestartCmd, i);

  QVERIFY(!m_parser.argsBase().m_restartable);
  QCOMPARE(i, 1);
}

void ArgParserTests::generic_restart()
{
  int i = 1;
  const int argc = 2;
  const char *kRestartCmd[argc] = {"stub", "--restart"};

  m_parser.parseGenericArgs(argc, kRestartCmd, i);

  QVERIFY(m_parser.argsBase().m_restartable);
  QCOMPARE(i, 1);
}

void ArgParserTests::generic_unknown()
{
  int i = 1;
  const int argc = 2;
  const char *kBackendCmd[argc] = {"stub", "-z"};

  QVERIFY(!m_parser.parseGenericArgs(argc, kBackendCmd, i));
}

void ArgParserTests::generic_noHook()
{
  int i = 1;
  const int argc = 2;
  const char *kNoHookCmd[argc] = {"stub", "--no-hooks"};

  m_parser.parseGenericArgs(argc, kNoHookCmd, i);

  QVERIFY(m_parser.argsBase().m_noHooks);
  QCOMPARE(i, 1);
}

QTEST_MAIN(ArgParserTests)
