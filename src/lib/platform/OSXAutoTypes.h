/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2022 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once

#include <Carbon/Carbon.h>
#include <dispatch/dispatch.h>
#include <memory>
#include <mutex>
#include <pthread.h>

using CFDeallocator = decltype(&CFRelease);
using AutoCFArray = std::unique_ptr<const __CFArray, CFDeallocator>;
using AutoCFDictionary = std::unique_ptr<const __CFDictionary, CFDeallocator>;
using AutoTISInputSourceRef = std::unique_ptr<__TISInputSource, CFDeallocator>;

inline std::mutex g_tisMutex;

// macOS 27 hard-asserts (dispatch_assert_queue) that TIS/TSM input-source APIs
// run on the main dispatch queue. Callers already serialize access to these
// APIs via g_tisMutex; wrap that critical section with this helper so it also
// runs on the main queue when called from a background thread.
template <typename Func> inline void runOnMainTISQueue(Func &&func)
{
  if (pthread_main_np()) {
    func();
  } else {
    dispatch_sync(dispatch_get_main_queue(), ^{
      func();
    });
  }
}
