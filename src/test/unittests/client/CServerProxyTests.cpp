/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2011 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#define TEST_ENV
#include "Global.h"

#include "CServerProxy.h"
#include "CMockClient.h"
#include "CMockStream.h"
#include "CMockEventQueue.h"
#include "ProtocolTypes.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::AnyNumber;

int streamReads = 0;

UInt32
streamRead(void* buffer, UInt32 n);

// TODO: fix linking in windows (works in unix for some reason).
#if 0
TEST(CServerProxyTests, parseMessage_mouseMove_valuesCorrect)
{
	NiceMock<CMockEventQueue> eventQueue;
	CMockClient client(eventQueue);
	CMockStream stream(eventQueue);

	ON_CALL(stream, read(_, _)).WillByDefault(Invoke(streamRead));
	EXPECT_CALL(stream, read(_, _)).Times(4);
	EXPECT_CALL(stream, write(_, _)).Times(1);
	EXPECT_CALL(stream, isReady()).Times(1);
	EXPECT_CALL(stream, getEventTarget()).Times(AnyNumber());

	CServerProxy serverProxy(&client, &stream, eventQueue);

	// skip handshake, go straight to normal parser.
	serverProxy.m_parser = &CServerProxy::parseMessage;

	// assert
	EXPECT_CALL(client, mouseMove(10, 20));

	serverProxy.handleData(NULL, NULL);
}
#endif

UInt32
streamRead(void* buffer, UInt32 n)
{
	streamReads++;
	UInt8* code = (UInt8*)buffer;

	if (streamReads == 1) {
		code[0] = 'D';
		code[1] = 'M';
		code[2] = 'M';
		code[3] = 'V';
		return 4;
	}
	else if (streamReads == 2) {
		code[0] = 0;
		code[1] = 10;
		return 2;
	}
	else if (streamReads == 3) {
		code[0] = 0;
		code[1] = 20;
		return 2;
	}

	return 0;
}
