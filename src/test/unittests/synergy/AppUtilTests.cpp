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

#include "test/global/gtest.h"
#include "synergy/AppUtil.h"

TEST(AppUtilTests, keyboardLayoutTest)
{
    String layoutList1 = "ruenby";
    std::vector<String> layoutList2 = { "ru", "en" };
    std::vector<String> missed;
    std::vector<String> supported;
    AppUtil::getKeyboardLayoutsDiff(layoutList1, layoutList2, missed, supported);

    EXPECT_EQ(missed, std::vector<String>{ "by" });
    EXPECT_EQ(supported, layoutList2);
}

TEST(AppUtilTests, joinVectorTest)
{
    std::vector<String> layouts = { "ru", "en" };
    auto result = AppUtil::joinStrVector(layouts, ", ");

    EXPECT_EQ(result, "ru, en");
}
