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
#include "test/global/gtest.h"

TEST(ArchNetworkBSDTests, pollSocket_errs_EACCES)
{
    ArchNetworkBSD networkBSD;
    ArchNetworkBSD::s_connectors.poll_impl = [](struct pollfd *, nfds_t, int){ errno = EACCES; return -1; };
    try {
        IArchNetwork::PollEntry pe[1] = {{nullptr,0,0}};
        networkBSD.pollSocket(pe, 2, 1);
        FAIL() << "Expected to throw";
    }
    catch(XArchNetworkAccess const &err)
    {
    }
    catch() {
        FAIL() << "Expected to throw XArchNetworkAccess"
    }
}
