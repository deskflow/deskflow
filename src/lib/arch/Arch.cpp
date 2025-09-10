/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/Arch.h"

#include <QSysInfo>

#include <thread>

#if SYSAPI_WIN32
#include "arch/win32/ArchMiscWindows.h"
#endif

//
// Arch
//

Arch *Arch::s_instance = nullptr;

Arch::Arch()
{
  assert(s_instance == nullptr);
  s_instance = this;
}

Arch::Arch(Arch *arch)
{
  s_instance = arch;
}

#if SYSAPI_WIN32
void Arch::init()
{
  ARCH_NETWORK::init();
  ArchMiscWindows::init();
}
#endif

Arch *Arch::getInstance()
{
  assert(s_instance != nullptr);
  return s_instance;
}

void Arch::sleep(double timeout)
{
  ARCH->testCancelThread();
  if (timeout < 0.0)
    return;
  const auto msec = static_cast<uint64_t>(timeout * 1000);
  std::this_thread::sleep_for(std::chrono::milliseconds(msec));
}

double Arch::time()
{
  auto sinceEpoch = std::chrono::steady_clock::now().time_since_epoch();
  auto uSecSinceEpoch = std::chrono::duration_cast<std::chrono::microseconds>(sinceEpoch).count();
  return double(uSecSinceEpoch / 1000000);
}

QString Arch::hostName()
{
  return QSysInfo::machineHostName();
}
