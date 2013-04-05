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
#include "CPacketStreamFilter.h"
using ::testing::_;
using ::testing::Invoke;

using namespace std;

void assertWrite(const void* in, UInt32 n);
UInt8 mockRead(void* out, UInt32 n);
void write4Read1_mockWrite(const void* in, UInt32 n);
UInt8 write4Read1_mockRead(void* out, UInt32 n);
void write1Read4_mockWrite(const void* in, UInt32 n);
UInt8 write1Read4_mockRead(void* out, UInt32 n);

UInt8 g_write4Read1_buffer[4];
UInt32 g_write4Read1_bufferIndex = 0;

UInt8 g_write1Read4_buffer[4];
UInt32 g_write1Read4_bufferIndex = 0;

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

	CCryptoStream cs(eventQueue, &innerStream, false);
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

	CCryptoStream cs(eventQueue, &innerStream, false);
	
	const UInt32 size = 4;
	UInt8* buffer = new UInt8[size];
	cs.read(buffer, size);

	EXPECT_EQ('D', buffer[0]);
	EXPECT_EQ('K', buffer[1]);
	EXPECT_EQ('D', buffer[2]);
	EXPECT_EQ('N', buffer[3]);
}

TEST(CCryptoTests, write4Read1)
{
	CMockEventQueue eventQueue;
	CMockStream innerStream(eventQueue);
	
	ON_CALL(innerStream, write(_, _)).WillByDefault(Invoke(write4Read1_mockWrite));
	ON_CALL(innerStream, read(_, _)).WillByDefault(Invoke(write4Read1_mockRead));
	EXPECT_CALL(innerStream, write(_, _)).Times(4);
	EXPECT_CALL(innerStream, read(_, _)).Times(1);
	EXPECT_CALL(innerStream, getEventTarget()).Times(6);
	EXPECT_CALL(eventQueue, removeHandlers(_)).Times(2);
	EXPECT_CALL(eventQueue, adoptHandler(_, _, _)).Times(2);
	EXPECT_CALL(eventQueue, removeHandler(_, _)).Times(2);

	CCryptoStream cs1(eventQueue, &innerStream, false);
	CCryptoStream cs2(eventQueue, &innerStream, false);
	
	cs1.write("a", 1);
	cs1.write("b", 1);
	cs1.write("c", 1);
	cs1.write("d", 1);
	
	UInt8 buffer[4];
	cs2.read(buffer, 4);
	
	EXPECT_EQ('a', buffer[0]);
	EXPECT_EQ('b', buffer[1]);
	EXPECT_EQ('c', buffer[2]);
	EXPECT_EQ('d', buffer[3]);
}

TEST(CCryptoTests, write1Read4)
{
	CMockEventQueue eventQueue;
	CMockStream innerStream(eventQueue);
	
	ON_CALL(innerStream, write(_, _)).WillByDefault(Invoke(write1Read4_mockWrite));
	ON_CALL(innerStream, read(_, _)).WillByDefault(Invoke(write1Read4_mockRead));
	EXPECT_CALL(innerStream, write(_, _)).Times(1);
	EXPECT_CALL(innerStream, read(_, _)).Times(4);
	EXPECT_CALL(innerStream, getEventTarget()).Times(6);
	EXPECT_CALL(eventQueue, removeHandlers(_)).Times(2);
	EXPECT_CALL(eventQueue, adoptHandler(_, _, _)).Times(2);
	EXPECT_CALL(eventQueue, removeHandler(_, _)).Times(2);

	CCryptoStream cs1(eventQueue, &innerStream, false);
	CCryptoStream cs2(eventQueue, &innerStream, false);

	UInt8 bufferIn[4];
	bufferIn[0] = 'a';
	bufferIn[1] = 'b';
	bufferIn[2] = 'c';
	bufferIn[3] = 'd';
	cs1.write(bufferIn, 4);

	UInt8 bufferOut[4];
	cs2.read(&bufferOut[0], 1);
	cs2.read(&bufferOut[1], 1);
	cs2.read(&bufferOut[2], 1);
	cs2.read(&bufferOut[3], 1);
	
	EXPECT_EQ('a', bufferOut[0]);
	EXPECT_EQ('b', bufferOut[1]);
	EXPECT_EQ('c', bufferOut[2]);
	EXPECT_EQ('d', bufferOut[3]);
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

void
write4Read1_mockWrite(const void* in, UInt32 n)
{
	UInt8* buffer = static_cast<UInt8*>(const_cast<void*>(in));
	g_write4Read1_buffer[g_write4Read1_bufferIndex++] = buffer[0];
}

UInt8
write4Read1_mockRead(void* out, UInt32 n)
{
	UInt8* buffer = static_cast<UInt8*>(out);
	buffer[0] = g_write4Read1_buffer[0];
	buffer[1] = g_write4Read1_buffer[1];
	buffer[2] = g_write4Read1_buffer[2];
	buffer[3] = g_write4Read1_buffer[3];
	return 4;
}

void
write1Read4_mockWrite(const void* in, UInt32 n)
{
	UInt8* buffer = static_cast<UInt8*>(const_cast<void*>(in));
	g_write1Read4_buffer[0] = buffer[0];
	g_write1Read4_buffer[1] = buffer[1];
	g_write1Read4_buffer[2] = buffer[2];
	g_write1Read4_buffer[3] = buffer[3];
}

UInt8
write1Read4_mockRead(void* out, UInt32 n)
{
	UInt8* buffer = static_cast<UInt8*>(out);
	buffer[0] = g_write1Read4_buffer[g_write1Read4_bufferIndex++];
	return 1;
}

