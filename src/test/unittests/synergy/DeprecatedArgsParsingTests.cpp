/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si, Inc.
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

#include "test/global/gtest.h"

using namespace synergy;

TEST(DeprecatedArgsParsingTests, parseDeprecatedArgs_cryptoPass_returnTrue)
{
	int i = 1;
	const int argc = 3;
	const char* kCryptoPassCmd[argc] = { "stub", "--crypto-pass", "mock_pass" };

	ArgParser argParser(NULL);

	bool result = argParser.parseDeprecatedArgs(argc, kCryptoPassCmd, i);

	EXPECT_EQ(true, result);
	EXPECT_EQ(2, i);
}

TEST(DeprecatedArgsParsingTests, parseDeprecatedArgs_cryptoPass_returnFalse)
{
	int i = 1;
	const int argc = 3;
	const char* kCryptoPassCmd[argc] = { "stub", "--mock-arg", "mock_value" };

	ArgParser argParser(NULL);

	bool result = argParser.parseDeprecatedArgs(argc, kCryptoPassCmd, i);

	EXPECT_FALSE(result);
	EXPECT_EQ(1, i);
}
