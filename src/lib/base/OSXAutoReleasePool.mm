/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#import "base/OSXAutoReleasePool.h"

#import <Foundation/Foundation.h>

namespace deskflow {

bool runInAutoReleasePool(const std::function<bool()> &fn)
{
  @autoreleasepool {
    return fn();
  }
}

} // namespace deskflow
