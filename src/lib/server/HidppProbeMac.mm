/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/HidppProbe.h"

#include "base/Log.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDManager.h>

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <vector>

namespace deskflow::server {

namespace {

constexpr uint8_t kLongId = 0x11;
constexpr size_t kLongLen = 20;
constexpr uint8_t kMySw = 0x0A;
constexpr uint16_t kFeatReprogV4 = 0x1B04;
constexpr uint32_t kVendorUsagePageFloor = 0xFF00;

struct ProbeSession
{
  IOHIDDeviceRef device = nullptr;
  std::mutex mutex;
  std::condition_variable cv;
  std::vector<uint8_t> lastReport;
  bool gotReport = false;
};

static void inputCallback(
    void *context, IOReturn result, void *, IOHIDReportType, uint32_t reportId, uint8_t *report, CFIndex reportLength
)
{
  if (result != kIOReturnSuccess || reportLength <= 0) {
    return;
  }
  auto *session = static_cast<ProbeSession *>(context);
  std::vector<uint8_t> bytes;
  if (reportId != 0 && (reportLength == 0 || report[0] != static_cast<uint8_t>(reportId))) {
    bytes.push_back(static_cast<uint8_t>(reportId));
  }
  bytes.insert(bytes.end(), report, report + reportLength);
  {
    std::scoped_lock lock{session->mutex};
    session->lastReport = std::move(bytes);
    session->gotReport = true;
  }
  session->cv.notify_one();
}

bool sendLongReport(ProbeSession &session, uint8_t devIdx, uint8_t feat, uint8_t func, const uint8_t *params, size_t paramLen)
{
  uint8_t buf[kLongLen] = {0};
  buf[0] = kLongId;
  buf[1] = devIdx;
  buf[2] = feat;
  buf[3] = static_cast<uint8_t>(((func & 0x0F) << 4) | (kMySw & 0x0F));
  for (size_t i = 0; i < paramLen && i + 4 < kLongLen; ++i) {
    buf[4 + i] = params[i];
  }
  const IOReturn rc = IOHIDDeviceSetReport(
      session.device, kIOHIDReportTypeOutput, kLongId, buf, static_cast<CFIndex>(kLongLen)
  );
  return rc == kIOReturnSuccess;
}

bool parseMessage(const std::vector<uint8_t> &raw, uint8_t &feat, uint8_t &func, uint8_t &sw, std::vector<uint8_t> &params)
{
  if (raw.size() < 4) {
    return false;
  }
  size_t off = (raw[0] == kLongId || raw[0] == 0x10) ? 1 : 0;
  if (off + 3 > raw.size()) {
    return false;
  }
  feat = raw[off + 1];
  const uint8_t fsw = raw[off + 2];
  func = (fsw >> 4) & 0x0F;
  sw = fsw & 0x0F;
  params.assign(raw.begin() + static_cast<std::ptrdiff_t>(off + 3), raw.end());
  return true;
}

bool waitForReport(ProbeSession &session, int timeoutMs)
{
  session.gotReport = false;
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
  while (std::chrono::steady_clock::now() < deadline) {
    {
      std::scoped_lock lock{session.mutex};
      if (session.gotReport) {
        return true;
      }
    }
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.01, true);
  }
  return false;
}

int findFeatureIndex(ProbeSession &session, uint8_t devIdx, uint16_t featureId, int timeoutMs)
{
  const uint8_t params[3] = {
      static_cast<uint8_t>((featureId >> 8) & 0xFF),
      static_cast<uint8_t>(featureId & 0xFF),
      0x00,
  };
  if (!sendLongReport(session, devIdx, 0x00, 0x00, params, sizeof(params))) {
    return -1;
  }
  if (!waitForReport(session, timeoutMs)) {
    return -1;
  }
  std::vector<uint8_t> raw;
  {
    std::scoped_lock lock{session.mutex};
    raw = session.lastReport;
    session.gotReport = false;
  }
  uint8_t feat = 0;
  uint8_t func = 0;
  uint8_t sw = 0;
  std::vector<uint8_t> respParams;
  if (!parseMessage(raw, feat, func, sw, respParams)) {
    return -1;
  }
  if (feat == 0xFF || sw != kMySw || func != 0x00) {
    return -1;
  }
  if (respParams.empty() || respParams[0] == 0) {
    return -1;
  }
  return respParams[0];
}

IOHIDDeviceRef openVendorDevice(uint16_t vendorId, uint16_t productId)
{
  IOHIDManagerRef manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
  if (manager == nullptr) {
    return nullptr;
  }

  const int vid = vendorId;
  const int pid = productId;
  CFNumberRef vidRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &vid);
  CFNumberRef pidRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &pid);
  const void *keys[] = {CFSTR(kIOHIDVendorIDKey), CFSTR(kIOHIDProductIDKey)};
  const void *values[] = {vidRef, pidRef};
  CFDictionaryRef match = CFDictionaryCreate(kCFAllocatorDefault, keys, values, 2, nullptr, nullptr);
  IOHIDManagerSetDeviceMatching(manager, match);
  CFRelease(match);
  CFRelease(vidRef);
  CFRelease(pidRef);

  IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone);
  CFSetRef deviceSet = IOHIDManagerCopyDevices(manager);
  IOHIDManagerClose(manager, kIOHIDOptionsTypeNone);
  CFRelease(manager);
  if (deviceSet == nullptr) {
    return nullptr;
  }

  IOHIDDeviceRef chosen = nullptr;
  const CFIndex count = CFSetGetCount(deviceSet);
  std::vector<const void *> devicePtrs(static_cast<size_t>(count));
  CFSetGetValues(deviceSet, devicePtrs.data());
  for (const void *ptr : devicePtrs) {
    IOHIDDeviceRef device = static_cast<IOHIDDeviceRef>(const_cast<void *>(ptr));
    CFTypeRef upRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDPrimaryUsagePageKey));
    int32_t usagePage = 0;
    if (upRef != nullptr && CFGetTypeID(upRef) == CFNumberGetTypeID()) {
      CFNumberGetValue(static_cast<CFNumberRef>(upRef), kCFNumberSInt32Type, &usagePage);
    }
    if (static_cast<uint32_t>(usagePage) < kVendorUsagePageFloor) {
      continue;
    }
    chosen = device;
    CFRetain(chosen);
    break;
  }
  CFRelease(deviceSet);
  return chosen;
}

} // namespace

HidppDecodeContext probeHidppDecodePlatform(uint16_t vendorId, uint16_t productId)
{
  HidppDecodeContext context;
  IOHIDDeviceRef device = openVendorDevice(vendorId, productId);
  if (device == nullptr) {
    LOG_DEBUG("hidpp probe(mac): no vendor interface for PID=0x%04X", productId);
    return context;
  }

  if (IOHIDDeviceOpen(device, kIOHIDOptionsTypeNone) != kIOReturnSuccess) {
    LOG_DEBUG("hidpp probe(mac): open failed for PID=0x%04X", productId);
    CFRelease(device);
    return context;
  }

  ProbeSession session;
  session.device = device;
  uint8_t buffer[kLongLen] = {0};
  IOHIDDeviceRegisterInputReportCallback(
      device, buffer, static_cast<CFIndex>(kLongLen), inputCallback, &session
  );
  IOHIDDeviceScheduleWithRunLoop(device, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

  static const uint8_t kDevIdxOrder[] = {0xFF, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
  for (const uint8_t devIdx : kDevIdxOrder) {
    const int featIdx = findFeatureIndex(session, devIdx, kFeatReprogV4, 400);
    if (featIdx > 0) {
      context.featIdx = featIdx;
      break;
    }
  }

  IOHIDDeviceUnscheduleFromRunLoop(device, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
  IOHIDDeviceClose(device, kIOHIDOptionsTypeNone);
  CFRelease(device);
  return context;
}

} // namespace deskflow::server
