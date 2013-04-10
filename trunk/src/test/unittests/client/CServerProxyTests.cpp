/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2011 Nick Bolton
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

#define TEST_ENV

#include <gtest/gtest.h>
#include "CServerProxy.h"
#include "CMockClient.h"
#include "CMockStream.h"
#include "CMockEventQueue.h"
#include "ProtocolTypes.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::AnyNumber;

const UInt8 mouseMove_bufferLen = 16;
UInt8 mouseMove_buffer[mouseMove_bufferLen];
UInt32 mouseMove_bufferIndex;
UInt32 mouseMove_mockRead(void* buffer, UInt32 n);

const UInt8 cryptoIv_bufferLen = 20;
UInt8 cryptoIv_buffer[cryptoIv_bufferLen];
UInt32 cryptoIv_bufferIndex;
CString cryptoIv_result;
UInt32 cryptoIv_mockRead(void* buffer, UInt32 n);
void cryptoIv_setCryptoIv(const UInt8*);

TEST(CServerProxyTests, mouseMove)
{
	mouseMove_bufferIndex = 0;

	NiceMock<CMockEventQueue> eventQueue;
	NiceMock<CMockClient> client;
	NiceMock<CMockStream> stream;

	ON_CALL(stream, read(_, _)).WillByDefault(Invoke(mouseMove_mockRead));
	
	EXPECT_CALL(client, mouseMove(1, 2)).Times(1);
	
	const char data[] = "DSOP\0\0\0\0DMMV\0\1\0\2";
	memcpy(mouseMove_buffer, data, mouseMove_bufferLen);

	CServerProxy serverProxy(&client, &stream, &eventQueue);
	serverProxy.handleDataForTest();
}

TEST(CServerProxyTests, cryptoIv)
{
	cryptoIv_bufferIndex = 0;

	NiceMock<CMockEventQueue> eventQueue;
	NiceMock<CMockClient> client;
	NiceMock<CMockStream> stream;

	ON_CALL(stream, read(_, _)).WillByDefault(Invoke(cryptoIv_mockRead));
	ON_CALL(client, setCryptoIv(_)).WillByDefault(Invoke(cryptoIv_setCryptoIv));

	const char data[] = "DSOP\0\0\0\0DCIV\0\0\0\4mock";
	memcpy(cryptoIv_buffer, data, cryptoIv_bufferLen);

	CServerProxy serverProxy(&client, &stream, &eventQueue);
	serverProxy.handleDataForTest();

	EXPECT_EQ("mock", cryptoIv_result);
}

UInt32
mouseMove_mockRead(void* buffer, UInt32 n)
{
	if (mouseMove_bufferIndex >= mouseMove_bufferLen) {
		return 0;
	}
	memcpy(buffer, &mouseMove_buffer[mouseMove_bufferIndex], n);
	mouseMove_bufferIndex += n;
	return n;
}

UInt32
cryptoIv_mockRead(void* buffer, UInt32 n)
{
	if (cryptoIv_bufferIndex >= cryptoIv_bufferLen) {
		return 0;
	}
	memcpy(buffer, &cryptoIv_buffer[cryptoIv_bufferIndex], n);
	cryptoIv_bufferIndex += n;
	return n;
}

void
cryptoIv_setCryptoIv(const UInt8* data)
{
	cryptoIv_result = reinterpret_cast<const char*>(data);
}
