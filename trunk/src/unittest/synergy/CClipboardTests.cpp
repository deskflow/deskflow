/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2010 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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
#include "CClipboard.h"

TEST(CClipboardTests, getTime_defaultState_returnsZero)
{
	// arrange
	CClipboard clipboard;

	// act
	CClipboard::Time actual = clipboard.getTime();

	// assert
	CClipboard::Time expected(0);
	EXPECT_EQ(expected, actual);
}
