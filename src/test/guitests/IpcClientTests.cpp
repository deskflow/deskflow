#include "IpcClient.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::_;
using testing::StrEq;

class MockStream : public QDataStreamProxy {
public:
  MOCK_METHOD(int, writeRawData, (const char *, int), (override));
};

TEST(IpcClientTests, sendCommand_anyCommand_commandSent) {
  auto mockStream = std::make_shared<MockStream>();
  IpcClient::StreamProvider streamProvider = [&mockStream]() {
    return mockStream;
  };

  EXPECT_CALL(*mockStream, writeRawData(_, _)).Times(3);
  EXPECT_CALL(*mockStream, writeRawData(StrEq("test"), 4)).Times(1);

  IpcClient ipcClient(streamProvider);
  ipcClient.sendCommand("test", ElevateMode::ElevateAsNeeded);
}
