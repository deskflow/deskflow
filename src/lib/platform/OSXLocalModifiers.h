/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <ApplicationServices/ApplicationServices.h>
#include <IOKit/hid/IOHIDManager.h>

#include <mutex>
#include <vector>

// Read physical keyboard elements, rather than ambient Quartz flags which also
// contain Deskflow's injected keys. No pressed-key cache can survive a missed release.
class OSXLocalModifiers
{
public:
  void start(CFRunLoopRef runLoop);
  void stop(CFRunLoopRef runLoop);
  CGEventFlags flags() const;

private:
  struct Keyboard
  {
    IOHIDDeviceRef device;
    CFArrayRef elements;
  };

  void clearDevices() const;

  mutable std::mutex m_mutex;
  IOHIDManagerRef m_manager = nullptr;
  mutable CFSetRef m_devices = nullptr;
  mutable std::vector<Keyboard> m_keyboards;
};
