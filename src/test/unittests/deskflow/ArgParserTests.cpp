/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <array>
#include <functional>

#include "common/constants.h"
#include "deskflow/ArgParser.h"
#include "deskflow/ArgsBase.h"
#include "deskflow/ClientArgs.h"
#include "deskflow/ServerArgs.h"

#include <gtest/gtest.h>

TEST(ArgParserTests, isArg_abbreviationsArg_returnTrue)
{
  int i = 1;
  const int argc = 2;
  const char *argv[argc] = {"stub", "-t"};
  bool result = ArgParser::isArg(i, argc, argv, "-t", NULL);

  EXPECT_EQ(true, result);
}

TEST(ArgParserTests, isArg_fullArg_returnTrue)
{
  int i = 1;
  const int argc = 2;
  const char *argv[argc] = {"stub", "--test"};
  bool result = ArgParser::isArg(i, argc, argv, NULL, "--test");

  EXPECT_EQ(true, result);
}

TEST(ArgParserTests, isArg_missingArgs_returnFalse)
{
  int i = 1;
  const int argc = 2;
  const char *argv[argc] = {"stub", "-t"};
  static deskflow::ArgsBase argsBase;
  ArgParser argParser(NULL);
  argParser.setArgsBase(argsBase);

  bool result = ArgParser::isArg(i, argc, argv, "-t", NULL, 1);

  EXPECT_FALSE(result);
  EXPECT_EQ(true, argsBase.m_shouldExitFail);
}

TEST(ArgParserTests, searchDoubleQuotes_doubleQuotedArg_returnTrue)
{
  std::string command("\"stub\"");
  size_t left = 0;
  size_t right = 0;

  bool result = ArgParser::searchDoubleQuotes(command, left, right);

  EXPECT_EQ(true, result);
  EXPECT_EQ(0, left);
  EXPECT_EQ(5, right);
}

TEST(ArgParserTests, searchDoubleQuotes_noDoubleQuotedArg_returnfalse)
{
  std::string command("stub");
  size_t left = 0;
  size_t right = 0;

  bool result = ArgParser::searchDoubleQuotes(command, left, right);

  EXPECT_FALSE(result);
  EXPECT_EQ(0, left);
  EXPECT_EQ(0, right);
}

TEST(ArgParserTests, searchDoubleQuotes_oneDoubleQuoteArg_returnfalse)
{
  std::string command("\"stub");
  size_t left = 0;
  size_t right = 0;

  bool result = ArgParser::searchDoubleQuotes(command, left, right);

  EXPECT_FALSE(result);
  EXPECT_EQ(0, left);
  EXPECT_EQ(0, right);
}

TEST(ArgParserTests, splitCommandString_oneArg_returnArgv)
{
  std::string command("stub");
  std::vector<std::string> argv;

  ArgParser::splitCommandString(command, argv);

  EXPECT_EQ(1, argv.size());
  EXPECT_EQ("stub", argv.at(0));
}

TEST(ArgParserTests, splitCommandString_twoArgs_returnArgv)
{
  std::string command("stub1 stub2");
  std::vector<std::string> argv;

  ArgParser::splitCommandString(command, argv);

  EXPECT_EQ(2, argv.size());
  EXPECT_EQ("stub1", argv.at(0));
  EXPECT_EQ("stub2", argv.at(1));
}

TEST(ArgParserTests, splitCommandString_doubleQuotedArgs_returnArgv)
{
  std::string command("\"stub1\" stub2 \"stub3\"");
  std::vector<std::string> argv;

  ArgParser::splitCommandString(command, argv);

  EXPECT_EQ(3, argv.size());
  EXPECT_EQ("stub1", argv.at(0));
  EXPECT_EQ("stub2", argv.at(1));
  EXPECT_EQ("stub3", argv.at(2));
}

TEST(ArgParserTests, splitCommandString_spaceDoubleQuotedArgs_returnArgv)
{
  std::string command("\"stub1\" stub2 \"stub3 space\"");
  std::vector<std::string> argv;

  ArgParser::splitCommandString(command, argv);

  EXPECT_EQ(3, argv.size());
  EXPECT_EQ("stub1", argv.at(0));
  EXPECT_EQ("stub2", argv.at(1));
  EXPECT_EQ("stub3 space", argv.at(2));
}

TEST(ArgParserTests, getArgv_stringArray_return2DArray)
{
  std::vector<std::string> argArray;
  argArray.push_back("stub1");
  argArray.push_back("stub2");
  argArray.push_back("stub3 space");
  const char **argv = ArgParser::getArgv(argArray);

  std::string row1(argv[0]);
  std::string row2(argv[1]);
  std::string row3(argv[2]);

  EXPECT_EQ("stub1", row1);
  EXPECT_EQ("stub2", row2);
  EXPECT_EQ("stub3 space", row3);

  delete[] argv;
}

TEST(ArgParserTests, assembleCommand_stringArray_returnCommand)
{
  std::vector<std::string> argArray;
  argArray.push_back("stub1");
  argArray.push_back("stub2");
  std::string command = ArgParser::assembleCommand(argArray);

  EXPECT_EQ("stub1 stub2", command);
}

TEST(ArgParserTests, assembleCommand_ignoreSecondArg_returnCommand)
{
  std::vector<std::string> argArray;
  argArray.push_back("stub1");
  argArray.push_back("stub2");
  std::string command = ArgParser::assembleCommand(argArray, "stub2");

  EXPECT_EQ("stub1", command);
}

TEST(ArgParserTests, assembleCommand_ignoreSecondArgWithOneParameter_returnCommand)
{
  std::vector<std::string> argArray;
  argArray.push_back("stub1");
  argArray.push_back("stub2");
  argArray.push_back("stub3");
  argArray.push_back("stub4");
  std::string command = ArgParser::assembleCommand(argArray, "stub2", 1);

  EXPECT_EQ("stub1 stub4", command);
}

TEST(ArgParserTests, assembleCommand_stringArrayWithSpace_returnCommand)
{
  std::vector<std::string> argArray;
  argArray.push_back("stub1 space");
  argArray.push_back("stub2");
  argArray.push_back("stub3 space");
  std::string command = ArgParser::assembleCommand(argArray);

  EXPECT_EQ("\"stub1 space\" stub2 \"stub3 space\"", command);
}

TEST(ArgParserTests, parseServerArgs_parses_each_category)
{
  ArgParser parser(nullptr);
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
  EXPECT_TRUE(parser.parseServerArgs(args, sizeof(argv) / sizeof(argv[0]), argv));
  EXPECT_EQ(args.m_shouldExitOk, true);
}

TEST(ArgParserTests, parseClientArgs_parses_single_help)
{
  ArgParser parser(nullptr);
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
  EXPECT_TRUE(parser.parseClientArgs(args, sizeof(argv) / sizeof(argv[0]), argv));
  EXPECT_EQ(args.m_shouldExitOk, true);
}
