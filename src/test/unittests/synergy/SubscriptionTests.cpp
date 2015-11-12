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

#include "synergy/SubscriptionManager.h"
#include "synergy/XSynergy.h"

#include "test/global/gtest.h"

TEST(SubscriptionTests, decode_invalidLength_throwException)
{
	SubscriptionManager subscriptionManager;
	String serial("ABC");

	EXPECT_THROW(subscriptionManager.decode(serial), XSubscription);
}

TEST(SubscriptionTests, decode_unrecognizedDigit_throwException)
{
	SubscriptionManager subscriptionManager;
	String serial("MOCK");

	EXPECT_THROW(subscriptionManager.decode(serial), XSubscription);
}

TEST(SubscriptionTests, parsePlainSerial_noParity_throwException)
{
	SubscriptionManager subscriptionManager;
	String painText("MOCK");
	SubscriptionKey key;

	EXPECT_THROW(subscriptionManager.parsePlainSerial(painText, key), XSubscription);
}

TEST(SubscriptionTests, parsePlainSerial_invalidSerial_throwException)
{
	SubscriptionManager subscriptionManager;
	String painText("{MOCK}");
	SubscriptionKey key;

	EXPECT_THROW(subscriptionManager.parsePlainSerial(painText, key), XSubscription);
}

TEST(SubscriptionTests, parsePlainSerial_validSerial_validSubscriptionKey)
{
	// valid until 2 March 2049
	SubscriptionManager subscriptionManager;
	String painText("{v1;trial;Bob;1;a@a.a;mock company;2147483647;2147483647}");
	SubscriptionKey key;
	subscriptionManager.parsePlainSerial(painText, key);

	EXPECT_EQ("trial", key.m_type);
	EXPECT_EQ("Bob", key.m_name);
	EXPECT_EQ(1, key.m_userLimit);
	EXPECT_EQ("a@a.a", key.m_email);
	EXPECT_EQ("mock company", key.m_company);
	EXPECT_EQ(2147483647, key.m_warnTime);
	EXPECT_EQ(2147483647, key.m_expireTime);
}

TEST(SubscriptionTests, parsePlainSerial_validSerialWithoutCompany_validSubscriptionKey)
{
	// valid until 2 March 2049
	SubscriptionManager subscriptionManager;
	String painText("{v1;trial;Bob;1;a@a.a;;2147483647;2147483647}");
	SubscriptionKey key;
	subscriptionManager.parsePlainSerial(painText, key);

	EXPECT_EQ("trial", key.m_type);
	EXPECT_EQ("Bob", key.m_name);
	EXPECT_EQ(1, key.m_userLimit);
	EXPECT_EQ("a@a.a", key.m_email);
	EXPECT_EQ("", key.m_company);
	EXPECT_EQ(2147483647, key.m_warnTime);
	EXPECT_EQ(2147483647, key.m_expireTime);
}

TEST(SubscriptionTests, parsePlainSerial_expiredTrialSerial_throwException)
{
	SubscriptionManager subscriptionManager;
	String painText("{v1;trial;Bob;1;1398297600;1398384000}");
	SubscriptionKey key;

	EXPECT_THROW(subscriptionManager.parsePlainSerial(painText, key), XSubscription);
}

TEST(SubscriptionTests, parsePlainSerial_expiredBasicSerial_validSubscriptionKey)
{
	SubscriptionManager subscriptionManager;
	String painText("{v1;basic;Bob;1;a@a.a;mock company;1398297600;1398384000}");
	SubscriptionKey key;
	subscriptionManager.parsePlainSerial(painText, key);

	EXPECT_EQ("basic", key.m_type);
	EXPECT_EQ("Bob", key.m_name);
	EXPECT_EQ(1, key.m_userLimit);
	EXPECT_EQ("a@a.a", key.m_email);
	EXPECT_EQ("mock company", key.m_company);
	EXPECT_EQ(1398297600, key.m_warnTime);
	EXPECT_EQ(1398384000, key.m_expireTime);
}
