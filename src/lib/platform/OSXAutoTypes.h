/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2022 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once

#include <Carbon/Carbon.h>
#include <dispatch/dispatch.h>
#include <functional>
#include <memory>
#include <mutex>
#include <pthread.h>

using CFDeallocator = decltype(&CFRelease);
using AutoCFArray = std::unique_ptr<const __CFArray, CFDeallocator>;
using AutoCFDictionary = std::unique_ptr<const __CFDictionary, CFDeallocator>;
using AutoTISInputSourceRef = std::unique_ptr<__TISInputSource, CFDeallocator>;

inline std::mutex g_tisMutex;

// macOS 26+ traps if Text Input Source calls run off the main queue.
inline void runOnMainQueue(std::function<void()> fn)
{
  if (pthread_main_np() != 0) {
    fn();
    return;
  }

  auto *job = new std::function<void()>(std::move(fn));
  dispatch_sync_f(dispatch_get_main_queue(), job, [](void *context) {
    std::unique_ptr<std::function<void()>> owned(static_cast<std::function<void()> *>(context));
    (*owned)();
  });
}
