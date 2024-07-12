#include "gui/src/QIpcClient.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::_;
using testing::StrEq;

class MockStream : public QDataStreamProxy {
public:
  MOCK_METHOD(int, writeRawData, (const char *, int), (override));
};

TEST(QIpcClientTests, sendCommand_anyCommand_commandSent) {
  auto mockStream = std::make_shared<MockStream>();
  QIpcClient::StreamProvider streamProvider = [&mockStream]() {
    return mockStream;
  };

  EXPECT_CALL(*mockStream, writeRawData(_, _)).Times(3);
  EXPECT_CALL(*mockStream, writeRawData(StrEq("test"), 4)).Times(1);

  QIpcClient ipcClient(streamProvider);
  ipcClient.sendCommand("test", ElevateMode::ElevateAsNeeded);
}
