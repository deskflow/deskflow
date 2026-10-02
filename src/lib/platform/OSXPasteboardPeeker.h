/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#import <CoreFoundation/CoreFoundation.h>

#if defined(__cplusplus)
extern "C"
{
#endif

  // Returns the dragged file URL as a CFString (caller owns it: CFRelease when
  // done). Returns NULL when there is no drag or resolution fails. The result
  // is a copy made via CFStringCreateWithCString, so ownership is explicit and
  // independent of ARC — never a dangling pointer. Never throws/crashes.
  CFStringRef getDraggedFileURL();

#if defined(__cplusplus)
}
#endif
