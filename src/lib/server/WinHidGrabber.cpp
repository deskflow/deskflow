/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/HidPassthrough.h"

#include "base/Log.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <hidsdi.h>
#include <setupapi.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace deskflow::server {

namespace {

constexpr uint32_t kVendorUsagePageFloor = 0xFF00;

bool matchesSelector(uint16_t vid, uint16_t pid, const std::vector<HidDeviceSelector> &selectors)
{
  for (const auto &selector : selectors) {
    if (selector.vid != vid) {
      continue;
    }
    if (selector.pid == 0 || selector.pid == pid) {
      return true;
    }
  }
  return false;
}

std::string wideToUtf8(const std::wstring &wide)
{
  if (wide.empty()) {
    return {};
  }
  const int size = WideCharToMultiByte(
      CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr
  );
  if (size <= 0) {
    return {};
  }
  std::string out(static_cast<size_t>(size), '\0');
  WideCharToMultiByte(
      CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), out.data(), size, nullptr, nullptr
  );
  return out;
}

//! Seize-only grabber for vendor-defined HID collections (docs/hid-passthrough.md).
class WinHidGrabber : public IHidGrabber
{
public:
  ~WinHidGrabber() override
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
    m_pendingSeize = seized;
  }

  void stop() override
  {
    m_running = false;
    if (m_thread.joinable()) {
      m_thread.join();
    }
    std::scoped_lock lock{m_mutex};
    for (auto &[id, grabbed] : m_devices) {
      stopReadThread(grabbed);
      closeHandle(grabbed);
    }
    m_devices.clear();
  }

private:
  struct GrabbedDevice
  {
    std::wstring path;
    HANDLE handle = INVALID_HANDLE_VALUE;
    HidDeviceDescriptor descriptor;
    std::vector<BYTE> reportBuffer;
    std::unique_ptr<std::thread> readThread;
    bool reading = false;
    bool open = false;
  };

  void runLoop()
  {
    LOG_DEBUG("hid passthrough: grabber run loop started");

    // A seize requested before the run loop published itself is applied here.
    if (m_pendingSeize.load()) {
      applySeized(true);
    }

    scanDevices();

    while (m_running) {
      const bool wantSeized = m_pendingSeize.load();
      if (wantSeized != m_seized) {
        applySeized(wantSeized);
      }

      scanDevices();
      pruneRemovedDevices();

      Sleep(250);
    }

    applySeized(false);
    LOG_DEBUG("hid passthrough: grabber run loop stopped");
  }

  void scanDevices()
  {
    std::set<std::wstring> seenPaths;

    GUID hidGuid{};
    HidD_GetHidGuid(&hidGuid);

    const HDEVINFO deviceInfoSet =
        SetupDiGetClassDevs(&hidGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
      LOG_WARN("hid passthrough: SetupDiGetClassDevs failed");
      return;
    }

    SP_DEVICE_INTERFACE_DATA deviceInterfaceData{};
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (DWORD index = 0;; ++index) {
      if (!SetupDiEnumDeviceInterfaces(deviceInfoSet, nullptr, &hidGuid, index, &deviceInterfaceData)) {
        break;
      }

      DWORD requiredSize = 0;
      SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, nullptr, 0, &requiredSize, nullptr);
      if (requiredSize == 0) {
        continue;
      }

      std::vector<BYTE> detailBuffer(requiredSize);
      auto *detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA *>(detailBuffer.data());
      detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
      if (!SetupDiGetDeviceInterfaceDetail(
              deviceInfoSet, &deviceInterfaceData, detail, requiredSize, &requiredSize, nullptr
          )) {
        continue;
      }

      const std::wstring path(detail->DevicePath);
      seenPaths.insert(path);
      considerDevice(path);
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    std::scoped_lock lock{m_mutex};
    m_seenPaths = std::move(seenPaths);
  }

  void considerDevice(const std::wstring &path)
  {
    HANDLE probe = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (probe == INVALID_HANDLE_VALUE) {
      return;
    }

    HIDD_ATTRIBUTES attributes{};
    attributes.Size = sizeof(HIDD_ATTRIBUTES);
    if (!HidD_GetAttributes(probe, &attributes)) {
      CloseHandle(probe);
      return;
    }

    if (!matchesSelector(attributes.VendorID, attributes.ProductID, m_selectors)) {
      CloseHandle(probe);
      return;
    }

    PHIDP_PREPARSED_DATA preparsed = nullptr;
    if (!HidD_GetPreparsedData(probe, &preparsed)) {
      CloseHandle(probe);
      return;
    }

    HIDP_CAPS caps{};
    if (HidP_GetCaps(preparsed, &caps) != HIDP_STATUS_SUCCESS || caps.UsagePage < kVendorUsagePageFloor) {
      HidD_FreePreparsedData(preparsed);
      CloseHandle(probe);
      return;
    }

    wchar_t productName[256]{};
    HidD_GetProductString(probe, productName, sizeof(productName));
    HidD_FreePreparsedData(preparsed);
    CloseHandle(probe);

    std::scoped_lock lock{m_mutex};
    for (const auto &[id, existing] : m_devices) {
      if (existing.path == path) {
        return;
      }
    }

    GrabbedDevice grabbed;
    grabbed.path = path;
    grabbed.descriptor.deviceId = m_nextDeviceId++;
    grabbed.descriptor.vid = attributes.VendorID;
    grabbed.descriptor.pid = attributes.ProductID;
    grabbed.descriptor.usagePage = caps.UsagePage;
    grabbed.descriptor.usage = caps.Usage;
    grabbed.descriptor.name = wideToUtf8(productName);
    grabbed.reportBuffer.resize(caps.InputReportByteLength > 0 ? caps.InputReportByteLength : 64);

    LOG_INFO(
        "hid passthrough: matched \"%s\" (%04x:%04x usage %x:%x)", grabbed.descriptor.name.c_str(),
        grabbed.descriptor.vid, grabbed.descriptor.pid, grabbed.descriptor.usagePage, grabbed.descriptor.usage
    );

    const auto descriptor = grabbed.descriptor;
    const uint16_t deviceId = grabbed.descriptor.deviceId;
    m_devices.emplace(deviceId, std::move(grabbed));

    if (m_onAttach) {
      m_onAttach(descriptor);
    }
    if (m_seized) {
      openSeized(m_devices.at(deviceId));
    }
  }

  void pruneRemovedDevices()
  {
    std::vector<uint16_t> removed;
    {
      std::scoped_lock lock{m_mutex};
      for (const auto &[id, grabbed] : m_devices) {
        if (m_seenPaths.find(grabbed.path) == m_seenPaths.end()) {
          removed.push_back(id);
        }
      }
    }

    for (const uint16_t deviceId : removed) {
      removeDevice(deviceId);
    }
  }

  void removeDevice(uint16_t deviceId)
  {
    DetachCallback onDetach;
    {
      std::scoped_lock lock{m_mutex};
      const auto it = m_devices.find(deviceId);
      if (it == m_devices.end()) {
        return;
      }
      stopReadThread(it->second);
      closeHandle(it->second);
      m_devices.erase(it);
      onDetach = m_onDetach;
    }
    LOG_INFO("hid passthrough: device %u unplugged", deviceId);
    if (onDetach) {
      onDetach(deviceId);
    }
  }

  void applySeized(bool seized)
  {
    std::scoped_lock lock{m_mutex};
    if (m_seized == seized) {
      return;
    }
    m_seized = seized;
    for (auto &[id, grabbed] : m_devices) {
      if (seized) {
        openSeized(grabbed);
      } else {
        closeHandle(grabbed);
      }
    }
  }

  void openSeized(GrabbedDevice &grabbed)
  {
    if (grabbed.open) {
      return;
    }
    HANDLE handle = CreateFileW(
        grabbed.path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr
    );
    if (handle == INVALID_HANDLE_VALUE) {
      LOG_WARN(
          "hid passthrough: seize of device %u failed (error %lu)", grabbed.descriptor.deviceId, GetLastError()
      );
      return;
    }
    grabbed.handle = handle;
    grabbed.open = true;
    grabbed.reading = true;
    grabbed.readThread = std::make_unique<std::thread>([this, deviceId = grabbed.descriptor.deviceId] { readLoop(deviceId); });
    LOG_INFO("hid passthrough: seized device %u (focus is remote)", grabbed.descriptor.deviceId);
  }

  void stopReadThread(GrabbedDevice &grabbed)
  {
    if (!grabbed.reading) {
      return;
    }
    grabbed.reading = false;
    if (grabbed.handle != INVALID_HANDLE_VALUE) {
      CancelIoEx(grabbed.handle, nullptr);
    }
    if (grabbed.readThread && grabbed.readThread->joinable()) {
      grabbed.readThread->join();
    }
    grabbed.readThread.reset();
  }

  void closeHandle(GrabbedDevice &grabbed)
  {
    if (!grabbed.open) {
      return;
    }
    stopReadThread(grabbed);
    if (grabbed.handle != INVALID_HANDLE_VALUE) {
      CloseHandle(grabbed.handle);
      grabbed.handle = INVALID_HANDLE_VALUE;
    }
    grabbed.open = false;
    LOG_INFO("hid passthrough: released device %u (focus is local)", grabbed.descriptor.deviceId);
  }

  void readLoop(uint16_t deviceId)
  {
    OVERLAPPED overlapped{};
    overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (overlapped.hEvent == nullptr) {
      return;
    }

    while (m_running) {
      HANDLE handle = INVALID_HANDLE_VALUE;
      std::vector<BYTE> buffer;
      FrameCallback onFrame;
      bool keepReading = false;
      {
        std::scoped_lock lock{m_mutex};
        const auto it = m_devices.find(deviceId);
        if (it == m_devices.end() || !it->second.open || !it->second.reading) {
          break;
        }
        handle = it->second.handle;
        buffer = it->second.reportBuffer;
        onFrame = m_onFrame;
        keepReading = true;
      }
      if (!keepReading) {
        break;
      }

      ResetEvent(overlapped.hEvent);
      DWORD bytesRead = 0;
      if (!ReadFile(handle, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, &overlapped)) {
        const DWORD err = GetLastError();
        if (err == ERROR_IO_PENDING) {
          const DWORD wait = WaitForSingleObject(overlapped.hEvent, 200);
          if (wait == WAIT_OBJECT_0) {
            if (!GetOverlappedResult(handle, &overlapped, &bytesRead, FALSE)) {
              break;
            }
          } else if (wait == WAIT_TIMEOUT) {
            continue;
          } else {
            break;
          }
        } else if (err == ERROR_DEVICE_NOT_CONNECTED || err == ERROR_OPERATION_ABORTED) {
          break;
        } else {
          continue;
        }
      }

      if (bytesRead == 0 || !onFrame) {
        continue;
      }

      onFrame(deviceId, std::string(reinterpret_cast<const char *>(buffer.data()), static_cast<size_t>(bytesRead)));
    }

    CloseHandle(overlapped.hEvent);
  }

  std::vector<HidDeviceSelector> m_selectors;
  AttachCallback m_onAttach;
  FrameCallback m_onFrame;
  DetachCallback m_onDetach;
  std::thread m_thread;
  std::atomic<bool> m_running{false};
  std::atomic<bool> m_pendingSeize{false};
  std::mutex m_mutex;
  std::map<uint16_t, GrabbedDevice> m_devices;
  std::set<std::wstring> m_seenPaths;
  bool m_seized = false;
  uint16_t m_nextDeviceId = 1;
};

} // namespace

std::unique_ptr<IHidGrabber> createHidGrabber()
{
  return std::make_unique<WinHidGrabber>();
}

} // namespace deskflow::server
