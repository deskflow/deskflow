/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ArgParser.h"
#include "deskflow/ClientArgs.h"
#include "test/mock/deskflow/MockArgParser.h"

#include <gtest/gtest.h>

#include <array>

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;

bool client_stubParseGenericArgs(int, const char *const *, int &)
{
  return false;
}

bool client_stubCheckUnexpectedArgs()
{
  return false;
}

TEST(ClientArgsParsingTests, parseClientArgs_yScrollArg_setYScroll)
{
  NiceMock<MockArgParser> argParser;
  ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(client_stubParseGenericArgs));
  ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(client_stubCheckUnexpectedArgs));
  deskflow::ClientArgs clientArgs;
  const int argc = 3;
  const char *kYScrollCmd[argc] = {"stub", "--yscroll", "1"};

  argParser.parseClientArgs(clientArgs, argc, kYScrollCmd);

  EXPECT_EQ(1, clientArgs.m_yscroll);
}

TEST(ClientArgsParsingTests, parseClientArgs_setLangSync)
{
  NiceMock<MockArgParser> argParser;
  ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(client_stubParseGenericArgs));
  ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(client_stubCheckUnexpectedArgs));
  deskflow::ClientArgs clientArgs;
  clientArgs.m_enableLangSync = false;
  const int argc = 2;
  std::array<const char *, argc> kLangCmd = {"stub", "--sync-language"};

  argParser.parseClientArgs(clientArgs, argc, kLangCmd.data());

  EXPECT_TRUE(clientArgs.m_enableLangSync);
}

TEST(ClientArgsParsingTests, parseClientArgs_setInvertScroll)
{
  NiceMock<MockArgParser> argParser;
  deskflow::ClientArgs clientArgs;
  const int argc = 2;
  std::array<const char *, argc> kLangCmd = {"stub", "--invert-scroll"};

  argParser.parseClientArgs(clientArgs, argc, kLangCmd.data());
  EXPECT_EQ(clientArgs.m_clientScrollDirection, deskflow::ClientScrollDirection::INVERT_SERVER);
}

TEST(ClientArgsParsingTests, parseClientArgs_setCommonArgs)
{
  NiceMock<MockArgParser> argParser;
  ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(client_stubParseGenericArgs));
  ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(client_stubCheckUnexpectedArgs));
  deskflow::ClientArgs clientArgs;
  clientArgs.m_enableLangSync = false;
  const int argc = 9;
  std::array<const char *, argc> kLangCmd = {"stub",       "--enable-crypto", "--profile-dir",
                                             "profileDir", "--plugin-dir",    "pluginDir",
                                             "--tls-cert", "tlsCertPath",     "--prevent-sleep"};

  argParser.parseClientArgs(clientArgs, argc, kLangCmd.data());

  EXPECT_TRUE(clientArgs.m_enableCrypto);
  EXPECT_EQ(clientArgs.m_profileDirectory, "profileDir");
  EXPECT_EQ(clientArgs.m_pluginDirectory, "pluginDir");
  EXPECT_EQ(clientArgs.m_tlsCertFile, "tlsCertPath");
  EXPECT_TRUE(clientArgs.m_preventSleep);
}

TEST(ClientArgsParsingTests, parseClientArgs_addressArg_setDeskflowAddress)
{
  NiceMock<MockArgParser> argParser;
  ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(client_stubParseGenericArgs));
  ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(client_stubCheckUnexpectedArgs));
  deskflow::ClientArgs clientArgs;
  const int argc = 2;
  const char *kAddressCmd[argc] = {"stub", "mock_address"};

  bool result = argParser.parseClientArgs(clientArgs, argc, kAddressCmd);

  EXPECT_EQ("mock_address", clientArgs.m_serverAddress);
  EXPECT_EQ(true, result);
}

TEST(ClientArgsParsingTests, parseClientArgs_noAddressArg_returnFalse)
{
  NiceMock<MockArgParser> argParser;
  ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(client_stubParseGenericArgs));
  ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(client_stubCheckUnexpectedArgs));
  deskflow::ClientArgs clientArgs;
  const int argc = 1;
  const char *kNoAddressCmd[argc] = {"stub"};

  bool result = argParser.parseClientArgs(clientArgs, argc, kNoAddressCmd);

  EXPECT_FALSE(result);
}

TEST(ClientArgsParsingTests, parseClientArgs_unrecognizedArg_returnFalse)
{
  NiceMock<MockArgParser> argParser;
  ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(client_stubParseGenericArgs));
  ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(client_stubCheckUnexpectedArgs));
  deskflow::ClientArgs clientArgs;
  const int argc = 3;
  const char *kUnrecognizedCmd[argc] = {"stub", "mock_arg", "mock_address"};

  bool result = argParser.parseClientArgs(clientArgs, argc, kUnrecognizedCmd);

  EXPECT_FALSE(result);
}
