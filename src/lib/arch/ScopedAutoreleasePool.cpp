/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/ScopedAutoreleasePool.h"

#if defined(__APPLE__)

// Declared rather than included so this stays a plain C++ translation unit.
extern "C" void *objc_autoreleasePoolPush(void);
extern "C" void objc_autoreleasePoolPop(void *pool);

namespace deskflow {

ScopedAutoreleasePool::ScopedAutoreleasePool() : m_pool(objc_autoreleasePoolPush())
{
  // do nothing
}

ScopedAutoreleasePool::~ScopedAutoreleasePool()
{
  objc_autoreleasePoolPop(m_pool);
}

} // namespace deskflow

#endif
