/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 *
 * Windows secure-desktop input bridge (Phase 1): submits mouse/keyboard HID
 * reports to deskflow-vhid.sys via IOCTL. Test mode reads simple commands from
 * stdin; pipe mode accepts the same commands over a named pipe for watchdog/core.
 *
 * Usage:
 *   deskflow-vhid-bridge.exe test
 *   deskflow-vhid-bridge.exe pipe
 *   deskflow-vhid-bridge.exe demo-uac
 *
 * Commands (test/pipe): move <dx> <dy> | click [left|right] | key <usage> down|up
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <setupapi.h>
#include <initguid.h>

#include "../../driver/deskflow-vhid/public/deskflow_vhid_ioctl.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr wchar_t kPipeName[] = L"\\\\.\\pipe\\deskflow-vhid-bridge";
constexpr int kDemoMoveStep = 12;
constexpr int kDemoPauseMs = 40;

void log_line(const char *message)
{
  std::fprintf(stderr, "[vhid-bridge] %s\n", message);
}

std::wstring findDevicePath()
{
  HDEVINFO deviceInfo =
      SetupDiGetClassDevsW(&GUID_DEVINTERFACE_DESKFLOW_VHID, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
  if (deviceInfo == INVALID_HANDLE_VALUE) {
    return {};
  }

  SP_DEVICE_INTERFACE_DATA interfaceData{};
  interfaceData.cbSize = sizeof(interfaceData);

  std::wstring path;
  if (SetupDiEnumDeviceInterfaces(deviceInfo, nullptr, &GUID_DEVINTERFACE_DESKFLOW_VHID, 0, &interfaceData)) {
    DWORD required = 0;
    SetupDiGetDeviceInterfaceDetailW(deviceInfo, &interfaceData, nullptr, 0, &required, nullptr);
    if (required > 0) {
      std::vector<BYTE> buffer(required);
      auto *detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W *>(buffer.data());
      detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
      if (SetupDiGetDeviceInterfaceDetailW(deviceInfo, &interfaceData, detail, required, &required, nullptr)) {
        path = detail->DevicePath;
      }
    }
  }

  SetupDiDestroyDeviceInfoList(deviceInfo);
  return path;
}

class VhidClient
{
public:
  bool open()
  {
    const std::wstring path = findDevicePath();
    if (path.empty()) {
      log_line("deskflow-vhid device not found (driver installed?)");
      return false;
    }
    handle_ = CreateFileW(
        path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr
    );
    if (handle_ == INVALID_HANDLE_VALUE) {
      log_line("CreateFile on deskflow-vhid failed");
      return false;
    }
    log_line("opened deskflow-vhid device");
    return true;
  }

  ~VhidClient()
  {
    if (handle_ != INVALID_HANDLE_VALUE) {
      CloseHandle(handle_);
    }
  }

  bool mouseReport(unsigned char buttons, char dx, char dy, char wheel = 0)
  {
    DESKFLOW_VHID_MOUSE_REPORT report{};
    report.buttons = buttons;
    report.dx = dx;
    report.dy = dy;
    report.wheel = wheel;
    return deviceIoControl(IOCTL_DESKFLOW_VHID_MOUSE_REPORT, &report, sizeof(report));
  }

  bool keyboardReport(unsigned char modifiers, const unsigned char keys[6])
  {
    DESKFLOW_VHID_KEYBOARD_REPORT report{};
    report.modifiers = modifiers;
    if (keys != nullptr) {
      std::memcpy(report.keys, keys, sizeof(report.keys));
    }
    return deviceIoControl(IOCTL_DESKFLOW_VHID_KEYBOARD_REPORT, &report, sizeof(report));
  }

  bool keyUsage(unsigned char usage, bool down)
  {
    unsigned char keys[6]{};
    if (down) {
      keys[0] = usage;
    }
    return keyboardReport(0, keys);
  }

  bool clickLeft()
  {
    if (!mouseReport(1, 0, 0)) {
      return false;
    }
    Sleep(30);
    return mouseReport(0, 0, 0);
  }

private:
  bool deviceIoControl(DWORD code, void *buffer, DWORD size)
  {
    if (handle_ == INVALID_HANDLE_VALUE) {
      return false;
    }
    DWORD bytes = 0;
    if (!DeviceIoControl(handle_, code, buffer, size, nullptr, 0, &bytes, nullptr)) {
      log_line("DeviceIoControl failed");
      return false;
    }
    return true;
  }

  HANDLE handle_ = INVALID_HANDLE_VALUE;
};

bool runCommand(VhidClient &client, const std::string &line)
{
  if (line.empty() || line[0] == '#') {
    return true;
  }

  char verb[32]{};
  int a = 0;
  int b = 0;
  char arg[32]{};
  unsigned keyUsage = 0;
  char keyState[16]{};

  if (std::sscanf(line.c_str(), "%31s %d %d", verb, &a, &b) == 3 && std::strcmp(verb, "move") == 0) {
    return client.mouseReport(0, static_cast<char>(a), static_cast<char>(b));
  }
  if (std::sscanf(line.c_str(), "%31s %31s", verb, arg) == 2 && std::strcmp(verb, "click") == 0) {
    if (std::strcmp(arg, "right") == 0) {
      if (!client.mouseReport(2, 0, 0)) {
        return false;
      }
      Sleep(30);
      return client.mouseReport(0, 0, 0);
    }
    return client.clickLeft();
  }
  if (std::sscanf(line.c_str(), "%31s %u %15s", verb, &keyUsage, keyState) == 3 && std::strcmp(verb, "key") == 0) {
    const bool down = std::strcmp(keyState, "down") == 0;
    return client.keyUsage(static_cast<unsigned char>(keyUsage), down);
  }

  log_line("unknown command");
  return false;
}

void runInteractive(VhidClient &client)
{
  log_line("interactive mode — commands: move <dx> <dy> | click [left|right] | key <usage> down|up | quit");
  char line[256];
  while (std::fgets(line, sizeof(line), stdin) != nullptr) {
    std::string cmd(line);
    while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r')) {
      cmd.pop_back();
    }
    if (cmd == "quit" || cmd == "exit") {
      break;
    }
    runCommand(client, cmd);
  }
}

void runPipeServer(VhidClient &client)
{
  log_line("pipe server listening on \\\\.\\pipe\\deskflow-vhid-bridge");
  while (true) {
    HANDLE pipe = CreateNamedPipeW(
        kPipeName, PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 4096, 4096, 0, nullptr
    );
    if (pipe == INVALID_HANDLE_VALUE) {
      log_line("CreateNamedPipe failed");
      return;
    }
    if (!ConnectNamedPipe(pipe, nullptr) && GetLastError() != ERROR_PIPE_CONNECTED) {
      CloseHandle(pipe);
      continue;
    }
    char buffer[512];
    DWORD read = 0;
    while (ReadFile(pipe, buffer, sizeof(buffer) - 1, &read, nullptr) && read > 0) {
      buffer[read] = '\0';
      std::string cmd(buffer);
      while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r')) {
        cmd.pop_back();
      }
      runCommand(client, cmd);
    }
    DisconnectNamedPipe(pipe);
    CloseHandle(pipe);
  }
}

void runDemoUac(VhidClient &client)
{
  log_line("demo-uac: nudge pointer in a small square (trigger UAC, then click Yes manually with physical mouse to compare)");
  for (int lap = 0; lap < 4; ++lap) {
    for (int i = 0; i < 20; ++i) {
      client.mouseReport(0, static_cast<char>(kDemoMoveStep), 0);
      Sleep(kDemoPauseMs);
    }
    for (int i = 0; i < 20; ++i) {
      client.mouseReport(0, 0, static_cast<char>(kDemoMoveStep));
      Sleep(kDemoPauseMs);
    }
    for (int i = 0; i < 20; ++i) {
      client.mouseReport(0, static_cast<char>(-kDemoMoveStep), 0);
      Sleep(kDemoPauseMs);
    }
    for (int i = 0; i < 20; ++i) {
      client.mouseReport(0, 0, static_cast<char>(-kDemoMoveStep));
      Sleep(kDemoPauseMs);
    }
  }
  client.clickLeft();
}

} // namespace

int main(int argc, char *argv[])
{
  const char *mode = (argc >= 2) ? argv[1] : "test";

  VhidClient client;
  if (!client.open()) {
    return 1;
  }

  if (std::strcmp(mode, "pipe") == 0) {
    runPipeServer(client);
    return 0;
  }
  if (std::strcmp(mode, "demo-uac") == 0) {
    runDemoUac(client);
    return 0;
  }

  runInteractive(client);
  return 0;
}
