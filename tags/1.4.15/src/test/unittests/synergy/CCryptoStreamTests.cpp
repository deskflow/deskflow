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
#include "CCryptoOptions.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;

using namespace std;

const byte kIv[] = "aaaaaaaaaaaaaaaa"; // AES block size = 16 (\0 not used)

UInt8 g_write_buffer[4];
void write_mockWrite(const void* in, UInt32 n);

UInt8 g_read_buffer[4];
UInt8 read_mockRead(void* out, UInt32 n);

UInt8 g_write4Read1_buffer[4];
UInt32 g_write4Read1_bufferIndex;
void write4Read1_mockWrite(const void* in, UInt32 n);
UInt8 write4Read1_mockRead(void* out, UInt32 n);

UInt8 g_write1Read4_buffer[4];
UInt32 g_write1Read4_bufferIndex;
void write1Read4_mockWrite(const void* in, UInt32 n);
UInt8 write1Read4_mockRead(void* out, UInt32 n);

UInt8 g_readWriteIvChanged_buffer[4];
UInt32 g_readWriteIvChangeTrigger_writeBufferIndex;
UInt32 g_readWriteIvChangeTrigger_readBufferIndex;
void readWriteIvChanged_mockWrite(const void* in, UInt32 n);
UInt8 readWriteIvChanged_mockRead(void* out, UInt32 n);

UInt8 g_readWriteIvChangeTrigger_buffer[4 + 4 + 16]; // abcd, DCIV, 16-byte IV
void readWriteIvChangeTrigger_mockWrite(const void* in, UInt32 n);
UInt8 readWriteIvChangeTrigger_mockRead(void* out, UInt32 n);

UInt8 g_newIvDoesNotChangeIv_buffer[1];
void newIvDoesNotChangeIv_mockWrite(const void* in, UInt32 n);

TEST(CCryptoStreamTests, write)
{
	const UInt32 size = 4;
	UInt8* buffer = new UInt8[size];
	buffer[0] = 'D';
	buffer[1] = 'K';
	buffer[2] = 'D';
	buffer[3] = 'N';
	
	NiceMock<CMockEventQueue> eventQueue;
	NiceMock<CMockStream> innerStream;
	CCryptoOptions options("cfb", "mock");
	
	ON_CALL(innerStream, write(_, _)).WillByDefault(Invoke(write_mockWrite));
	
	CCryptoStream cs(&eventQueue, &innerStream, options, false);
	cs.setEncryptIv(kIv);
	cs.write(buffer, size);
	
	EXPECT_EQ(95, g_write_buffer[0]);
	EXPECT_EQ(107, g_write_buffer[1]);
	EXPECT_EQ(152, g_write_buffer[2]);
	EXPECT_EQ(220, g_write_buffer[3]);
}

TEST(CCryptoStreamTests, read)
{
	NiceMock<CMockEventQueue> eventQueue;
	NiceMock<CMockStream> innerStream;
	CCryptoOptions options("cfb", "mock");
	
	ON_CALL(innerStream, read(_, _)).WillByDefault(Invoke(read_mockRead));
	
	CCryptoStream cs(&eventQueue, &innerStream, options, false);
	cs.setEncryptIv(kIv);
	cs.setDecryptIv(kIv);
	
	g_read_buffer[0] = 95;
	g_read_buffer[1] = 107;
	g_read_buffer[2] = 152;
	g_read_buffer[3] = 220;

	const UInt32 size = 4;
	UInt8* buffer = new UInt8[size];
	cs.read(buffer, size);

	EXPECT_EQ('D', buffer[0]);
	EXPECT_EQ('K', buffer[1]);
	EXPECT_EQ('D', buffer[2]);
	EXPECT_EQ('N', buffer[3]);
}

TEST(CCryptoStreamTests, write4Read1)
{
	g_write4Read1_bufferIndex = 0;

	NiceMock<CMockEventQueue> eventQueue;
	NiceMock<CMockStream> innerStream;
	CCryptoOptions options("cfb", "mock");
	
	ON_CALL(innerStream, write(_, _)).WillByDefault(Invoke(write4Read1_mockWrite));
	ON_CALL(innerStream, read(_, _)).WillByDefault(Invoke(write4Read1_mockRead));
	
	CCryptoStream cs1(&eventQueue, &innerStream, options, false);
	cs1.setEncryptIv(kIv);
	
	cs1.write("a", 1);
	cs1.write("b", 1);
	cs1.write("c", 1);
	cs1.write("d", 1);

	CCryptoStream cs2(&eventQueue, &innerStream, options, false);
	cs2.setDecryptIv(kIv);
	
	UInt8 buffer[4];
	cs2.read(buffer, 4);
	
	EXPECT_EQ('a', buffer[0]);
	EXPECT_EQ('b', buffer[1]);
	EXPECT_EQ('c', buffer[2]);
	EXPECT_EQ('d', buffer[3]);
}

TEST(CCryptoStreamTests, write1Read4)
{
	g_write1Read4_bufferIndex = 0;

	NiceMock<CMockEventQueue> eventQueue;
	NiceMock<CMockStream> innerStream;
	CCryptoOptions options("cfb", "mock");
	
	ON_CALL(innerStream, write(_, _)).WillByDefault(Invoke(write1Read4_mockWrite));
	ON_CALL(innerStream, read(_, _)).WillByDefault(Invoke(write1Read4_mockRead));

	CCryptoStream cs1(&eventQueue, &innerStream, options, false);
	cs1.setEncryptIv(kIv);

	UInt8 bufferIn[4];
	bufferIn[0] = 'a';
	bufferIn[1] = 'b';
	bufferIn[2] = 'c';
	bufferIn[3] = 'd';
	cs1.write(bufferIn, 4);
	
	CCryptoStream cs2(&eventQueue, &innerStream, options, false);
	cs2.setDecryptIv(kIv);

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

TEST(CCryptoStreamTests, readWriteIvChanged)
{
	g_readWriteIvChangeTrigger_writeBufferIndex = 0;
	g_readWriteIvChangeTrigger_readBufferIndex = 0;

	NiceMock<CMockEventQueue> eventQueue;
	NiceMock<CMockStream> innerStream;
	CCryptoOptions options("cfb", "mock");
	
	ON_CALL(innerStream, write(_, _)).WillByDefault(Invoke(readWriteIvChanged_mockWrite));
	ON_CALL(innerStream, read(_, _)).WillByDefault(Invoke(readWriteIvChanged_mockRead));
	
	// AES block size = 16 (\0 not used)
	const byte iv1[] = "aaaaaaaaaaaaaaaa";
	const byte iv2[] = "bbbbbbbbbbbbbbbb";
	
	CCryptoStream cs1(&eventQueue, &innerStream, options, false);
	cs1.setEncryptIv(iv1);
	
	UInt8 bufferIn[4];
	bufferIn[0] = 'a';
	bufferIn[1] = 'b';
	bufferIn[2] = 'c';
	bufferIn[3] = 'd';
	cs1.write(bufferIn, 4);
	
	CCryptoStream cs2(&eventQueue, &innerStream, options, false);
	cs1.setDecryptIv(iv2);

	UInt8 bufferOut[4];
	cs2.read(bufferOut, 4);
	
	// assert that the values cannot be decrypted, since the second crypto
	// class instance is using a different IV.
	EXPECT_NE('a', bufferOut[0]);
	EXPECT_NE('b', bufferOut[1]);
	EXPECT_NE('c', bufferOut[2]);
	EXPECT_NE('d', bufferOut[3]);

	// generate a new IV and copy it to the second crypto class, and
	// ensure that the new IV is used.
	byte iv[CRYPTO_IV_SIZE];
	cs1.newIv(iv);
	cs1.setEncryptIv(iv);
	cs2.setDecryptIv(iv);

	cs1.write(bufferIn, 4);
	cs2.read(bufferOut, 4);

	EXPECT_EQ('a', bufferOut[0]);
	EXPECT_EQ('b', bufferOut[1]);
	EXPECT_EQ('c', bufferOut[2]);
	EXPECT_EQ('d', bufferOut[3]);
}

TEST(CCryptoStreamTests, createKey)
{
	byte hash1[16];
	CCryptoStream::createKey(hash1, "MockLongPassword", 16, 16);
	EXPECT_EQ(hash1[0], 149);
	EXPECT_EQ(hash1[15], 235);
	
	byte hash2[32];
	CCryptoStream::createKey(hash2, "MockLongPassword", 32, 16);
	EXPECT_EQ(hash2[0], 149);
	EXPECT_EQ(hash2[15], 235);
	EXPECT_EQ(hash2[31], 7);
	
	byte hash3[32];
	CCryptoStream::createKey(hash3, "7accbf2f86a9191da0947107d4fe7054", 32, 16);
	EXPECT_EQ(hash3[0], 120);
	EXPECT_EQ(hash3[15], 11);
	EXPECT_EQ(hash3[31], 233);
}

TEST(CCryptoStreamTests, newIvDoesNotChangeIv)
{
	NiceMock<CMockEventQueue> eventQueue;
	NiceMock<CMockStream> innerStream;
	CCryptoOptions options("cfb", "mock");	
	
	ON_CALL(innerStream, write(_, _)).WillByDefault(Invoke(newIvDoesNotChangeIv_mockWrite));

	CCryptoStream cs1(&eventQueue, &innerStream, options, false);
	cs1.write("a", 1);
	EXPECT_EQ(175, g_newIvDoesNotChangeIv_buffer[0]);

	byte iv[CRYPTO_IV_SIZE];
	cs1.newIv(iv);

	cs1.write("a", 1);
	EXPECT_EQ(92, g_newIvDoesNotChangeIv_buffer[0]);
}

void
write_mockWrite(const void* in, UInt32 n)
{
	memcpy(g_write_buffer, in, n);
}

UInt8
read_mockRead(void* out, UInt32 n)
{
	memcpy(out, g_read_buffer, n);
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
	memcpy(out, g_write4Read1_buffer, n);
	return n;
}

void
write1Read4_mockWrite(const void* in, UInt32 n)
{
	memcpy(g_write1Read4_buffer, in, n);
}

UInt8
write1Read4_mockRead(void* out, UInt32 n)
{
	UInt8* buffer = static_cast<UInt8*>(out);
	buffer[0] = g_write1Read4_buffer[g_write1Read4_bufferIndex++];
	return 1;
}

void
readWriteIvChanged_mockWrite(const void* in, UInt32 n)
{
	memcpy(g_readWriteIvChanged_buffer, in, n);
}

UInt8
readWriteIvChanged_mockRead(void* out, UInt32 n)
{
	memcpy(out, g_readWriteIvChanged_buffer, n);
	return n;
}

void
readWriteIvChangeTrigger_mockWrite(const void* in, UInt32 n)
{
	assert(g_readWriteIvChangeTrigger_writeBufferIndex <= sizeof(g_readWriteIvChangeTrigger_buffer));
	memcpy(&g_readWriteIvChangeTrigger_buffer[g_readWriteIvChangeTrigger_writeBufferIndex], in, n);
	g_readWriteIvChangeTrigger_writeBufferIndex += n;
}

UInt8
readWriteIvChangeTrigger_mockRead(void* out, UInt32 n)
{
	assert(g_readWriteIvChangeTrigger_readBufferIndex <= sizeof(g_readWriteIvChangeTrigger_buffer));
	memcpy(out, &g_readWriteIvChangeTrigger_buffer[g_readWriteIvChangeTrigger_readBufferIndex], n);
	g_readWriteIvChangeTrigger_readBufferIndex += n;
	return n;
}

void
newIvDoesNotChangeIv_mockWrite(const void* in, UInt32 n)
{
	memcpy(g_newIvDoesNotChangeIv_buffer, in, 1);
}
