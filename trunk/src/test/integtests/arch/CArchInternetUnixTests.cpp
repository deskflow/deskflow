/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Bolton Software Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>
#include "CArchInternetUnix.h"

#define TEST_URL "https://synergy-foss.org/tests/?testString"
//#define TEST_URL "http://localhost/synergy/tests/?testString"

TEST(CArchInternetWindowsTests, openWebPage)
{
	CArchInternetUnix internet;
	CString result = internet.get(TEST_URL);
	ASSERT_EQ("Hello world!", result);
}
