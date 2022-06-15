/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2016 Symless Inc.
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

#define TEST_ENV

#include <climits>
#include "shared/SerialKey.h"

#include "test/global/gtest.h"

TEST(SerialKeyTests, isExpiring_validV2TrialBasicSerial_returnFalse)
{
    // {v2;trial;basic;Bob;1;email;company name;1;86400}
    SerialKey serial("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636F6D70616E79206E616D653B313B38363430307D");
    EXPECT_EQ(true, serial.isTrial());
    EXPECT_EQ(true, serial.isTemporary());
    EXPECT_FALSE(serial.isExpiring(0));
    EXPECT_EQ(kBasic, serial.edition());
}

TEST(SerialKeyTests, isExpiring_expiringV2TrialBasicSerial_returnTrue)
{
    // {v2;trial;basic;Bob;1;email;company name;0;86400}
    SerialKey serial("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636F6D70616E79206E616D653B303B38363430307D");
    EXPECT_EQ(true, serial.isTrial());
    EXPECT_EQ(true, serial.isTemporary());
    EXPECT_EQ(true, serial.isExpiring(1));
}

TEST(SerialKeyTests, isExpiring_expiredV2TrialBasicSerial_returnFalse)
{
    // {v2;trial;basic;Bob;1;email;company name;0;86400}
    SerialKey serial("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636F6D70616E79206E616D653B303B38363430307D");
    EXPECT_EQ(true, serial.isTrial());
    EXPECT_EQ(true, serial.isTemporary());
    EXPECT_FALSE(serial.isExpiring(86401));
}

TEST(SerialKeyTests, isExpired_validV2TrialBasicSerial_returnFalse)
{
    // {v2;trial;basic;Bob;1;email;company name;0;86400}
    SerialKey serial("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636F6D70616E79206E616D653B303B38363430307D");
    EXPECT_EQ(true, serial.isTrial());
    EXPECT_EQ(true, serial.isTemporary());
    EXPECT_FALSE(serial.isExpired(0));
}

TEST(SerialKeyTests, isExpired_expiringV2TrialBasicSerial_returnFalse)
{
    // {v2;trial;basic;Bob;1;email;company name;0;86400}
    SerialKey serial("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636F6D70616E79206E616D653B303B38363430307D");
    EXPECT_EQ(true, serial.isTrial());
    EXPECT_EQ(true, serial.isTemporary());
    EXPECT_FALSE(serial.isExpired(1));
}

TEST(SerialKeyTests, isExpired_expiredV2TrialBasicSerial_returnTrue)
{
    // {v2;trial;basic;Bob;1;email;company name;0;86400}
    SerialKey serial("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636F6D70616E79206E616D653B303B38363430307D");
    EXPECT_EQ(true, serial.isTrial());
    EXPECT_EQ(true, serial.isTemporary());
    EXPECT_EQ(true, serial.isExpired(86401));
}

TEST(SerialKeyTests, daysLeft_validExactlyOneDayV2TrialBasicSerial_returnOne)
{
    // {v2;trial;basic;Bob;1;email;company name;0;86400}
    SerialKey serial("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636F6D70616E79206E616D653B303B38363430307D");
    EXPECT_EQ(1, serial.daysLeft(0));
}

TEST(SerialKeyTests, daysLeft_validWithinOneDayV2TrialBasicSerial_returnOne)
{
    // {v2;trial;basic;Bob;1;email;company name;0;86400}
    SerialKey serial("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636F6D70616E79206E616D653B303B38363430307D");
    EXPECT_EQ(1, serial.daysLeft(1));
}

TEST(SerialKeyTests, daysLeft_expiredV2TrialBasicSerial_returnZero)
{
    // {v2;trial;basic;Bob;1;email;company name;0;86400}
    SerialKey serial("7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636F6D70616E79206E616D653B303B38363430307D");
    EXPECT_EQ(0, serial.daysLeft(86401));
}

//Subscription license tests
TEST(SerialKeyTests, isExpiring_validV2SubscriptionBasicSerial_returnFalse)
{
    // {v2;subscription;basic;Bob;1;email;company name;1;86400}
    SerialKey serial("7B76323B737562736372697074696F6E3B62617369633B426F623B313B656D61696C3B636F6D70616E79206E616D653B313B38363430307D");
    EXPECT_EQ(false, serial.isTrial());
    EXPECT_EQ(true, serial.isTemporary());
    EXPECT_FALSE(serial.isExpiring(0));
    EXPECT_EQ(kBasic, serial.edition());
}

TEST(SerialKeyTests, isExpiring_expiringV2SubscriptionBasicSerial_returnTrue)
{
    // {v2;subscription;basic;Bob;1;email;company name;0;86400}
    SerialKey serial("7B76323B737562736372697074696F6E3B62617369633B426F623B313B656D61696C3B636F6D70616E79206E616D653B303B38363430307D");
    EXPECT_EQ(false, serial.isTrial());
    EXPECT_EQ(true, serial.isTemporary());
    EXPECT_EQ(true, serial.isExpiring(1));
    EXPECT_FALSE(serial.isMaintenance());
}

TEST(SerialKeyTests, isExpiring_expiringV2MentenanceSerial_returnTrue)
{
    // {v2;maintenance;basic;Bob;1;email;company name;0;86400}
    SerialKey serial("7B76323B6D61696E74656E616E63653B70726F3B736572686969206861647A68696C6F763B313B7365726869694073796D6C6573732E636F6D3B203B303B313635353132343139307D");
    EXPECT_FALSE(serial.isTrial());
    EXPECT_FALSE(serial.isTemporary());
    EXPECT_TRUE(serial.isMaintenance());
}

TEST(SerialKeyTests, isExpired_expiredV2SubscriptionBasicSerial_returnTrue)
{
    // {v2;subscription;basic;Bob;1;email;company name;0;86400}
    SerialKey serial("7B76323B737562736372697074696F6E3B62617369633B426F623B313B656D61696C3B636F6D70616E79206E616D653B303B38363430307D");
    EXPECT_EQ(false, serial.isTrial());
    EXPECT_EQ(true, serial.isTemporary());
    EXPECT_EQ(true, serial.isExpired(86401));
}

//toString method tests
TEST(SerialKeyTests, toStringV2SubscriptionBasicSerialKey)
{
    //{v2;subscription;basic;Bob;1;email;company name;0;86400}
    const std::string Expected = "7B76323B737562736372697074696F6E3B62617369633B426F623B313B656D61696C3B636F6D70616E79206E616D653B303B38363430307D";
    SerialKey serial(Expected);
    EXPECT_EQ(Expected, serial.toString());
}

TEST(SerialKeyTests, toStringV2TrialBasicSerialKey)
{
    //{v2;trial;basic;Bob;1;email;company name;0;86400}
    const std::string Expected = "7B76323B747269616C3B62617369633B426F623B313B656D61696C3B636F6D70616E79206E616D653B303B38363430307D";
    SerialKey serial(Expected);
    EXPECT_EQ(Expected, serial.toString());
}

TEST(SerialKeyTests, toStringV1BasicSerialKey)
{
    //{v1;basic;Bob;1;email;company name;0;0}
    const std::string Expected = "7B76313B62617369633B426F623B313B656D61696C3B636F6D70616E79206E616D653B303B307D";
    SerialKey serial(Expected);
    EXPECT_EQ(Expected, serial.toString());
}

TEST(SerialKeyTests, IsValidKey_false)
{
    //{v1;basic;Bob;1;email;company name;0;0}
    SerialKey serial(kUnregistered);
    EXPECT_EQ(false, serial.isValid());
}

TEST(SerialKeyTests, IsValidKey_true)
{
    SerialKey serial(kBasic);
    EXPECT_EQ(true, serial.isValid());
}

TEST(SerialKeyTests, IsValidExpiredKey_false)
{
    SerialKey serial("7B76323B737562736372697074696F6E3B62617369633B426F623B313B656D61696C3B636F6D70616E79206E616D653B303B38363430307D");
    EXPECT_EQ(false, serial.isValid());
}

TEST(SerialKeyTests, test_getSpanLeft)
{
    SerialKey key;
    int expected{-1};
    EXPECT_EQ(expected, key.getSpanLeft());
}

TEST(SerialKeyTests, test_getSpanLeft_subscription)
{
    // {v2;subscription;basic;Bob;1;email;company name;0;86400}
    SerialKey key("7B76323B737562736372697074696F6E3B62617369633B426F623B313B656D61696C3B636F6D70616E79206E616D653B303B38363430307D");
    EXPECT_EQ(1000, key.getSpanLeft(86399));
    EXPECT_EQ(-1, key.getSpanLeft(86401));
}

TEST(SerialKeyTests, test_getSpanLeft_max_int)
{
    //"{v2;subscription;basic;Bob;1;email;company name;0;3147483647}"
    SerialKey key("7B76323B737562736372697074696F6E3B62617369633B426F623B313B656D61696C3B636F6D70616E79206E616D653B303B333134373438333634377D");
    EXPECT_EQ(INT_MAX, key.getSpanLeft(1));
}


