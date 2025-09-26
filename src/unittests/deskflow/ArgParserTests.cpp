/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ArgParserTests.h"

#include "deskflow/ArgsBase.h"

// This file is generated at build time
#include <common/Constants.h>

void ArgParserTests::initTestCase()
{
  m_arch.init();
  m_log.setFilter(LogLevel::Debug2);
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

QTEST_MAIN(ArgParserTests)
