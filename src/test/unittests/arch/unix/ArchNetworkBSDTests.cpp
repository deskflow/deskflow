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

#ifndef _WIN32
#include <sys/types.h>
#include <netinet/in.h>
#include <array>
#include <memory>
#include "lib/arch/unix/ArchNetworkBSD.h"
#include "lib/arch/XArch.h"
#include "test/global/gtest.h"

TEST(ArchNetworkBSDTests, pollSocket_errs_EACCES)
{
    ArchNetworkBSD networkBSD;
    ArchNetworkBSD::s_connectors.poll_impl = [](struct pollfd *, nfds_t, int){ errno = EACCES; return -1; };
    try {
        std::array<IArchNetwork::PollEntry,2>  pe {{nullptr,0,0}};
        networkBSD.pollSocket(pe.data(), pe.size(), 1);
        FAIL() << "Expected to throw";
    }
    catch(XArchNetworkAccess const &err)
    {
        EXPECT_STREQ(err.what(), "Permission denied");
    }
    catch(std::runtime_error const &baseerr) {
        FAIL() << "Expected to throw XArchNetworkAccess but got " << baseerr.what();
    }
}

TEST(ArchNetworkBSDTests, isAnyAddr_IP6)
{
    ArchNetworkBSD networkBSD;
    std::unique_ptr<ArchNetAddressImpl> addr;
    addr.reset(networkBSD.newAnyAddr(IArchNetwork::kINET6));
    EXPECT_TRUE(networkBSD.isAnyAddr(addr.get()));

    auto scratch = (char *)&addr->m_addr;
    scratch[2] = 'b';
    scratch[3] = 'a';
    scratch[4] = 'd';
    scratch[5] = 'a';
    scratch[6] = 'd';
    scratch[7] = 'd';
    scratch[8] = 'r';
    EXPECT_FALSE(networkBSD.isAnyAddr(addr.get()));
}

#endif // #ifdnef _WIN32
