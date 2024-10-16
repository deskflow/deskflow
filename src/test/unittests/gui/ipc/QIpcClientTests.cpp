/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
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
