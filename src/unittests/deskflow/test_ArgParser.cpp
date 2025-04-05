/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

#include "../../lib/deskflow/ArgsBase.h"
#include "../../lib/deskflow/ClientArgs.h"
#include "../../lib/deskflow/ServerArgs.h"
#include <common/Constants.h>

#include "test_ArgParser.h"

void ArgParser_Test::initTestCase()
{
  m_arch.init();
  m_log.setFilter(kDEBUG2);
}

void ArgParser_Test::isArg()
{
  int i = 1;
  const int argc = 2;
  const char *argShort[argc] = {"stub", "-t"};
  QVERIFY(ArgParser::isArg(i, argc, argShort, "-t", NULL));

  const char *argLong[argc] = {"stub", "--test"};
  QVERIFY(ArgParser::isArg(i, argc, argLong, NULL, "--test"));
}

void ArgParser_Test::missingArg()
{
  int i = 1;
  const int argc = 2;
  const char *argv[argc] = {"stub", "-t"};
  static deskflow::ArgsBase argsBase;
  m_parser.setArgsBase(argsBase);
  QVERIFY(!ArgParser::isArg(i, argc, argv, "-t", NULL, 1));
  QVERIFY(argsBase.m_shouldExitFail);
}

void ArgParser_Test::withQuotes()
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

void ArgParser_Test::splitCommand()
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

void ArgParser_Test::getArgv()
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

void ArgParser_Test::assembleCommand()
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

void ArgParser_Test::serverArgs()
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

void ArgParser_Test::clientArgs()
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

QTEST_MAIN(ArgParser_Test)
