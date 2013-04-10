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

const UInt8 cryptoIvWrite_bufferLen = 200;
UInt8 cryptoIvWrite_buffer[cryptoIvWrite_bufferLen];
UInt32 cryptoIvWrite_bufferIndex;

void
cryptoIv_mockWrite(const void* in, UInt32 n);

TEST(CClientProxyTests, cryptoIvWrite)
{
	cryptoIvWrite_bufferIndex = 0;

	NiceMock<CMockEventQueue> eventQueue;
	NiceMock<CMockStream> innerStream;
	NiceMock<CMockServer> server;
	NiceMock<CMockCryptoStream>* stream = new NiceMock<CMockCryptoStream>(&eventQueue, &innerStream);

	ON_CALL(*stream, write(_, _)).WillByDefault(Invoke(cryptoIv_mockWrite));

	CClientProxy1_4 clientProxy("stub", stream, &server, &eventQueue);

	// DCIV, then DKDN.
	cryptoIvWrite_bufferIndex = 0;
	clientProxy.keyDown(1, 2, 3);
	EXPECT_EQ('D', cryptoIvWrite_buffer[0]);
	EXPECT_EQ('C', cryptoIvWrite_buffer[1]);
	EXPECT_EQ('I', cryptoIvWrite_buffer[2]);
	EXPECT_EQ('V', cryptoIvWrite_buffer[3]);
	EXPECT_EQ('D', cryptoIvWrite_buffer[24]);
	EXPECT_EQ('K', cryptoIvWrite_buffer[25]);
	EXPECT_EQ('D', cryptoIvWrite_buffer[26]);
	EXPECT_EQ('N', cryptoIvWrite_buffer[27]);
	
	// DCIV, then DKUP.
	cryptoIvWrite_bufferIndex = 0;
	clientProxy.keyUp(1, 2, 3);
	EXPECT_EQ('D', cryptoIvWrite_buffer[0]);
	EXPECT_EQ('C', cryptoIvWrite_buffer[1]);
	EXPECT_EQ('I', cryptoIvWrite_buffer[2]);
	EXPECT_EQ('V', cryptoIvWrite_buffer[3]);
	EXPECT_EQ('D', cryptoIvWrite_buffer[24]);
	EXPECT_EQ('K', cryptoIvWrite_buffer[25]);
	EXPECT_EQ('U', cryptoIvWrite_buffer[26]);
	EXPECT_EQ('P', cryptoIvWrite_buffer[27]);
	
	// DCIV, then DKRP.
	cryptoIvWrite_bufferIndex = 0;
	clientProxy.keyRepeat(1, 2, 4, 4);
	EXPECT_EQ('D', cryptoIvWrite_buffer[0]);
	EXPECT_EQ('C', cryptoIvWrite_buffer[1]);
	EXPECT_EQ('I', cryptoIvWrite_buffer[2]);
	EXPECT_EQ('V', cryptoIvWrite_buffer[3]);
	EXPECT_EQ('D', cryptoIvWrite_buffer[24]);
	EXPECT_EQ('K', cryptoIvWrite_buffer[25]);
	EXPECT_EQ('R', cryptoIvWrite_buffer[26]);
	EXPECT_EQ('P', cryptoIvWrite_buffer[27]);
}

void
cryptoIv_mockWrite(const void* in, UInt32 n)
{
	if (cryptoIvWrite_bufferIndex >= cryptoIvWrite_bufferLen) {
		return;
	}
	memcpy(&cryptoIvWrite_buffer[cryptoIvWrite_bufferIndex], in, n);
	cryptoIvWrite_bufferIndex += n;
}
