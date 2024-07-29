/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
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

struct MockDeps : public ArchNetworkBSD::Deps {
  MockDeps() {
    ON_CALL(*this, makePollFD(_)).WillByDefault([this](nfds_t n) {
      m_pollFD = ArchNetworkBSD::Deps::makePollFD(n);
      return m_pollFD;
    });
  }

  MOCK_METHOD(void, sleep, (double), (override));
  MOCK_METHOD(int, poll, (struct pollfd *, nfds_t, int), (override));
  MOCK_METHOD(std::shared_ptr<PollFD>, makePollFD, (nfds_t), (override));
  MOCK_METHOD(ssize_t, read, (int, void *, size_t), (override));
  MOCK_METHOD(void, testCancelThread, (), (override));

  std::shared_ptr<PollFD> m_pollFD;
};

TEST(ArchNetworkBSDTests, pollSocket_zeroEntries_callsSleep) {
  MockDeps deps;
  ArchNetworkBSD networkBSD(deps);

  EXPECT_CALL(deps, sleep(1)).Times(1);
  auto result = networkBSD.pollSocket(nullptr, 0, 1);

  EXPECT_EQ(result, 0);
}

TEST(ArchNetworkBSDTests, pollSocket_mockAccessError_throws) {
  NiceMock<MockDeps> deps;
  ON_CALL(deps, poll(_, _, _)).WillByDefault([]() {
    errno = EACCES;
    return -1;
  });
  ArchNetworkBSD networkBSD(deps);
  PollEntries entries{{nullptr, 0, 0}};

  const auto f = [&] {
    networkBSD.pollSocket(entries.data(), static_cast<int>(entries.size()), 1);
  };

  EXPECT_THROW({ f(); }, XArchNetworkAccess);
}

TEST(ArchNetworkBSDTests, pollSocket_pfdHasRevents_copiedToEntries) {
  NiceMock<MockDeps> deps;
  ON_CALL(deps, poll(_, _, _)).WillByDefault([](auto pfd, auto, auto) {
    pfd[0].revents = POLLIN | POLLOUT | POLLERR | POLLNVAL;
    return 0;
  });
  ArchNetworkBSD networkBSD(deps);
  PollEntries entries{{nullptr, 0, 0}};

  networkBSD.pollSocket(entries.data(), static_cast<int>(entries.size()), 1);

  const auto expect = IArchNetwork::kPOLLIN | IArchNetwork::kPOLLOUT |
                      IArchNetwork::kPOLLERR | IArchNetwork::kPOLLNVAL;
  EXPECT_EQ(entries[0].m_revents, expect);
}

TEST(ArchNetworkBSDTests, pollSocket_nullSocket_fdIsNegativeOne) {
  NiceMock<MockDeps> deps;
  ArchNetworkBSD networkBSD(deps);
  PollEntries entries{{nullptr, 0, 0}};

  networkBSD.pollSocket(entries.data(), static_cast<int>(entries.size()), 1);

  EXPECT_EQ(deps.m_pollFD[0].fd, -1);
}

TEST(ArchNetworkBSDTests, pollSocket_socketSet_fdWasSet) {
  NiceMock<MockDeps> deps;
  ArchNetworkBSD networkBSD(deps);
  ArchSocketImpl socket{1, 0};
  PollEntries entries{{&socket, 0, 0}};

  networkBSD.pollSocket(entries.data(), static_cast<int>(entries.size()), 1);

  EXPECT_EQ(deps.m_pollFD[0].fd, 1);
}

TEST(ArchNetworkBSDTests, pollSocket_eventHasPollInBit_bitWasSet) {
  NiceMock<MockDeps> deps;
  ArchNetworkBSD networkBSD(deps);
  ArchSocketImpl socket{1, 0};
  PollEntries entries{{&socket, IArchNetwork::kPOLLIN, 0}};

  networkBSD.pollSocket(entries.data(), static_cast<int>(entries.size()), 1);

  EXPECT_EQ(deps.m_pollFD[0].events, POLLIN);
}

TEST(ArchNetworkBSDTests, pollSocket_eventHasPollOutBit_bitWasSet) {
  NiceMock<MockDeps> deps;
  ArchNetworkBSD networkBSD(deps);
  ArchSocketImpl socket{1, 0};
  PollEntries entries{{&socket, IArchNetwork::kPOLLOUT, 0}};

  networkBSD.pollSocket(entries.data(), static_cast<int>(entries.size()), 1);

  EXPECT_EQ(deps.m_pollFD[0].events, POLLOUT);
}

TEST(ArchNetworkBSDTests, pollSocket_nullSocket_unblockPipeAppended) {
  NiceMock<MockDeps> deps;
  ArchNetworkBSD networkBSD(deps);
  PollEntries entries{{nullptr, 0, 0}};

  networkBSD.pollSocket(entries.data(), static_cast<int>(entries.size()), 1);

  // interesting: unblock pipe fd comes from `getNetworkDataForThread` which
  // seems to differ depending on linux distro.
  EXPECT_GT(deps.m_pollFD[1].fd, -1);
}

TEST(ArchNetworkBSDTests, pollSocket_unblockPipeReventsError_readCalled) {
  const auto unblockPipeIndex = 1;
  NiceMock<MockDeps> deps;
  ON_CALL(deps, poll(_, _, _)).WillByDefault([](auto pfd, auto, auto) {
    pfd[unblockPipeIndex].revents = POLLIN;
    return 1;
  });
  ON_CALL(deps, read(_, _, _)).WillByDefault([]() {
    errno = EAGAIN;
    return 0;
  });
  ArchNetworkBSD networkBSD(deps);
  PollEntries entries{{nullptr, 0, 0}};

  EXPECT_CALL(deps, read(_, _, _)).Times(1);
  networkBSD.pollSocket(entries.data(), static_cast<int>(entries.size()), 1);
}

TEST(ArchNetworkBSDTests, pollSocket_interruptSystemCall_testCancelThread) {
  NiceMock<MockDeps> deps;
  ON_CALL(deps, poll(_, _, _)).WillByDefault([]() {
    errno = EINTR;
    return -1;
  });
  ArchNetworkBSD networkBSD(deps);
  PollEntries entries{{nullptr, 0, 0}};

  EXPECT_CALL(deps, testCancelThread()).Times(1);
  networkBSD.pollSocket(entries.data(), static_cast<int>(entries.size()), 1);
}

TEST(ArchNetworkBSDTests, isAnyAddr_goodAddress_returnsTrue) {
  MockDeps deps;
  ArchNetworkBSD networkBSD(deps);
  std::unique_ptr<ArchNetAddressImpl> addr;
  addr.reset(networkBSD.newAnyAddr(IArchNetwork::kINET6));

  auto result = networkBSD.isAnyAddr(addr.get());

  EXPECT_TRUE(result);
}

TEST(ArchNetworkBSDTests, isAnyAddr_badAddress_returnsFalse) {
  MockDeps deps;
  ArchNetworkBSD networkBSD(deps);
  std::unique_ptr<ArchNetAddressImpl> addr;
  addr.reset(networkBSD.newAnyAddr(IArchNetwork::kINET6));
  auto scratch = (char *)&addr->m_addr;
  std::string badAddr = "badaddr";
  std::ranges::copy(badAddr, scratch + 2);

  auto result = networkBSD.isAnyAddr(addr.get());

  EXPECT_FALSE(result);
}
