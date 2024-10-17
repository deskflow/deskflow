/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "deskflow/ArgParser.h"
#include "deskflow/ServerArgs.h"
#include "test/mock/deskflow/MockArgParser.h"

#include <array>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;

bool server_stubParseGenericArgs(int, const char *const *, int &)
{
  return false;
}

bool server_stubCheckUnexpectedArgs()
{
  return false;
}

TEST(ServerArgs, ServerArgs_will_construct_from_copy)
{
  deskflow::ServerArgs serverArgs;
  serverArgs.m_display = "display0";
  deskflow::ServerArgs serverArgs2{serverArgs};
  EXPECT_EQ(serverArgs.m_display, serverArgs2.m_display);
}

TEST(ServerArgsParsingTests, parseServerArgs_addressArg_setDeskflowAddress)
{
  NiceMock<MockArgParser> argParser;
  ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(server_stubParseGenericArgs));
  ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(server_stubCheckUnexpectedArgs));
  deskflow::ServerArgs serverArgs;
  const int argc = 3;
  const char *kAddressCmd[argc] = {"stub", "--address", "mock_address"};

  argParser.parseServerArgs(serverArgs, argc, kAddressCmd);

  EXPECT_EQ("mock_address", serverArgs.m_deskflowAddress);
}

TEST(ServerArgsParsingTests, parseServerArgs_configArg_setConfigFile)
{
  NiceMock<MockArgParser> argParser;
  ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(server_stubParseGenericArgs));
  ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(server_stubCheckUnexpectedArgs));
  deskflow::ServerArgs serverArgs;
  const int argc = 3;
  const char *kConfigCmd[argc] = {"stub", "--config", "mock_configFile"};

  argParser.parseServerArgs(serverArgs, argc, kConfigCmd);

  EXPECT_EQ("mock_configFile", serverArgs.m_configFile);
}

TEST(ServerArgsParsingTests, parseServerArgs_checkUnexpectedParams)
{
  NiceMock<MockArgParser> argParser;
  ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(server_stubParseGenericArgs));
  ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(server_stubCheckUnexpectedArgs));
  deskflow::ServerArgs serverArgs;
  const int argc = 2;
  std::array<const char *, argc> kUnknownCmd = {"stub", "--unknown"};

  EXPECT_FALSE(argParser.parseServerArgs(serverArgs, argc, kUnknownCmd.data()));
}
