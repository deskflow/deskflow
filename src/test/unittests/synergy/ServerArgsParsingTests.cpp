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
#include "synergy/ServerArgs.h"
#include "test/mock/synergy/MockArgParser.h"

#include "test/global/gtest.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;

bool
server_stubParseGenericArgs(int, const char* const*, int&)
{
	return false;
}

bool
server_stubCheckUnexpectedArgs()
{
	return false;
}

TEST(ServerArgsParsingTests, parseServerArgs_addressArg_setSynergyAddress)
{
	NiceMock<MockArgParser> argParser;
	ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(server_stubParseGenericArgs));
	ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(server_stubCheckUnexpectedArgs));
	ServerArgs serverArgs;
	const int argc = 3;
	const char* kAddressCmd[argc] = { "stub", "--address", "mock_address" };

	argParser.parseServerArgs(serverArgs, argc, kAddressCmd);

	EXPECT_EQ("mock_address", serverArgs.m_synergyAddress);
}

TEST(ServerArgsParsingTests, parseServerArgs_configArg_setConfigFile)
{
	NiceMock<MockArgParser> argParser;
	ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(server_stubParseGenericArgs));
	ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(server_stubCheckUnexpectedArgs));
	ServerArgs serverArgs;
	const int argc = 3;
	const char* kConfigCmd[argc] = { "stub", "--config", "mock_configFile" };

	argParser.parseServerArgs(serverArgs, argc, kConfigCmd);

	EXPECT_EQ("mock_configFile", serverArgs.m_configFile);
}
