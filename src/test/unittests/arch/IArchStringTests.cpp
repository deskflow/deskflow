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

#include "lib/arch/IArchString.h"
#include "test/global/gtest.h"

class SampleIArchString: public IArchString {
public:
    EWideCharEncoding getWideCharEncoding() override {
        return kUTF16;
    }
};

TEST(IArchStringTests, convStringWCToMB_will_work_do_simple_conversions)
{
    SampleIArchString as;
    char buff[20];
    bool errors;
    auto converted = as.convStringWCToMB(buff, L"Hello", 6, &errors);
    EXPECT_STREQ(buff, "Hello");
    EXPECT_EQ(converted, 6);
    EXPECT_EQ(errors, false);
}

TEST(IArchStringTests, convStringWCToMB_will_work_do_simple_conversions_noresult)
{
    SampleIArchString as;
    bool errors;
    auto converted = as.convStringWCToMB(nullptr, L"Hello", 6, &errors);
    EXPECT_EQ(converted, 6);
    EXPECT_EQ(errors, false);
}

TEST(IArchStringTests, convStringMBToWC_will_work_do_simple_conversions)
{
    SampleIArchString as;
    wchar_t buff[20];
    bool errors;
    auto converted = as.convStringMBToWC(buff, "Hello", 6, &errors);
    EXPECT_STREQ(buff, L"Hello");
    EXPECT_EQ(converted, 6);
    EXPECT_EQ(errors, false);
}
