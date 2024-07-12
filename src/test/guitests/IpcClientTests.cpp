#include "IpcClient.h"
#include "TempTest.h"
#include "providers/StreamProvider.h"

#include "gmock/gmock-spec-builders.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <qobject.h>
#include <qtcpsocket.h>

using testing::_;
using testing::StrEq;

class MockStream : public StreamProvider::StreamProxy {
public:
  MOCK_METHOD(int, writeRawData, (const char *, int), (override));
};

class MockStreamProvider : public StreamProvider {
public:
  explicit MockStreamProvider(MockStream *mockStream)
      : m_MockStream(mockStream) {}

  StreamProxy *makeStream() override { return m_MockStream; }

private:
  MockStream *m_MockStream;
};

void testStreamProvider(StreamProvider &sp) {
  auto stream = sp.makeStream();
  stream->writeRawData("test", 4);
}

TEST(IpcClientTests, sendCommand_anyCommand_commandSent) {
  MockStream mockStream;
  MockStreamProvider mockStreamProvider(&mockStream);

  EXPECT_CALL(mockStream, writeRawData(_, _)).Times(3);
  EXPECT_CALL(mockStream, writeRawData(StrEq("test"), 4)).Times(1);

  IpcClient ipcClient(&mockStreamProvider);
  ipcClient.sendCommand("test", ElevateMode::ElevateAsNeeded);
}
