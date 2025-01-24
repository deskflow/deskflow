/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "lib/arch/IArchString.h"

#include <gtest/gtest.h>

class SampleIArchString : public IArchString
{
public:
  EWideCharEncoding getWideCharEncoding() override
  {
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
