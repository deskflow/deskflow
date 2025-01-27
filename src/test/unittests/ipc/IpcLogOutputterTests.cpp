/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#define TEST_ENV

#include "test/mock/ipc/MockIpcServer.h"

#include "base/String.h"
#include "common/common.h"
#include "ipc/IpcLogOutputter.h"
#include "mt/Thread.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// HACK: ipc logging only used on windows anyway
#if WINAPI_MSWINDOWS

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Matcher;
using ::testing::MatcherCast;
using ::testing::Property;
using ::testing::Return;
using ::testing::StrEq;

using namespace deskflow;

// TODO Fix the IPC Tests for windows, see #6709
//  Tests disabled due to gtest/gmock update causing build problems
//  Decision to disable tests and create issue instead due to time constraints
// inline const Matcher<const IpcMessage&> IpcLogLineMessageEq(const std::string& s)
// {
//     const Matcher<const IpcLogLineMessage&> m(
//         Property(&IpcLogLineMessage::logLine, StrEq(s)));
//     return MatcherCast<const IpcMessage&>(m);
// }
//
// TEST(IpcLogOutputterTests, write_threadingEnabled_bufferIsSent)
//{
//     MockIpcServer mockServer;
//     mockServer.delegateToFake();
//
//     ON_CALL(mockServer, hasClients(_)).WillByDefault(Return(true));
//
//     EXPECT_CALL(mockServer, hasClients(_)).Times(AtLeast(3));
//     EXPECT_CALL(mockServer, send(IpcLogLineMessageEq("mock 1\n"),
//     _)).Times(1); EXPECT_CALL(mockServer, send(IpcLogLineMessageEq("mock
//     2\n"), _)).Times(1);
//
//     IpcLogOutputter outputter(mockServer, IpcClientType::Unknown, true);
//     outputter.write(kNOTE, "mock 1");
//     mockServer.waitForSend();
//     outputter.write(kNOTE, "mock 2");
//     mockServer.waitForSend();
// }
//
// TEST(IpcLogOutputterTests, write_overBufferMaxSize_firstLineTruncated)
//{
//     MockIpcServer mockServer;
//
//     ON_CALL(mockServer, hasClients(_)).WillByDefault(Return(true));
//     EXPECT_CALL(mockServer, hasClients(_)).Times(1);
//     EXPECT_CALL(mockServer, send(IpcLogLineMessageEq("mock 2\nmock 3\n"),
//     _)).Times(1);
//
//     IpcLogOutputter outputter(mockServer, IpcClientType::Unknown, false);
//     outputter.bufferMaxSize(2);
//
//     // log more lines than the buffer can contain
//     outputter.write(kNOTE, "mock 1");
//     outputter.write(kNOTE, "mock 2");
//     outputter.write(kNOTE, "mock 3");
//     outputter.sendBuffer();
// }
//
// TEST(IpcLogOutputterTests, write_underBufferMaxSize_allLinesAreSent)
//{
//     MockIpcServer mockServer;
//
//     ON_CALL(mockServer, hasClients(_)).WillByDefault(Return(true));
//
//     EXPECT_CALL(mockServer, hasClients(_)).Times(1);
//     EXPECT_CALL(mockServer, send(IpcLogLineMessageEq("mock 1\nmock 2\n"),
//     _)).Times(1);
//
//     IpcLogOutputter outputter(mockServer, IpcClientType::Unknown, false);
//     outputter.bufferMaxSize(2);
//
//     // log more lines than the buffer can contain
//     outputter.write(kNOTE, "mock 1");
//     outputter.write(kNOTE, "mock 2");
//     outputter.sendBuffer();
// }
//
//// HACK: temporarily disable this intermittently failing unit test.
//// when the build machine is under heavy load, a race condition
//// usually happens.
// #if 0
//  TEST(IpcLogOutputterTests, write_overBufferRateLimit_lastLineTruncated)
//{
//     MockIpcServer mockServer;
//
//     ON_CALL(mockServer, hasClients(_)).WillByDefault(Return(true));
//
//     EXPECT_CALL(mockServer, hasClients(_)).Times(2);
//     EXPECT_CALL(mockServer, send(IpcLogLineMessageEq("mock 1\nmock 2\n"),
//     _)).Times(1); EXPECT_CALL(mockServer, send(IpcLogLineMessageEq("mock
//     4\nmock 5\n"), _)).Times(1);
//
//     IpcLogOutputter outputter(mockServer, false);
//     outputter.bufferRateLimit(2, 1); // 1s
//
//     // log 1 more line than the buffer can accept in time limit.
//     outputter.write(kNOTE, "mock 1");
//     outputter.write(kNOTE, "mock 2");
//     outputter.write(kNOTE, "mock 3");
//
//     outputter.sendBuffer();
//
//     // after waiting the time limit send another to make sure
//     // we can log after the time limit passes.
//     // HACK: sleep causes the unit test to fail intermittently,
//     // so lets try 100ms (there must be a better way to solve this)
//     ARCH->sleep(2); // 2s
//     outputter.write(kNOTE, "mock 4");
//     outputter.write(kNOTE, "mock 5");
//     outputter.write(kNOTE, "mock 6");
//
//     outputter.sendBuffer();
// }
// #endif
//
//  TEST(IpcLogOutputterTests, write_underBufferRateLimit_allLinesAreSent)
//{
//     MockIpcServer mockServer;
//
//     ON_CALL(mockServer, hasClients(_)).WillByDefault(Return(true));
//
//     EXPECT_CALL(mockServer, hasClients(_)).Times(2);
//     EXPECT_CALL(mockServer, send(IpcLogLineMessageEq("mock 1\nmock 2\n"),
//     _)).Times(1); EXPECT_CALL(mockServer, send(IpcLogLineMessageEq("mock
//     3\nmock 4\n"), _)).Times(1);
//
//     IpcLogOutputter outputter(mockServer, IpcClientType::Unknown, false);
//     outputter.bufferRateLimit(4, 1); // 1s (should be plenty of time)
//
//     // log 1 more line than the buffer can accept in time limit.
//     outputter.write(kNOTE, "mock 1");
//     outputter.write(kNOTE, "mock 2");
//     outputter.sendBuffer();
//
//     // after waiting the time limit send another to make sure
//     // we can log after the time limit passes.
//     outputter.write(kNOTE, "mock 3");
//     outputter.write(kNOTE, "mock 4");
//     outputter.sendBuffer();
// }

#endif // WINAPI_MSWINDOWS
