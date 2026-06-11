/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/HidPassthrough.h"

#include "base/Log.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDManager.h>

#include <atomic>
#include <map>
#include <thread>
#include <vector>

namespace deskflow::server {

namespace {

const uint32_t kVendorUsagePageFloor = 0xFF00; // kHIDPage_VendorDefinedStart

int32_t numberProperty(IOHIDDeviceRef device, CFStringRef key)
{
  const auto value = static_cast<CFNumberRef>(IOHIDDeviceGetProperty(device, key));
  if (value == nullptr || CFGetTypeID(value) != CFNumberGetTypeID()) {
    return 0;
  }
  int32_t number = 0;
  CFNumberGetValue(value, kCFNumberSInt32Type, &number);
  return number;
}

std::string stringProperty(IOHIDDeviceRef device, CFStringRef key)
{
  const auto value = static_cast<CFStringRef>(IOHIDDeviceGetProperty(device, key));
  if (value == nullptr || CFGetTypeID(value) != CFStringGetTypeID()) {
    return {};
  }
  char buffer[256] = {0};
  if (!CFStringGetCString(value, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
    return {};
  }
  return buffer;
}

//! Seize-only grabber for vendor-defined HID collections.
/*!
The standard pointer interface is never touched -- only collections whose
primary usage page is vendor-defined (>= 0xFF00, e.g. Logitech HID++).
The seize is an open option the kernel revokes on process exit, so a
crash always returns the device to the host (docs/hid-passthrough.md,
fail-safe guarantees).
*/
class OSXHidGrabber : public IHidGrabber
{
public:
  ~OSXHidGrabber() override
  {
    stop();
  }

  bool start(
      std::vector<HidDeviceSelector> selectors, AttachCallback onAttach, FrameCallback onFrame, DetachCallback onDetach
  ) override
  {
    if (m_thread.joinable()) {
      return true;
    }
    m_selectors = std::move(selectors);
    m_onAttach = std::move(onAttach);
    m_onFrame = std::move(onFrame);
    m_onDetach = std::move(onDetach);
    m_running = true;
    m_thread = std::thread([this] { runLoop(); });
    return true;
  }

  void setSeized(bool seized) override
  {
    // IOHID objects are only safe to drive from the run loop they are
    // scheduled on; marshal the state change there.
    CFRunLoopRef loop = m_runLoop;
    if (loop == nullptr) {
      m_pendingSeize = seized; // run loop not up yet; applied on entry
      return;
    }
    CFRunLoopPerformBlock(loop, kCFRunLoopDefaultMode, ^{
      applySeized(seized);
    });
    CFRunLoopWakeUp(loop);
  }

  void stop() override
  {
    m_running = false;
    if (m_runLoop != nullptr) {
      CFRunLoopStop(m_runLoop);
    }
    if (m_thread.joinable()) {
      m_thread.join();
    }
    m_runLoop = nullptr;
  }

private:
  struct GrabbedDevice
  {
    IOHIDDeviceRef device = nullptr;
    HidDeviceDescriptor descriptor;
    std::vector<uint8_t> reportBuffer;
    bool open = false;
  };

  static void deviceMatched(void *context, IOReturn, void *, IOHIDDeviceRef device)
  {
    static_cast<OSXHidGrabber *>(context)->onDeviceMatched(device);
  }

  static void deviceRemoved(void *context, IOReturn, void *, IOHIDDeviceRef device)
  {
    static_cast<OSXHidGrabber *>(context)->onDeviceRemoved(device);
  }

  static void inputReport(
      void *context, IOReturn result, void *sender, IOHIDReportType, uint32_t reportId, uint8_t *report,
      CFIndex reportLength
  )
  {
    if (result != kIOReturnSuccess || reportLength <= 0) {
      return;
    }
    static_cast<OSXHidGrabber *>(context)->onInputReport(
        static_cast<IOHIDDeviceRef>(sender), reportId, report, static_cast<size_t>(reportLength)
    );
  }

  void onDeviceMatched(IOHIDDeviceRef device)
  {
    const auto usagePage = static_cast<uint32_t>(numberProperty(device, CFSTR(kIOHIDPrimaryUsagePageKey)));
    if (usagePage < kVendorUsagePageFloor) {
      return; // never touch standard collections (pointer, keyboard)
    }

    GrabbedDevice grabbed;
    grabbed.device = device;
    grabbed.descriptor.deviceId = m_nextDeviceId++;
    grabbed.descriptor.vid = static_cast<uint16_t>(numberProperty(device, CFSTR(kIOHIDVendorIDKey)));
    grabbed.descriptor.pid = static_cast<uint16_t>(numberProperty(device, CFSTR(kIOHIDProductIDKey)));
    grabbed.descriptor.usagePage = usagePage;
    grabbed.descriptor.usage = static_cast<uint32_t>(numberProperty(device, CFSTR(kIOHIDPrimaryUsageKey)));
    grabbed.descriptor.name = stringProperty(device, CFSTR(kIOHIDProductKey));
    const auto maxReport = numberProperty(device, CFSTR(kIOHIDMaxInputReportSizeKey));
    grabbed.reportBuffer.resize(maxReport > 0 ? static_cast<size_t>(maxReport) : 64);

    LOG_INFO(
        "hid passthrough: matched \"%s\" (%04x:%04x usage %x:%x)", grabbed.descriptor.name.c_str(),
        grabbed.descriptor.vid, grabbed.descriptor.pid, grabbed.descriptor.usagePage, grabbed.descriptor.usage
    );

    const auto descriptor = grabbed.descriptor;
    m_devices.emplace(device, std::move(grabbed));
    if (m_onAttach) {
      m_onAttach(descriptor);
    }
    if (m_seized) {
      openSeized(m_devices.at(device));
    }
  }

  void onDeviceRemoved(IOHIDDeviceRef device)
  {
    const auto it = m_devices.find(device);
    if (it == m_devices.end()) {
      return;
    }
    const uint16_t deviceId = it->second.descriptor.deviceId;
    closeSeized(it->second);
    m_devices.erase(it);
    LOG_INFO("hid passthrough: device %u unplugged", deviceId);
    if (m_onDetach) {
      m_onDetach(deviceId);
    }
  }

  void onInputReport(IOHIDDeviceRef device, uint32_t reportId, const uint8_t *report, size_t length)
  {
    const auto it = m_devices.find(device);
    if (it == m_devices.end() || !it->second.open || !m_onFrame) {
      return;
    }
    // Consumers (e.g. Mouser's HID++ parser) expect the report id as the
    // first byte; IOKit reports may or may not include it, so normalize.
    std::string bytes;
    if (reportId != 0 && (length == 0 || report[0] != static_cast<uint8_t>(reportId))) {
      bytes.reserve(length + 1);
      bytes.push_back(static_cast<char>(reportId));
      bytes.append(reinterpret_cast<const char *>(report), length);
    } else {
      bytes.assign(reinterpret_cast<const char *>(report), length);
    }
    m_onFrame(it->second.descriptor.deviceId, std::move(bytes));
  }

  void openSeized(GrabbedDevice &grabbed)
  {
    if (grabbed.open) {
      return;
    }
    const IOReturn rc = IOHIDDeviceOpen(grabbed.device, kIOHIDOptionsTypeSeizeDevice);
    if (rc != kIOReturnSuccess) {
      LOG_WARN("hid passthrough: seize of device %u failed (0x%x)", grabbed.descriptor.deviceId, rc);
      return;
    }
    IOHIDDeviceRegisterInputReportCallback(
        grabbed.device, grabbed.reportBuffer.data(), static_cast<CFIndex>(grabbed.reportBuffer.size()), inputReport,
        this
    );
    IOHIDDeviceScheduleWithRunLoop(grabbed.device, m_runLoop, kCFRunLoopDefaultMode);
    grabbed.open = true;
    LOG_INFO("hid passthrough: seized device %u (focus is remote)", grabbed.descriptor.deviceId);
  }

  void closeSeized(GrabbedDevice &grabbed)
  {
    if (!grabbed.open) {
      return;
    }
    IOHIDDeviceRegisterInputReportCallback(
        grabbed.device, grabbed.reportBuffer.data(), static_cast<CFIndex>(grabbed.reportBuffer.size()), nullptr, this
    );
    IOHIDDeviceUnscheduleFromRunLoop(grabbed.device, m_runLoop, kCFRunLoopDefaultMode);
    IOHIDDeviceClose(grabbed.device, kIOHIDOptionsTypeSeizeDevice);
    grabbed.open = false;
    LOG_INFO("hid passthrough: released device %u (focus is local)", grabbed.descriptor.deviceId);
  }

  void applySeized(bool seized)
  {
    if (m_seized == seized) {
      return;
    }
    m_seized = seized;
    for (auto &[device, grabbed] : m_devices) {
      if (seized) {
        openSeized(grabbed);
      } else {
        closeSeized(grabbed);
      }
    }
  }

  void runLoop()
  {
    m_manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDManagerOptionNone);
    if (m_manager == nullptr) {
      LOG_WARN("hid passthrough: could not create IOHIDManager");
      return;
    }

    CFMutableArrayRef matching = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
    for (const auto &selector : m_selectors) {
      CFMutableDictionaryRef dict = CFDictionaryCreateMutable(
          kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks
      );
      const int32_t vid = selector.vid;
      CFNumberRef vidNumber = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &vid);
      CFDictionarySetValue(dict, CFSTR(kIOHIDVendorIDKey), vidNumber);
      CFRelease(vidNumber);
      if (selector.pid != 0) {
        const int32_t pid = selector.pid;
        CFNumberRef pidNumber = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &pid);
        CFDictionarySetValue(dict, CFSTR(kIOHIDProductIDKey), pidNumber);
        CFRelease(pidNumber);
      }
      CFArrayAppendValue(matching, dict);
      CFRelease(dict);
    }
    IOHIDManagerSetDeviceMatchingMultiple(m_manager, matching);
    CFRelease(matching);

    IOHIDManagerRegisterDeviceMatchingCallback(m_manager, deviceMatched, this);
    IOHIDManagerRegisterDeviceRemovalCallback(m_manager, deviceRemoved, this);

    m_runLoop = CFRunLoopGetCurrent();
    IOHIDManagerScheduleWithRunLoop(m_manager, m_runLoop, kCFRunLoopDefaultMode);
    // Discovery-only open: devices are opened individually (with seize)
    // when focus goes remote.
    IOHIDManagerOpen(m_manager, kIOHIDManagerOptionNone);
    LOG_DEBUG("hid passthrough: grabber run loop started");

    if (m_pendingSeize) {
      applySeized(true);
      m_pendingSeize = false;
    }

    while (m_running) {
      CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.25, true);
    }

    applySeized(false);
    IOHIDManagerUnscheduleFromRunLoop(m_manager, m_runLoop, kCFRunLoopDefaultMode);
    IOHIDManagerClose(m_manager, kIOHIDManagerOptionNone);
    CFRelease(m_manager);
    m_manager = nullptr;
    m_devices.clear();
    LOG_DEBUG("hid passthrough: grabber run loop stopped");
  }

  std::vector<HidDeviceSelector> m_selectors;
  AttachCallback m_onAttach;
  FrameCallback m_onFrame;
  DetachCallback m_onDetach;
  std::thread m_thread;
  std::atomic<bool> m_running{false};
  std::atomic<bool> m_pendingSeize{false};
  IOHIDManagerRef m_manager = nullptr;
  CFRunLoopRef m_runLoop = nullptr;
  std::map<IOHIDDeviceRef, GrabbedDevice> m_devices; // run-loop thread only
  bool m_seized = false;                             // run-loop thread only
  uint16_t m_nextDeviceId = 1;                       // run-loop thread only
};

} // namespace

std::unique_ptr<IHidGrabber> createHidGrabber()
{
  return std::make_unique<OSXHidGrabber>();
}

} // namespace deskflow::server
