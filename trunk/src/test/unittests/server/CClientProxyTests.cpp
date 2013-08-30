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
#include "CClientProxy1_4.h"
#include "CMockServer.h"
#include "CMockStream.h"
#include "CMockCryptoStream.h"
#include "CMockEventQueue.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Invoke;
using ::testing::ReturnRef;

const UInt8 g_cryptoIvWrite_bufferLen = 200;
UInt8 g_cryptoIvWrite_buffer[g_cryptoIvWrite_bufferLen];
UInt32 g_cryptoIvWrite_writeBufferIndex;
UInt32 g_cryptoIvWrite_readBufferIndex;

void cryptoIv_mockWrite(const void* in, UInt32 n);
UInt8 cryptoIv_mockRead(void* out, UInt32 n);

TEST(CClientProxyTests, cryptoIvWrite)
{
	g_cryptoIvWrite_writeBufferIndex = 0;
	g_cryptoIvWrite_readBufferIndex = 0;

	NiceMock<CMockEventQueue> eventQueue;
	NiceMock<CMockStream> innerStream;
	NiceMock<CMockServer> server;
	CCryptoOptions options("cfb", "mock");
	IStreamEvents streamEvents;
	streamEvents.setEvents(&eventQueue);

	CCryptoStream* serverStream = new CCryptoStream(&eventQueue, &innerStream, options, false);
	CCryptoStream* clientStream = new CCryptoStream(&eventQueue, &innerStream, options, false);

	byte iv[CRYPTO_IV_SIZE];
	serverStream->newIv(iv);
	serverStream->setEncryptIv(iv);
	clientStream->setDecryptIv(iv);
	
	ON_CALL(eventQueue, forIStream()).WillByDefault(ReturnRef(streamEvents));
	ON_CALL(innerStream, write(_, _)).WillByDefault(Invoke(cryptoIv_mockWrite));
	ON_CALL(innerStream, read(_, _)).WillByDefault(Invoke(cryptoIv_mockRead));

	CClientProxy1_4 clientProxy("stub", serverStream, &server, &eventQueue);
	
	UInt8 buffer[100];
	clientStream->read(buffer, 4);

	g_cryptoIvWrite_writeBufferIndex = 0;
	g_cryptoIvWrite_readBufferIndex = 0;

	// DCIV, then DKDN.
	clientProxy.keyDown(1, 2, 3);
	clientStream->read(buffer, 24);
	EXPECT_EQ('D', buffer[0]);
	EXPECT_EQ('C', buffer[1]);
	EXPECT_EQ('I', buffer[2]);
	EXPECT_EQ('V', buffer[3]);
	clientStream->setDecryptIv(&buffer[8]);
	clientStream->read(buffer, 10);
	EXPECT_EQ('D', buffer[0]);
	EXPECT_EQ('K', buffer[1]);
	EXPECT_EQ('D', buffer[2]);
	EXPECT_EQ('N', buffer[3]);
	
	g_cryptoIvWrite_writeBufferIndex = 0;
	g_cryptoIvWrite_readBufferIndex = 0;

	// DCIV, then DKUP.
	clientProxy.keyUp(1, 2, 3);
	clientStream->read(buffer, 24);
	EXPECT_EQ('D', buffer[0]);
	EXPECT_EQ('C', buffer[1]);
	EXPECT_EQ('I', buffer[2]);
	EXPECT_EQ('V', buffer[3]);
	clientStream->setDecryptIv(&buffer[8]);
	clientStream->read(buffer, 10);
	EXPECT_EQ('D', buffer[0]);
	EXPECT_EQ('K', buffer[1]);
	EXPECT_EQ('U', buffer[2]);
	EXPECT_EQ('P', buffer[3]);
	
	g_cryptoIvWrite_writeBufferIndex = 0;
	g_cryptoIvWrite_readBufferIndex = 0;
	
	// DCIV, then DKRP.
	clientProxy.keyRepeat(1, 2, 4, 4);
	clientStream->read(buffer, 24);
	EXPECT_EQ('D', buffer[0]);
	EXPECT_EQ('C', buffer[1]);
	EXPECT_EQ('I', buffer[2]);
	EXPECT_EQ('V', buffer[3]);
	clientStream->setDecryptIv(&buffer[8]);
	clientStream->read(buffer, 12);
	EXPECT_EQ('D', buffer[0]);
	EXPECT_EQ('K', buffer[1]);
	EXPECT_EQ('R', buffer[2]);
	EXPECT_EQ('P', buffer[3]);
}

void
cryptoIv_mockWrite(const void* in, UInt32 n)
{
	assert(g_cryptoIvWrite_writeBufferIndex <= sizeof(g_cryptoIvWrite_buffer));
	memcpy(&g_cryptoIvWrite_buffer[g_cryptoIvWrite_writeBufferIndex], in, n);
	g_cryptoIvWrite_writeBufferIndex += n;
}

UInt8
cryptoIv_mockRead(void* out, UInt32 n)
{
	assert(g_cryptoIvWrite_readBufferIndex <= sizeof(g_cryptoIvWrite_buffer));
	memcpy(out, &g_cryptoIvWrite_buffer[g_cryptoIvWrite_readBufferIndex], n);
	g_cryptoIvWrite_readBufferIndex += n;
	return n;
}
