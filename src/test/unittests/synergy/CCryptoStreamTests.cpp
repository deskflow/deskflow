/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Bolton Software Ltd.
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
#include "CCryptoStream.h"
#include "CMockStream.h"
#include "CMockEventQueue.h"

using ::testing::_;
using ::testing::Invoke;

using namespace std;

void assertWrite(const void* in, UInt32 n);
UInt8 mockRead(void* out, UInt32 n);

TEST(CCryptoTests, write)
{
	const UInt32 size = 4;
	UInt8* buffer = new UInt8[size];
	buffer[0] = 'D';
	buffer[1] = 'K';
	buffer[2] = 'D';
	buffer[3] = 'N';
	
	CMockEventQueue eventQueue;
	CMockStream innerStream(eventQueue);
	
	ON_CALL(innerStream, write(_, _)).WillByDefault(Invoke(assertWrite));
	EXPECT_CALL(innerStream, write(_, _)).Times(1);
	EXPECT_CALL(innerStream, getEventTarget()).Times(3);
	EXPECT_CALL(eventQueue, removeHandlers(_)).Times(1);
	EXPECT_CALL(eventQueue, adoptHandler(_, _, _)).Times(1);
	EXPECT_CALL(eventQueue, removeHandler(_, _)).Times(1);

	CCryptoStream cs(eventQueue, &innerStream);
	cs.write(buffer, size);
}

TEST(CCryptoTests, read)
{
	CMockEventQueue eventQueue;
	CMockStream innerStream(eventQueue);
	
	ON_CALL(innerStream, read(_, _)).WillByDefault(Invoke(mockRead));
	EXPECT_CALL(innerStream, read(_, _)).Times(1);
	EXPECT_CALL(innerStream, getEventTarget()).Times(3);
	EXPECT_CALL(eventQueue, removeHandlers(_)).Times(1);
	EXPECT_CALL(eventQueue, adoptHandler(_, _, _)).Times(1);
	EXPECT_CALL(eventQueue, removeHandler(_, _)).Times(1);

	CCryptoStream cs(eventQueue, &innerStream);
	
	const UInt32 size = 4;
	UInt8* buffer = new UInt8[size];
	cs.read(buffer, size);

	EXPECT_EQ('D', buffer[0]);
	EXPECT_EQ('K', buffer[1]);
	EXPECT_EQ('D', buffer[2]);
	EXPECT_EQ('N', buffer[3]);
}

void
assertWrite(const void* in, UInt32 n)
{
	UInt8* buffer = static_cast<UInt8*>(const_cast<void*>(in));
	EXPECT_EQ(55, buffer[0]);
	EXPECT_EQ(142, buffer[1]);
	EXPECT_EQ(189, buffer[2]);
	EXPECT_EQ(237, buffer[3]);
}

UInt8
mockRead(void* out, UInt32 n)
{
	UInt8* buffer = static_cast<UInt8*>(out);
	buffer[0] = 55;
	buffer[1] = 142;
	buffer[2] = 189;
	buffer[3] = 237;
	return n;
}
