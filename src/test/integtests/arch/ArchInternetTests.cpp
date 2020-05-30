/*
 * barrier -- mouse and keyboard sharing utility
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

#include "arch/Arch.h"

#include "test/global/gtest.h"

#define TEST_URL "https://symless.com/tests/?testString"
//#define TEST_URL "http://localhost/barrier/tests/?testString"

TEST(ArchInternetTests, DISABLED_get)
{
    ARCH_INTERNET internet;
    std::string result = internet.get(TEST_URL);
    ASSERT_EQ("Hello world!", result);
}

TEST(ArchInternetTests, urlEncode)
{
    ARCH_INTERNET internet;
    std::string result = internet.urlEncode("hello=+&world");
    ASSERT_EQ("hello%3D%2B%26world", result);
}
