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

#include <fstream>
#include <gtest/gtest.h>
#include <iostream>

#define LOCK_TIMEOUT 30

void lock(const std::string &lockFile);
void unlock(const std::string &lockFile);

int main(int argc, char **argv) {
#if SYSAPI_WIN32
  // record window instance for tray icon, etc
  ArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif

  Arch arch;
  arch.init();

  Log log;
  log.setFilter(kDEBUG2);

  std::string lockFile;
  for (int i = 0; i < argc; i++) {
    const std::string option(argv[i]);
    if (option.find("--lock-file") != std::string::npos) {
      lockFile = argv[i + 1];
    }
  }

  if (!lockFile.empty()) {
    lock(lockFile);
  }

  ::testing::GTEST_FLAG(throw_on_failure) = true;
  testing::InitGoogleTest(&argc, argv);

  int result = RUN_ALL_TESTS();

  if (!lockFile.empty()) {
    unlock(lockFile);
  }

  // return code 1 means the test failed.
  // any other non-zero code is probably a memory error.
  return result;
}

void lock(const std::string &lockFile) {
  double start = ARCH->time();

  // keep checking until timeout is reached.
  while ((ARCH->time() - start) < LOCK_TIMEOUT) {

    std::ifstream is(lockFile.c_str());
    bool noLock = !is;
    is.close();

    if (noLock) {
      break;
    }

    // check every second if file has gone.
    ARCH->sleep(1);
  }

  // write empty lock file.
  std::ofstream os(lockFile.c_str());
  os.close();
}

void unlock(const std::string &lockFile) { remove(lockFile.c_str()); }
