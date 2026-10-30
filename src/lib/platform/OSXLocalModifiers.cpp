/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXLocalModifiers.h"

#include "base/Log.h"

#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/hid/IOHIDUsageTables.h>

void OSXLocalModifiers::start(CFRunLoopRef runLoop)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_manager)
    return;
  m_manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
  if (!m_manager)
    return;
  const int page = kHIDPage_GenericDesktop;
  const int usage = kHIDUsage_GD_Keyboard;
  CFNumberRef pageNumber = CFNumberCreate(nullptr, kCFNumberIntType, &page);
  CFNumberRef usageNumber = CFNumberCreate(nullptr, kCFNumberIntType, &usage);
  const void *keys[] = {CFSTR(kIOHIDDeviceUsagePageKey), CFSTR(kIOHIDDeviceUsageKey)};
  const void *values[] = {pageNumber, usageNumber};
  CFDictionaryRef matching =
      CFDictionaryCreate(nullptr, keys, values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  IOHIDManagerSetDeviceMatching(m_manager, matching);
  CFRelease(matching);
  CFRelease(pageNumber);
  CFRelease(usageNumber);
  const auto result = IOHIDManagerOpen(m_manager, kIOHIDOptionsTypeNone);
  if (result != kIOReturnSuccess) {
    LOG_WARN("cannot read local keyboard modifiers (HID error 0x%x); check Input Monitoring permission", result);
    CFRelease(m_manager);
    m_manager = nullptr;
    return;
  }
  // Scheduling keeps the device set current when keyboards are connected or removed.
  IOHIDManagerScheduleWithRunLoop(m_manager, runLoop, kCFRunLoopDefaultMode);
}

void OSXLocalModifiers::clearDevices() const
{
  for (const auto &keyboard : m_keyboards)
    CFRelease(keyboard.elements);
  m_keyboards.clear();
  if (m_devices) {
    CFRelease(m_devices);
    m_devices = nullptr;
  }
}

void OSXLocalModifiers::stop(CFRunLoopRef runLoop)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  clearDevices();
  if (m_manager) {
    IOHIDManagerUnscheduleFromRunLoop(m_manager, runLoop, kCFRunLoopDefaultMode);
    IOHIDManagerClose(m_manager, kIOHIDOptionsTypeNone);
    CFRelease(m_manager);
    m_manager = nullptr;
  }
}

CGEventFlags OSXLocalModifiers::flags() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_manager)
    return 0;
  CFSetRef devices = IOHIDManagerCopyDevices(m_manager);
  if (!devices) {
    clearDevices();
    return 0;
  }
  if (!m_devices || !CFEqual(devices, m_devices)) {
    clearDevices();
    m_devices = devices;
    std::vector<const void *> entries(CFSetGetCount(devices));
    CFSetGetValues(devices, entries.data());
    for (const auto entry : entries) {
      auto device = static_cast<IOHIDDeviceRef>(const_cast<void *>(entry));
      CFArrayRef all = IOHIDDeviceCopyMatchingElements(device, nullptr, kIOHIDOptionsTypeNone);
      if (!all)
        continue;
      CFMutableArrayRef modifiers = CFArrayCreateMutable(nullptr, 0, &kCFTypeArrayCallBacks);
      for (CFIndex i = 0; i < CFArrayGetCount(all); ++i) {
        auto element = static_cast<IOHIDElementRef>(const_cast<void *>(CFArrayGetValueAtIndex(all, i)));
        const auto type = IOHIDElementGetType(element);
        const auto usage = IOHIDElementGetUsage(element);
        if ((type == kIOHIDElementTypeInput_Button || type == kIOHIDElementTypeInput_ScanCodes) &&
            IOHIDElementGetUsagePage(element) == kHIDPage_KeyboardOrKeypad && usage >= kHIDUsage_KeyboardLeftControl &&
            usage <= kHIDUsage_KeyboardRightGUI)
          CFArrayAppendValue(modifiers, element);
      }
      CFRelease(all);
      m_keyboards.push_back({device, modifiers});
    }
  } else {
    CFRelease(devices);
  }

  const CGEventFlags modifierFlags[] = {
      kCGEventFlagMaskControl, kCGEventFlagMaskShift, kCGEventFlagMaskAlternate, kCGEventFlagMaskCommand
  };
  CGEventFlags result = 0;
  for (const auto &keyboard : m_keyboards) {
    for (CFIndex i = 0; i < CFArrayGetCount(keyboard.elements); ++i) {
      auto element = static_cast<IOHIDElementRef>(const_cast<void *>(CFArrayGetValueAtIndex(keyboard.elements, i)));
      IOHIDValueRef value = nullptr;
      if (IOHIDDeviceGetValue(keyboard.device, element, &value) == kIOReturnSuccess && value &&
          IOHIDValueGetIntegerValue(value) != 0)
        result |= modifierFlags[(IOHIDElementGetUsage(element) - kHIDUsage_KeyboardLeftControl) % 4];
    }
  }
  return result;
}
