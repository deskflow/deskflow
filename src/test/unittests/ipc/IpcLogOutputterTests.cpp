/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
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

#include "test/mock/ipc/MockIpcServer.h"

#include "mt/Thread.h"
#include "ipc/IpcLogOutputter.h"
#include "base/String.h"

#include "test/global/gmock.h"
#include "test/global/gtest.h"

using ::testing::_;
using ::testing::Return;
using ::testing::Matcher;
using ::testing::MatcherCast;
using ::testing::Property;
using ::testing::StrEq;

using namespace synergy;

inline const Matcher<const IpcMessage&> IpcLogLineMessageEq(const String& s) {
	const Matcher<const IpcLogLineMessage&> m(
		Property(&IpcLogLineMessage::logLine, StrEq(s)));
	return MatcherCast<const IpcMessage&>(m);
}

TEST(IpcLogOutputterTests, write_bufferSizeWrapping)
{
	MockIpcServer mockServer;
	
	ON_CALL(mockServer, hasClients(_)).WillByDefault(Return(true));

	EXPECT_CALL(mockServer, hasClients(_)).Times(1);
	EXPECT_CALL(mockServer, send(IpcLogLineMessageEq("mock 2\nmock 3\n"), _)).Times(1);

	IpcLogOutputter outputter(mockServer);
	outputter.bufferMaxSize(2);

	// log more lines than the buffer can contain
	for (UInt8 i = 1; i <= 3; i++) {
		String s = string::sprintf("mock %d", i);
		outputter.write(kNOTE, s.c_str());
	}

	// close, but wait until the buffer is empty.
	outputter.close(true);

	EXPECT_EQ(true, true);
}
