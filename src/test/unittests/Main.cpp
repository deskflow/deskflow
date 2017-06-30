/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2011 Nick Bolton
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

#include "arch/Arch.h"
#include "base/Log.h"

#if SYSAPI_WIN32
#include "arch/win32/ArchMiscWindows.h"
#endif

#include "test/global/gtest.h"

int
main (int argc, char** argv) {
#if SYSAPI_WIN32
    // HACK: shouldn't be needed, but logging fails without this.
    ArchMiscWindows::setInstanceWin32 (GetModuleHandle (NULL));
#endif

    Arch arch;
    arch.init ();

    Log log;
    log.setFilter (kDEBUG4);

    testing::InitGoogleTest (&argc, argv);

    // gtest seems to randomly finish with error codes (e.g. -1, -1073741819)
    // even when no tests have failed. not sure what causes this, but it
    // happens on all platforms and  keeps leading to false positives.
    // according to the documentation, 1 is a failure, so we should be
    // able to trust that code.
    return (RUN_ALL_TESTS () == 1) ? 1 : 0;
}
