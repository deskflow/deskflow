/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2016 Symless Ltd.
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

#include "test/global/gtest.h"

using namespace synergy;

TEST (DeprecatedArgsParsingTests, parseDeprecatedArgs_cryptoPass_returnTrue) {
    int i                            = 1;
    const int argc                   = 3;
    const char* kCryptoPassCmd[argc] = {"stub", "--crypto-pass", "mock_pass"};

    ArgParser argParser (NULL);

    bool result = argParser.parseDeprecatedArgs (argc, kCryptoPassCmd, i);

    EXPECT_EQ (true, result);
    EXPECT_EQ (2, i);
}

TEST (DeprecatedArgsParsingTests, parseDeprecatedArgs_cryptoPass_returnFalse) {
    int i                            = 1;
    const int argc                   = 3;
    const char* kCryptoPassCmd[argc] = {"stub", "--mock-arg", "mock_value"};

    ArgParser argParser (NULL);

    bool result = argParser.parseDeprecatedArgs (argc, kCryptoPassCmd, i);

    EXPECT_FALSE (result);
    EXPECT_EQ (1, i);
}
