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

#pragma once

#include "SubscriptionKey.h"
#include "common/common.h"

#include "gtest/gtest_prod.h"

class SubscriptionManager {
public:
	SubscriptionManager();
	
	//! Check the subscription activation file
	void				checkFile(const String& filename);

	//! Create a subscription activation file based on a serial
	void				activate(const String& serial);

	//! Use standard output to return subscription filename to gui
	void				printFilename();

private:
	FRIEND_TEST(SubscriptionTests, decode_invalidLength_throwException);
	FRIEND_TEST(SubscriptionTests, decode_unrecognizedDigit_throwException);
	FRIEND_TEST(SubscriptionTests, decode_invalidSerial_outputPlainText);
	FRIEND_TEST(SubscriptionTests, parsePlainSerial_noParity_throwException);
	FRIEND_TEST(SubscriptionTests, parsePlainSerial_invalidSerial_throwException);
	FRIEND_TEST(SubscriptionTests, parsePlainSerial_validSerial_throwException);
	FRIEND_TEST(SubscriptionTests, parsePlainSerial_expiredSerial_throwException);

private:
	String				decode(const String& input);
	void				parsePlainSerial(const String& plainText, SubscriptionKey& key);
	String				getFilename();

	SubscriptionKey		m_key;
};
