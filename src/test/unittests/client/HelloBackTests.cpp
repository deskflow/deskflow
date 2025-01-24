/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "client/HelloBack.h"

#include "common/common.h"
#include "mock/io/MockStream.h"

#include <array>
#include <cstring>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iomanip>
#include <sstream>

using HelloBack = deskflow::client::HelloBack;
using namespace testing;

namespace {

class MockDeps : public HelloBack::Deps
{
public:
  ~MockDeps() override = default;
  MOCK_METHOD(void, invalidHello, (), (override));
  MOCK_METHOD(void, incompatible, (int major, int minor), (override));
};

void intTo2ByteBuf(int16_t value, std::array<char, 2> &buf)
{
  buf[0] = static_cast<char>((value >> 8) & 0xFF); // MSB
  buf[1] = static_cast<char>(value & 0xFF);        // LSB
}

void intTo4ByteBuf(int32_t value, std::array<char, 4> &buf)
{
  buf[0] = static_cast<char>((value >> 24) & 0xFF); // MSB
  buf[1] = static_cast<char>((value >> 16) & 0xFF);
  buf[2] = static_cast<char>((value >> 8) & 0xFF);
  buf[3] = static_cast<char>(value & 0xFF); // LSB
}

std::string printAsHex(const char *buffer, size_t size)
{
  std::ostringstream hexStream;
  for (size_t i = 0; i < size; ++i) {
    hexStream << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(static_cast<unsigned char>(buffer[i])) << " ";
  }
  return hexStream.str();
}

void setupMockHelloRead(
    MockStream &stream, const std::string &protocolName, const int16_t majorVersion, const int16_t minorVersion
)
{

  std::array<char, 2> majorBuf;
  std::array<char, 2> minorBuf;
  intTo2ByteBuf(majorVersion, majorBuf);
  intTo2ByteBuf(minorVersion, minorBuf);

  EXPECT_CALL(stream, read(_, _))
      .WillOnce(DoAll(
          WithArg<0>([protocolName](void *vbuffer) {
            auto buffer = static_cast<char *>(vbuffer);
            std::copy(protocolName.begin(), protocolName.end(), buffer);
          }),
          Return(7)
      ))
      .WillOnce(DoAll(
          WithArg<0>([majorBuf](void *vbuffer) {
            auto buffer = static_cast<char *>(vbuffer);
            std::memcpy(buffer, majorBuf.data(), majorBuf.size());
          }),
          Return(2)
      ))
      .WillOnce(DoAll(
          WithArg<0>([minorBuf](void *vbuffer) {
            auto buffer = static_cast<char *>(vbuffer);
            std::memcpy(buffer, minorBuf.data(), minorBuf.size());
          }),
          Return(2)
      ));
}

void setupMockHelloBackWrite(
    MockStream &stream, const std::string &protocolName, const int16_t majorVersion, const int16_t minorVersion,
    const std::string &name
)
{

  std::array<char, 2> majorBuf;
  std::array<char, 2> minorBuf;
  std::array<char, 4> nameLenBuf;
  intTo2ByteBuf(majorVersion, majorBuf);
  intTo2ByteBuf(minorVersion, minorBuf);
  intTo4ByteBuf(static_cast<int32_t>(name.size()), nameLenBuf);

  const auto versionIntSize = 4;
  const auto clientNameIntSize = 4;
  const uint32_t helloBackSize =
      static_cast<uint32_t>(protocolName.size() + versionIntSize + clientNameIntSize + name.size());

  std::vector<char> expect;
  expect.reserve(helloBackSize);
  expect.insert(expect.end(), protocolName.begin(), protocolName.end());
  expect.insert(expect.end(), majorBuf.begin(), majorBuf.end());
  expect.insert(expect.end(), minorBuf.begin(), minorBuf.end());
  expect.insert(expect.end(), nameLenBuf.begin(), nameLenBuf.end());
  expect.insert(expect.end(), name.begin(), name.end());

  EXPECT_CALL(stream, write(_, helloBackSize)).WillOnce(WithArg<0>([expect, helloBackSize](const void *vbuffer) {
    const auto buffer = static_cast<const char *>(vbuffer);

    EXPECT_TRUE(std::memcmp(expect.data(), buffer, helloBackSize) == 0)
        << "Buffer mismatch\n"
        << "Expected: " << printAsHex(expect.data(), helloBackSize) << "\n"
        << "Actual:   " << printAsHex(buffer, helloBackSize) << "\n";
  }));
}

} // namespace

TEST(HelloBackTests, handleHello_nastyProtocol_invalidHello)
{
  auto deps = std::make_shared<NiceMock<MockDeps>>();
  HelloBack helloBack(deps);
  NiceMock<MockStream> stream;
  const std::string clientName = "stub";

  setupMockHelloRead(stream, "ShareMouse", 0, 0);

  EXPECT_CALL(*deps, invalidHello()).Times(1);

  helloBack.handleHello(&stream, clientName);
}

TEST(HelloBackTests, handleHello_synergyProtocolCurrent_validMessage)
{
  auto deps = std::make_shared<NiceMock<MockDeps>>();
  HelloBack helloBack(deps, 1, 2);
  NiceMock<MockStream> stream;
  const std::string clientName = "stub";

  setupMockHelloRead(stream, "Synergy", 1, 2);

  EXPECT_CALL(*deps, incompatible(_, _)).Times(0);
  EXPECT_CALL(*deps, invalidHello()).Times(0);

  helloBack.handleHello(&stream, clientName);
}

TEST(HelloBackTests, handleHello_barrierProtocolCurrent_validMessage)
{
  auto deps = std::make_shared<NiceMock<MockDeps>>();
  HelloBack helloBack(deps, 1, 2);
  NiceMock<MockStream> stream;
  const std::string clientName = "stub";

  setupMockHelloRead(stream, "Barrier", 1, 2);

  EXPECT_CALL(*deps, incompatible(_, _)).Times(0);
  EXPECT_CALL(*deps, invalidHello()).Times(0);

  helloBack.handleHello(&stream, clientName);
}

TEST(HelloBackTests, handleHello_synergyProtocolOlder_validMessage)
{
  auto deps = std::make_shared<NiceMock<MockDeps>>();
  HelloBack helloBack(deps, 1, 2);
  NiceMock<MockStream> stream;
  const std::string clientName = "stub";

  setupMockHelloRead(stream, "Synergy", 1, 1);

  EXPECT_CALL(*deps, incompatible(1, 1)).Times(1);

  helloBack.handleHello(&stream, clientName);
}

TEST(HelloBackTests, handleHello_synergyProtocolCurrent_wroteHelloBack)
{
  auto deps = std::make_shared<NiceMock<MockDeps>>();
  HelloBack helloBack(deps, 1, 2);
  NiceMock<MockStream> stream;
  const std::string clientName = "test client";

  setupMockHelloRead(stream, "Synergy", 1, 2);

  setupMockHelloBackWrite(stream, "Synergy", 1, 2, "test client");

  helloBack.handleHello(&stream, clientName);
}

TEST(HelloBackTests, handleHello_barrierProtocolCurrent_wroteHelloBack)
{
  auto deps = std::make_shared<NiceMock<MockDeps>>();
  HelloBack helloBack(deps, 1, 2);
  NiceMock<MockStream> stream;
  const std::string clientName = "test client";

  setupMockHelloRead(stream, "Barrier", 1, 2);

  setupMockHelloBackWrite(stream, "Barrier", 1, 2, "test client");

  helloBack.handleHello(&stream, clientName);
}

// If the client is protocol version 1.8 and the server is 1.6, the client
// should downgrade and respond with the server version.
TEST(HelloBackTests, handleHello_synergyProtocolCompat_wroteHelloBack)
{
  auto deps = std::make_shared<NiceMock<MockDeps>>();
  HelloBack helloBack(deps, 1, 8);
  NiceMock<MockStream> stream;
  const std::string clientName = "test client";

  setupMockHelloRead(stream, "Synergy", 1, 6);

  setupMockHelloBackWrite(stream, "Synergy", 1, 6, "test client");

  helloBack.handleHello(&stream, clientName);
}
