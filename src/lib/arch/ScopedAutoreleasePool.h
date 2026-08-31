/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

namespace deskflow {

//! Scoped Objective-C autorelease pool
/*!
Pushes a pool on construction, pops it on destruction. Usable from plain C++,
so translation units need not be Objective-C++. Empty on non-Apple platforms.
*/
#if defined(__APPLE__)
class ScopedAutoreleasePool
{
public:
  ScopedAutoreleasePool();
  ~ScopedAutoreleasePool();

  ScopedAutoreleasePool(const ScopedAutoreleasePool &) = delete;
  ScopedAutoreleasePool &operator=(const ScopedAutoreleasePool &) = delete;
  ScopedAutoreleasePool(ScopedAutoreleasePool &&) = delete;
  ScopedAutoreleasePool &operator=(ScopedAutoreleasePool &&) = delete;

private:
  void *m_pool;
};
#else
class ScopedAutoreleasePool
{
public:
  // User-provided rather than defaulted so the unused local in dispatchEvent
  // does not trip -Wunused-variable on non-Apple builds.
  ScopedAutoreleasePool()
  {
    // do nothing
  }
  ~ScopedAutoreleasePool()
  {
    // do nothing
  }

  ScopedAutoreleasePool(const ScopedAutoreleasePool &) = delete;
  ScopedAutoreleasePool &operator=(const ScopedAutoreleasePool &) = delete;
  ScopedAutoreleasePool(ScopedAutoreleasePool &&) = delete;
  ScopedAutoreleasePool &operator=(ScopedAutoreleasePool &&) = delete;
};
#endif

} // namespace deskflow
