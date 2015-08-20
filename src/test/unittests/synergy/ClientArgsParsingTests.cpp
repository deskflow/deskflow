/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Synergy Si, Inc.
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
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library.
 * You must obey the GNU General Public License in all respects for all of
 * the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete
 * this exception statement from your version. If you delete this exception
 * statement from all source files in the program, then also delete it here.
 */

#include "synergy/ArgParser.h"
#include "synergy/ClientArgs.h"
#include "test/mock/synergy/MockArgParser.h"

#include "test/global/gtest.h"

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
	ClientArgs clientArgs;
	const int argc = 3;
	const char* kYScrollCmd[argc] = { "stub", "--yscroll", "1" };

	argParser.parseClientArgs(clientArgs, argc, kYScrollCmd);

	EXPECT_EQ(1, clientArgs.m_yscroll);
}

TEST(ClientArgsParsingTests, parseClientArgs_addressArg_setSynergyAddress)
{
	NiceMock<MockArgParser> argParser;
	ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(client_stubParseGenericArgs));
	ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(client_stubCheckUnexpectedArgs));
	ClientArgs clientArgs;
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
	ClientArgs clientArgs;
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
	ClientArgs clientArgs;
	const int argc = 3;
	const char* kUnrecognizedCmd[argc] = { "stub", "mock_arg", "mock_address"};

	bool result = argParser.parseClientArgs(clientArgs, argc, kUnrecognizedCmd);

	EXPECT_FALSE(result);
}
