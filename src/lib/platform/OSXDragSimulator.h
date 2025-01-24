/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/common.h"

#import <CoreFoundation/CoreFoundation.h>

#if defined(__cplusplus)
extern "C"
{
#endif
  void runCocoaApp();
  void stopCocoaLoop();
  void fakeDragging(const char *str, int cursorX, int cursorY);
  CFStringRef getCocoaDropTarget();

#if defined(__cplusplus)
}
#endif
