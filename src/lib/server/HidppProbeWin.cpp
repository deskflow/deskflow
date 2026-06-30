/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/HidppProbe.h"

#include "base/Log.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <hidpi.h>
#include <hidsdi.h>
#include <setupapi.h>

#include <cstring>
#include <string>
#include <vector>

namespace deskflow::server {

namespace {

constexpr uint8_t kLongId = 0x11;
constexpr size_t kLongLen = 20;
constexpr uint8_t kMySw = 0x0A;
constexpr uint16_t kFeatReprogV4 = 0x1B04;
constexpr uint32_t kVendorUsagePageFloor = 0xFF00;

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

bool sendLongReport(HANDLE handle, uint8_t devIdx, uint8_t feat, uint8_t func, const uint8_t *params, size_t paramLen)
{
  uint8_t buf[kLongLen] = {0};
  buf[0] = kLongId;
  buf[1] = devIdx;
  buf[2] = feat;
  buf[3] = static_cast<uint8_t>(((func & 0x0F) << 4) | (kMySw & 0x0F));
  for (size_t i = 0; i < paramLen && i + 4 < kLongLen; ++i) {
    buf[4 + i] = params[i];
  }
  DWORD written = 0;
  return WriteFile(handle, buf, static_cast<DWORD>(kLongLen), &written, nullptr) && written == kLongLen;
}

bool readReport(HANDLE handle, std::vector<uint8_t> &out, int timeoutMs)
{
  const DWORD deadline = GetTickCount() + static_cast<DWORD>(timeoutMs);
  uint8_t buffer[64] = {0};
  while (GetTickCount() < deadline) {
    DWORD read = 0;
    if (ReadFile(handle, buffer, sizeof(buffer), &read, nullptr) && read > 0) {
      out.assign(buffer, buffer + read);
      return true;
    }
    Sleep(10);
  }
  return false;
}

int findFeatureIndex(HANDLE handle, uint8_t devIdx, uint16_t featureId, int timeoutMs)
{
  const uint8_t params[3] = {
      static_cast<uint8_t>((featureId >> 8) & 0xFF),
      static_cast<uint8_t>(featureId & 0xFF),
      0x00,
  };
  if (!sendLongReport(handle, devIdx, 0x00, 0x00, params, sizeof(params))) {
    return -1;
  }
  std::vector<uint8_t> raw;
  if (!readReport(handle, raw, timeoutMs)) {
    return -1;
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

HANDLE openVendorDevice(uint16_t vendorId, uint16_t productId)
{
  GUID hidGuid{};
  HidD_GetHidGuid(&hidGuid);
  HDEVINFO info = SetupDiGetClassDevs(&hidGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
  if (info == INVALID_HANDLE_VALUE) {
    return INVALID_HANDLE_VALUE;
  }

  SP_DEVICE_INTERFACE_DATA ifData{};
  ifData.cbSize = sizeof(ifData);
  HANDLE chosen = INVALID_HANDLE_VALUE;

  for (DWORD index = 0; SetupDiEnumDeviceInterfaces(info, nullptr, &hidGuid, index, &ifData); ++index) {
    DWORD required = 0;
    SetupDiGetDeviceInterfaceDetail(info, &ifData, nullptr, 0, &required, nullptr);
    if (required == 0) {
      continue;
    }
    std::vector<BYTE> buffer(required);
    auto *detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA *>(buffer.data());
    detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    if (!SetupDiGetDeviceInterfaceDetail(info, &ifData, detail, required, nullptr, nullptr)) {
      continue;
    }

    HANDLE handle = CreateFile(
        detail->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0,
        nullptr
    );
    if (handle == INVALID_HANDLE_VALUE) {
      continue;
    }

    HIDD_ATTRIBUTES attrs{};
    attrs.Size = sizeof(attrs);
    if (!HidD_GetAttributes(handle, &attrs) || attrs.VendorID != vendorId || attrs.ProductID != productId) {
      CloseHandle(handle);
      continue;
    }

    PHIDP_PREPARSED_DATA preparsed = nullptr;
    if (!HidD_GetPreparsedData(handle, &preparsed)) {
      CloseHandle(handle);
      continue;
    }
    HIDP_CAPS caps{};
    const bool capsOk = HidP_GetCaps(preparsed, &caps) == HIDP_STATUS_SUCCESS;
    HidD_FreePreparsedData(preparsed);
    if (!capsOk || caps.UsagePage < kVendorUsagePageFloor) {
      CloseHandle(handle);
      continue;
    }

    chosen = handle;
    break;
  }

  SetupDiDestroyDeviceInfoList(info);
  return chosen;
}

} // namespace

HidppDecodeContext probeHidppDecodePlatform(uint16_t vendorId, uint16_t productId)
{
  HidppDecodeContext context;
  HANDLE handle = openVendorDevice(vendorId, productId);
  if (handle == INVALID_HANDLE_VALUE) {
    LOG_DEBUG("hidpp probe(win): no vendor interface for PID=0x%04X", productId);
    return context;
  }

  static const uint8_t kDevIdxOrder[] = {0xFF, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
  for (const uint8_t devIdx : kDevIdxOrder) {
    const int featIdx = findFeatureIndex(handle, devIdx, kFeatReprogV4, 400);
    if (featIdx > 0) {
      context.featIdx = featIdx;
      break;
    }
  }

  CloseHandle(handle);
  return context;
}

} // namespace deskflow::server
