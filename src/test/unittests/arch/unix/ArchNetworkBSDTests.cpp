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

#include "lib/arch/unix/ArchNetworkBSD.h"

#include "lib/arch/XArch.h"

#include <array>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <netinet/in.h>
#include <sys/types.h>

using ::testing::_;

struct MockDeps : public ArchNetworkBSD::Deps {
  MockDeps() {
    ON_CALL(*this, makePollFD(_)).WillByDefault([this](nfds_t n) {
      return ArchNetworkBSD::Deps::makePollFD(n);
    });
  }

  MOCK_METHOD(void, sleep, (double), (override));
  MOCK_METHOD(int, poll, (struct pollfd *, nfds_t, int), (override));
  MOCK_METHOD(
      std::unique_ptr<struct pollfd[]>, makePollFD, (nfds_t), (override));
};

TEST(ArchNetworkBSDTests, pollSocket_zeroEntries_callsSleep) {
  MockDeps deps;
  ArchNetworkBSD networkBSD(deps);

  EXPECT_CALL(deps, sleep(1)).Times(1);
  auto result = networkBSD.pollSocket(nullptr, 0, 1);

  EXPECT_EQ(result, 0);
}

TEST(ArchNetworkBSDTests, pollSocket_stubEntry_throwsAccessError) {
  MockDeps deps;
  ON_CALL(deps, poll(_, _, _)).WillByDefault([]() {
    errno = EACCES;
    return -1;
  });
  ArchNetworkBSD networkBSD(deps);
  std::array<IArchNetwork::PollEntry, 2> entries{{nullptr, 0, 0}};

  const auto f = [&] {
    networkBSD.pollSocket(entries.data(), entries.size(), 1);
  };

  EXPECT_THROW({ f(); }, XArchNetworkAccess);
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
