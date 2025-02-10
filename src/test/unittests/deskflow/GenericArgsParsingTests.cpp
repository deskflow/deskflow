/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ArgParser.h"
#include "deskflow/ArgsBase.h"
#include "test/mock/deskflow/MockApp.h"

#include <gtest/gtest.h>

using namespace deskflow;
using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;

bool g_helpShowed = false;
bool g_versionShowed = false;

void showMockHelp()
{
  g_helpShowed = true;
}

void showMockVersion()
{
  g_versionShowed = true;
}

class GenericArgsParsingTests : public ::testing::Test
{
public:
  void SetUp()
  {
    m_argParser = new ArgParser(nullptr);
    m_argParser->setArgsBase(argsBase);
  }

  void TearDown()
  {
    delete m_argParser;
  }

  static deskflow::ArgsBase argsBase;
  ArgParser *m_argParser = nullptr;
};

deskflow::ArgsBase GenericArgsParsingTests::argsBase;

TEST_F(GenericArgsParsingTests, parseGenericArgs_logLevelCmd_setLogLevel)
{
  int i = 1;
  const int argc = 3;
  const char *kLogLevelCmd[argc] = {"stub", "--debug", "DEBUG"};

  m_argParser->parseGenericArgs(argc, kLogLevelCmd, i);
  std::string logFilter(argsBase.m_logFilter);

  EXPECT_EQ("DEBUG", logFilter);
  EXPECT_EQ(2, i);
}

TEST_F(GenericArgsParsingTests, parseGenericArgs_logFileCmd_saveLogFilename)
{
  int i = 1;
  const int argc = 3;
  const char *kLogFileCmd[argc] = {"stub", "--log", "mock_filename"};

  m_argParser->parseGenericArgs(argc, kLogFileCmd, i);
  std::string logFile(argsBase.m_logFile);

  EXPECT_EQ("mock_filename", logFile);
  EXPECT_EQ(2, i);
}

TEST_F(GenericArgsParsingTests, parseGenericArgs_logFileCmdWithSpace_saveLogFilename)
{
  int i = 1;
  const int argc = 3;
  const char *kLogFileCmdWithSpace[argc] = {"stub", "--log", "mo ck_filename"};

  m_argParser->parseGenericArgs(argc, kLogFileCmdWithSpace, i);
  std::string logFile(argsBase.m_logFile);

  EXPECT_EQ("mo ck_filename", logFile);
  EXPECT_EQ(2, i);
}

TEST_F(GenericArgsParsingTests, parseGenericArgs_noDeamonCmd_daemonFalse)
{
  int i = 1;
  const int argc = 2;
  const char *kNoDeamonCmd[argc] = {"stub", "-f"};

  m_argParser->parseGenericArgs(argc, kNoDeamonCmd, i);

  EXPECT_FALSE(argsBase.m_daemon);
  EXPECT_EQ(1, i);
}

TEST_F(GenericArgsParsingTests, parseGenericArgs_deamonCmd_daemonTrue)
{
  int i = 1;
  const int argc = 2;
  const char *kDeamonCmd[argc] = {"stub", "--daemon"};

  m_argParser->parseGenericArgs(argc, kDeamonCmd, i);

  EXPECT_EQ(true, argsBase.m_daemon);
  EXPECT_EQ(1, i);
}

TEST_F(GenericArgsParsingTests, parseGenericArgs_nameCmd_saveName)
{
  int i = 1;
  const int argc = 3;
  const char *kNameCmd[argc] = {"stub", "--name", "mock"};

  m_argParser->parseGenericArgs(argc, kNameCmd, i);

  EXPECT_EQ("mock", argsBase.m_name);
  EXPECT_EQ(2, i);
}

TEST_F(GenericArgsParsingTests, parseGenericArgs_noRestartCmd_restartFalse)
{
  int i = 1;
  const int argc = 2;
  const char *kNoRestartCmd[argc] = {"stub", "--no-restart"};

  m_argParser->parseGenericArgs(argc, kNoRestartCmd, i);

  EXPECT_FALSE(argsBase.m_restartable);
  EXPECT_EQ(1, i);
}

TEST_F(GenericArgsParsingTests, parseGenericArgs_restartCmd_restartTrue)
{
  int i = 1;
  const int argc = 2;
  const char *kRestartCmd[argc] = {"stub", "--restart"};

  m_argParser->parseGenericArgs(argc, kRestartCmd, i);

  EXPECT_EQ(true, argsBase.m_restartable);
  EXPECT_EQ(1, i);
}

TEST_F(GenericArgsParsingTests, parseGenericArgs_backendCmd_rejected)
{
  int i = 1;
  const int argc = 2;
  const char *kBackendCmd[argc] = {"stub", "-z"};

  EXPECT_FALSE(m_argParser->parseGenericArgs(argc, kBackendCmd, i));
}

TEST_F(GenericArgsParsingTests, parseGenericArgs_noHookCmd_noHookTrue)
{
  int i = 1;
  const int argc = 2;
  const char *kNoHookCmd[argc] = {"stub", "--no-hooks"};

  m_argParser->parseGenericArgs(argc, kNoHookCmd, i);

  EXPECT_EQ(true, argsBase.m_noHooks);
  EXPECT_EQ(1, i);
}

TEST_F(GenericArgsParsingTests, parseGenericArgs_helpCmd_showHelp)
{
  g_helpShowed = false;
  int i = 1;
  const int argc = 2;
  const char *kHelpCmd[argc] = {"stub", "--help"};

  NiceMock<MockApp> app;
  ArgParser argParser(&app);
  argParser.setArgsBase(argsBase);
  ON_CALL(app, help()).WillByDefault(Invoke(showMockHelp));

  argParser.parseGenericArgs(argc, kHelpCmd, i);

  EXPECT_EQ(true, g_helpShowed);
  EXPECT_EQ(1, i);
}

TEST_F(GenericArgsParsingTests, parseGenericArgs_versionCmd_showVersion)
{
  g_versionShowed = false;
  int i = 1;
  const int argc = 2;
  const char *kVersionCmd[argc] = {"stub", "--version"};

  NiceMock<MockApp> app;
  ArgParser argParser(&app);
  argParser.setArgsBase(argsBase);

  ON_CALL(app, version()).WillByDefault(Invoke(showMockVersion));

  argParser.parseGenericArgs(argc, kVersionCmd, i);

  EXPECT_EQ(true, g_versionShowed);
  EXPECT_EQ(1, i);
}

#ifndef WINAPI_XWINDOWS
TEST_F(GenericArgsParsingTests, parseGenericArgs_dragDropCmdOnNonLinux_enableDragDropTrue)
{
  int i = 1;
  const int argc = 2;
  const char *kDragDropCmd[argc] = {"stub", "--enable-drag-drop"};

  m_argParser->parseGenericArgs(argc, kDragDropCmd, i);

  EXPECT_EQ(true, argsBase.m_enableDragDrop);
  EXPECT_EQ(1, i);
}
#endif

#ifdef WINAPI_XWINDOWS
TEST_F(GenericArgsParsingTests, parseGenericArgs_dragDropCmdOnLinux_enableDragDropFalse)
{
  int i = 1;
  const int argc = 2;
  const char *kDragDropCmd[argc] = {"stub", "--enable-drag-drop"};

  m_argParser->parseGenericArgs(argc, kDragDropCmd, i);

  EXPECT_FALSE(argsBase.m_enableDragDrop);
  EXPECT_EQ(1, i);
}
#endif
