/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "gui/ipc/QIpcClient.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::_;
using testing::StrEq;

namespace {

class MockStream : public QDataStreamProxy
{
public:
  MOCK_METHOD(qint64, writeRawData, (const char *, int), (override));
};

} // namespace

TEST(QIpcClientTests, sendCommand_anyCommand_commandSent)
{
  auto mockStream = std::make_shared<MockStream>();
  QIpcClient::StreamProvider streamProvider = [&mockStream]() { return mockStream; };

  EXPECT_CALL(*mockStream, writeRawData(_, _)).Times(3);
  EXPECT_CALL(*mockStream, writeRawData(StrEq("test"), 4)).Times(1);

  QIpcClient ipcClient(streamProvider);
  ipcClient.sendCommand("test", ElevateMode::kAutomatic);
}
