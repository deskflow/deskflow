/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#import <Carbon/Carbon.h>
#import <CoreFoundation/CoreFoundation.h>

#include "deskflow/key_types.h"

#if defined(__cplusplus)
extern "C"
{
#endif
  bool fakeNativeMediaKey(KeyID id);
  bool isMediaKeyEvent(CGEventRef event);
  bool getMediaKeyEventInfo(CGEventRef event, KeyID *keyId, bool *down, bool *isRepeat);
#if defined(__cplusplus)
}
#endif
