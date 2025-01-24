/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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
