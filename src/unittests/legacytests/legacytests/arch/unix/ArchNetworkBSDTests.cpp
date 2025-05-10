/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/IArchNetwork.h"
#include "lib/arch/unix/ArchNetworkBSD.h"

#include "lib/arch/XArch.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <netinet/in.h>
#include <sys/types.h>

using ::testing::_;
using ::testing::NiceMock;
using PollEntries = std::vector<IArchNetwork::PollEntry>;
using PollFD = struct pollfd[];

namespace {
struct MockDeps : public ArchNetworkBSD::Deps
{
  std::shared_ptr<PollFD> m_pollFD;

  MockDeps()
  {
    ON_CALL(*this, makePollFD(_)).WillByDefault([this](nfds_t n) {
      m_pollFD = ArchNetworkBSD::Deps::makePollFD(n);
      return m_pollFD;
    });
  }

  static std::shared_ptr<NiceMock<MockDeps>> makeNice()
  {
    return std::make_shared<NiceMock<MockDeps>>();
  }

  MOCK_METHOD(void, sleep, (double), (override));
  MOCK_METHOD(int, poll, (struct pollfd *, nfds_t, int), (override));
  MOCK_METHOD(std::shared_ptr<PollFD>, makePollFD, (nfds_t), (override));
  MOCK_METHOD(ssize_t, read, (int, void *, size_t), (override));
  MOCK_METHOD(void, testCancelThread, (), (override));
};
} // namespace

TEST(ArchNetworkBSDTests, pollSocket_zeroEntries_callsSleep)
{
  auto deps = MockDeps::makeNice();
  ArchNetworkBSD networkBSD(deps);

  EXPECT_CALL(*deps, sleep(1)).Times(1);
  auto result = networkBSD.pollSocket(nullptr, 0, 1);

  EXPECT_EQ(result, 0);
}

TEST(ArchNetworkBSDTests, pollSocket_mockAccessError_throws)
{
  auto deps = MockDeps::makeNice();
  ON_CALL(*deps, poll(_, _, _)).WillByDefault([]() {
    errno = EACCES;
    return -1;
  });
  ArchNetworkBSD networkBSD(deps);
  PollEntries entries{{nullptr, 0, 0}};

  const auto f = [&] { networkBSD.pollSocket(entries.data(), static_cast<int>(entries.size()), 1); };

  EXPECT_THROW({ f(); }, XArchNetworkAccess);
}

TEST(ArchNetworkBSDTests, pollSocket_pfdHasRevents_copiedToEntries)
{
  auto deps = MockDeps::makeNice();
  ON_CALL(*deps, poll(_, _, _)).WillByDefault([](auto pfd, auto, auto) {
    pfd[0].revents = POLLIN | POLLOUT | POLLERR | POLLNVAL;
    return 0;
  });
  ArchNetworkBSD networkBSD(deps);
  PollEntries entries{{nullptr, 0, 0}};

  networkBSD.pollSocket(entries.data(), static_cast<int>(entries.size()), 1);

  const auto expect = IArchNetwork::kPOLLIN | IArchNetwork::kPOLLOUT | IArchNetwork::kPOLLERR | IArchNetwork::kPOLLNVAL;
  EXPECT_EQ(entries[0].m_revents, expect);
}

TEST(ArchNetworkBSDTests, pollSocket_nullSocket_fdIsNegativeOne)
{
  auto deps = MockDeps::makeNice();
  ArchNetworkBSD networkBSD(deps);
  PollEntries entries{{nullptr, 0, 0}};

  networkBSD.pollSocket(entries.data(), static_cast<int>(entries.size()), 1);

  EXPECT_EQ(deps->m_pollFD[0].fd, -1);
}

TEST(ArchNetworkBSDTests, pollSocket_socketSet_fdWasSet)
{
  auto deps = MockDeps::makeNice();
  ArchNetworkBSD networkBSD(deps);
  ArchSocketImpl socket{1, 0};
  PollEntries entries{{&socket, 0, 0}};

  networkBSD.pollSocket(entries.data(), static_cast<int>(entries.size()), 1);

  EXPECT_EQ(deps->m_pollFD[0].fd, 1);
}

TEST(ArchNetworkBSDTests, pollSocket_eventHasPollInBit_bitWasSet)
{
  auto deps = MockDeps::makeNice();
  ArchNetworkBSD networkBSD(deps);
  ArchSocketImpl socket{1, 0};
  PollEntries entries{{&socket, IArchNetwork::kPOLLIN, 0}};

  networkBSD.pollSocket(entries.data(), static_cast<int>(entries.size()), 1);

  EXPECT_EQ(deps->m_pollFD[0].events, POLLIN);
}

TEST(ArchNetworkBSDTests, pollSocket_eventHasPollOutBit_bitWasSet)
{
  auto deps = MockDeps::makeNice();
  ArchNetworkBSD networkBSD(deps);
  ArchSocketImpl socket{1, 0};
  PollEntries entries{{&socket, IArchNetwork::kPOLLOUT, 0}};

  networkBSD.pollSocket(entries.data(), static_cast<int>(entries.size()), 1);

  EXPECT_EQ(deps->m_pollFD[0].events, POLLOUT);
}

TEST(ArchNetworkBSDTests, pollSocket_nullSocket_unblockPipeAppended)
{
  auto deps = MockDeps::makeNice();
  ArchNetworkBSD networkBSD(deps);
  PollEntries entries{{nullptr, 0, 0}};

  networkBSD.pollSocket(entries.data(), static_cast<int>(entries.size()), 1);

  // interesting: unblock pipe fd comes from `getNetworkDataForThread` which
  // seems to differ depending on linux distro.
  EXPECT_GT(deps->m_pollFD[1].fd, -1);
}

TEST(ArchNetworkBSDTests, pollSocket_unblockPipeReventsError_readCalled)
{
  const auto unblockPipeIndex = 1;
  auto deps = MockDeps::makeNice();
  ON_CALL(*deps, poll(_, _, _)).WillByDefault([](auto pfd, auto, auto) {
    pfd[unblockPipeIndex].revents = POLLIN;
    return 1;
  });
  ON_CALL(*deps, read(_, _, _)).WillByDefault([]() {
    errno = EAGAIN;
    return 0;
  });
  ArchNetworkBSD networkBSD(deps);
  PollEntries entries{{nullptr, 0, 0}};

  EXPECT_CALL(*deps, read(_, _, _)).Times(1);
  networkBSD.pollSocket(entries.data(), static_cast<int>(entries.size()), 1);
}

TEST(ArchNetworkBSDTests, pollSocket_interruptSystemCall_testCancelThread)
{
  auto deps = MockDeps::makeNice();
  ON_CALL(*deps, poll(_, _, _)).WillByDefault([]() {
    errno = EINTR;
    return -1;
  });
  ArchNetworkBSD networkBSD(deps);
  PollEntries entries{{nullptr, 0, 0}};

  EXPECT_CALL(*deps, testCancelThread()).Times(1);
  networkBSD.pollSocket(entries.data(), static_cast<int>(entries.size()), 1);
}

TEST(ArchNetworkBSDTests, isAnyAddr_goodAddress_returnsTrue)
{
  auto deps = MockDeps::makeNice();
  ArchNetworkBSD networkBSD(deps);
  std::unique_ptr<ArchNetAddressImpl> addr;
  addr.reset(networkBSD.newAnyAddr(IArchNetwork::kINET6));

  auto result = networkBSD.isAnyAddr(addr.get());

  EXPECT_TRUE(result);
}

TEST(ArchNetworkBSDTests, isAnyAddr_badAddress_returnsFalse)
{
  auto deps = MockDeps::makeNice();
  ArchNetworkBSD networkBSD(deps);
  std::unique_ptr<ArchNetAddressImpl> addr;
  addr.reset(networkBSD.newAnyAddr(IArchNetwork::kINET6));
  auto scratch = (char *)&addr->m_addr;
  std::string badAddr = "badaddr";
  std::ranges::copy(badAddr, scratch + 2);

  auto result = networkBSD.isAnyAddr(addr.get());

  EXPECT_FALSE(result);
}
