/*
 * synergy -- mouse and keyboard sharing utility
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

#include "synergy/ArgParser.h"
#include "synergy/ClientArgs.h"
#include "test/mock/synergy/MockArgParser.h"

#include "test/global/gtest.h"

#include <array>

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;

bool
client_stubParseGenericArgs(int, const char* const*, int&)
{
    return false;
}

bool
client_stubCheckUnexpectedArgs()
{
    return false;
}

TEST(ClientArgsParsingTests, parseClientArgs_yScrollArg_setYScroll)
{
    NiceMock<MockArgParser> argParser;
    ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(client_stubParseGenericArgs));
    ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(client_stubCheckUnexpectedArgs));
    lib::synergy::ClientArgs clientArgs;
    const int argc = 3;
    const char* kYScrollCmd[argc] = { "stub", "--yscroll", "1" };

    argParser.parseClientArgs(clientArgs, argc, kYScrollCmd);

    EXPECT_EQ(1, clientArgs.m_yscroll);
}

TEST(ClientArgsParsingTests, parseClientArgs_setLangSync)
{
    NiceMock<MockArgParser> argParser;
    ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(client_stubParseGenericArgs));
    ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(client_stubCheckUnexpectedArgs));
    lib::synergy::ClientArgs clientArgs;
    clientArgs.m_enableLangSync = false;
    const int argc = 2;
    std::array<const char*, argc> kLangCmd = { "stub", "--sync-language" };

    argParser.parseClientArgs(clientArgs, argc, kLangCmd.data());

    EXPECT_TRUE(clientArgs.m_enableLangSync);
}

TEST(ClientArgsParsingTests, parseClientArgs_setInvertScroll)
{
    NiceMock<MockArgParser> argParser;
    lib::synergy::ClientArgs clientArgs;
    const int argc = 2;
    std::array<const char*, argc> kLangCmd = { "stub", "--invert-scroll" };

    argParser.parseClientArgs(clientArgs, argc, kLangCmd.data());
    EXPECT_EQ(clientArgs.m_clientScrollDirection, lib::synergy::ClientScrollDirection::INVERT_SERVER);
}

TEST(ClientArgsParsingTests, parseClientArgs_setCommonArgs)
{
    NiceMock<MockArgParser> argParser;
    ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(client_stubParseGenericArgs));
    ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(client_stubCheckUnexpectedArgs));
    lib::synergy::ClientArgs clientArgs;
    clientArgs.m_enableLangSync = false;
    const int argc = 9;
    std::array<const char*, argc> kLangCmd = { "stub", "--enable-crypto", "--profile-dir", "profileDir",
                                               "--plugin-dir", "pluginDir", "--tls-cert", "tlsCertPath",
                                               "--prevent-sleep" };

    argParser.parseClientArgs(clientArgs, argc, kLangCmd.data());

    EXPECT_TRUE(clientArgs.m_enableCrypto);
    EXPECT_EQ(clientArgs.m_profileDirectory, "profileDir");
    EXPECT_EQ(clientArgs.m_pluginDirectory, "pluginDir");
    EXPECT_EQ(clientArgs.m_tlsCertFile, "tlsCertPath");
    EXPECT_TRUE(clientArgs.m_preventSleep);
}

TEST(ClientArgsParsingTests, parseClientArgs_addressArg_setSynergyAddress)
{
    NiceMock<MockArgParser> argParser;
    ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(client_stubParseGenericArgs));
    ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(client_stubCheckUnexpectedArgs));
    lib::synergy::ClientArgs clientArgs;
    const int argc = 2;
    const char* kAddressCmd[argc] = { "stub", "mock_address" };

    bool result = argParser.parseClientArgs(clientArgs, argc, kAddressCmd);

    EXPECT_EQ("mock_address", clientArgs.m_synergyAddress);
    EXPECT_EQ(true, result);
}

TEST(ClientArgsParsingTests, parseClientArgs_noAddressArg_returnFalse)
{
    NiceMock<MockArgParser> argParser;
    ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(client_stubParseGenericArgs));
    ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(client_stubCheckUnexpectedArgs));
    lib::synergy::ClientArgs clientArgs;
    const int argc = 1;
    const char* kNoAddressCmd[argc] = { "stub" };

    bool result = argParser.parseClientArgs(clientArgs, argc, kNoAddressCmd);

    EXPECT_FALSE(result);
}

TEST(ClientArgsParsingTests, parseClientArgs_unrecognizedArg_returnFalse)
{
    NiceMock<MockArgParser> argParser;
    ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(client_stubParseGenericArgs));
    ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(client_stubCheckUnexpectedArgs));
    lib::synergy::ClientArgs clientArgs;
    const int argc = 3;
    const char* kUnrecognizedCmd[argc] = { "stub", "mock_arg", "mock_address"};

    bool result = argParser.parseClientArgs(clientArgs, argc, kUnrecognizedCmd);

    EXPECT_FALSE(result);
}
