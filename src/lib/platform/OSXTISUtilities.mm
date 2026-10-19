/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXTISUtilities.h"

#include "platform/OSXAutoTypes.h"

#include <dispatch/dispatch.h>
#include <pthread.h>

void runTISOnMainThread(const std::function<void()> &task)
{
  if (pthread_main_np()) {
    std::lock_guard<std::mutex> lock(g_tisMutex);
    task();
    return;
  }

  dispatch_sync(dispatch_get_main_queue(), ^{
    std::lock_guard<std::mutex> lock(g_tisMutex);
    task();
  });
}
