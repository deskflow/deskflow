/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Seamless Inc.
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

#include "synergy/DpiHelper.h"

#include "test/global/gtest.h"

void resetStaticVariables()
{
	DpiHelper::s_resolutionWidth = 0;
	DpiHelper::s_resolutionHeight = 0;
	DpiHelper::s_primaryWidthCenter = 0;
	DpiHelper::s_primaryHeightCenter = 0;
	DpiHelper::s_dpi = DpiHelper::kDefaultDpi;
	DpiHelper::s_dpiScaled = false;
}

TEST(DpiHelperTests, calculateDpi_samePhysicalAndVirtualResolutions_defaultDpi)
{
	resetStaticVariables();

	DpiHelper::s_resolutionWidth = 1920;
	DpiHelper::s_resolutionHeight = 1080;
	DpiHelper::s_primaryWidthCenter = 960;
	DpiHelper::s_primaryHeightCenter = 540;

	DpiHelper::calculateDpi(1920, 1080);

	EXPECT_FALSE(DpiHelper::s_dpiScaled);
	EXPECT_EQ(DpiHelper::kDefaultDpi, DpiHelper::s_dpi);
}

TEST(DpiHelperTests, calculateDpi_differentPhysicalAndVirtualResolutions_scaledDpi)
{
	resetStaticVariables();

	DpiHelper::s_resolutionWidth = 1920;
	DpiHelper::s_resolutionHeight = 1080;
	DpiHelper::s_primaryWidthCenter = 960;
	DpiHelper::s_primaryHeightCenter = 540;

	DpiHelper::calculateDpi(960, 540);

	EXPECT_TRUE(DpiHelper::s_dpiScaled);
	EXPECT_EQ(200, DpiHelper::s_dpi);
}

TEST(DpiHelperTests, calculateDpi_defaultStaticValues_defaultDpi)
{
	resetStaticVariables();

	DpiHelper::calculateDpi(1920, 1080);

	EXPECT_FALSE(DpiHelper::s_dpiScaled);
	EXPECT_EQ(DpiHelper::kDefaultDpi, DpiHelper::s_dpi);
}
