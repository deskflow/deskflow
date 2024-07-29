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
#include <gtest/gtest.h>
#include <memory>
#include <netinet/in.h>
#include <sys/types.h>

TEST(ArchNetworkBSDTests, pollSocket_nullSocketEntry_throwsAccessError) {
  auto pollMock = [](struct pollfd const *, nfds_t, int) {
    errno = EACCES;
    return -1;
  };
  ArchNetworkBSD networkBSD(pollMock);
  std::array<IArchNetwork::PollEntry, 2> pollEntries{{nullptr, 0, 0}};

  const auto f = [&] {
    networkBSD.pollSocket(pollEntries.data(), pollEntries.size(), 1);
  };

  EXPECT_THROW({ f(); }, XArchNetworkAccess);
}

TEST(ArchNetworkBSDTests, isAnyAddr_goodAddress_returnsTrue) {
  ArchNetworkBSD networkBSD;
  std::unique_ptr<ArchNetAddressImpl> addr;
  addr.reset(networkBSD.newAnyAddr(IArchNetwork::kINET6));

  auto result = networkBSD.isAnyAddr(addr.get());

  EXPECT_TRUE(result);
}

TEST(ArchNetworkBSDTests, isAnyAddr_badAddress_returnsFalse) {
  ArchNetworkBSD networkBSD;
  std::unique_ptr<ArchNetAddressImpl> addr;
  addr.reset(networkBSD.newAnyAddr(IArchNetwork::kINET6));
  auto scratch = (char *)&addr->m_addr;
  std::string badAddr = "badaddr";
  std::ranges::copy(badAddr, scratch + 2);

  auto result = networkBSD.isAnyAddr(addr.get());

  EXPECT_FALSE(result);
}
