/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
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
#include "shared/ExitTimeout.h"

#if SYSAPI_WIN32
#include "arch/win32/ArchMiscWindows.h"
#endif

#include <filesystem>
#include <gtest/gtest.h>

using deskflow::test::ExitTimeout;

const auto testDir = "tmp/test";

int main(int argc, char **argv)
{
  // HACK: Unit tests should not use the filesystem.
  std::filesystem::create_directories(testDir);

  ExitTimeout exitTimeout(1, "Integration tests");

#if SYSAPI_WIN32
  // record window instance for tray icon, etc
  ArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif

  Arch arch;
  arch.init();

  Log log;
  log.setFilter(kDEBUG2);

  ::testing::GTEST_FLAG(throw_on_failure) = true;
  testing::InitGoogleTest(&argc, argv);

  // return code 1 means the test failed.
  // any other non-zero code is probably a memory error.
  return RUN_ALL_TESTS();
}
