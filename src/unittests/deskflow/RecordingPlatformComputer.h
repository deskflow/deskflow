/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 lxf82dev <lxf82dev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/PlatformComputer.h"

#include <vector>

// In-memory platform boundary for connection and navigation tests. Never posts input.
class RecordingPlatformComputer : public PlatformComputer
{
public:
  explicit RecordingPlatformComputer(IEventQueue *events) : PlatformComputer(events)
  {
  }
  struct KeyPress
  {
    KeyID key;
    KeyModifierMask mask;
    KeyButton button;
  };
  std::vector<KeyPress> keys;
  std::vector<KeyButton> releases;
  std::vector<std::pair<ButtonID, bool>> buttons;
  bool enabled = false;
  bool handleSas = false;
  int sasRequests = 0;

  void *getEventTarget() const override
  {
    return const_cast<RecordingPlatformComputer *>(this);
  }
  bool getClipboard(ClipboardID, IClipboard *) const override
  {
    return false;
  }
  void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const override
  {
    x = y = 0;
    width = 1920;
    height = 1080;
  }
  void getCursorPos(int32_t &x, int32_t &y) const override
  {
    x = y = 0;
  }
  void reconfigure(uint32_t) override
  {
  }
  uint32_t activeSides() override
  {
    return 0;
  }
  void warpCursor(int32_t, int32_t) override
  {
  }
  uint32_t registerHotKey(KeyID, KeyModifierMask) override
  {
    return 0;
  }
  void unregisterHotKey(uint32_t) override
  {
  }
  void fakeInputBegin() override
  {
  }
  void fakeInputEnd() override
  {
  }
  int32_t getJumpZoneSize() const override
  {
    return 1;
  }
  bool isAnyMouseButtonDown(uint32_t &) const override
  {
    return false;
  }
  void getCursorCenter(int32_t &x, int32_t &y) const override
  {
    x = y = 0;
  }
  void fakeMouseButton(ButtonID id, bool down) override
  {
    buttons.emplace_back(id, down);
  }
  void fakeMouseMove(int32_t, int32_t) override
  {
  }
  void fakeMouseRelativeMove(int32_t, int32_t) const override
  {
  }
  void fakeMouseWheel(ScrollDelta) const override
  {
  }
  void updateKeyMap() override
  {
  }
  void updateKeyState() override
  {
  }
  void setHalfDuplexMask(KeyModifierMask) override
  {
  }
  void fakeKeyDown(KeyID key, KeyModifierMask mask, KeyButton button, const std::string &) override
  {
    keys.push_back({key, mask, button});
  }
  bool fakeKeyUp(KeyButton button) override
  {
    releases.push_back(button);
    return true;
  }
  void fakeAllKeysUp() override
  {
  }
  bool fakeCtrlAltDel() override
  {
    ++sasRequests;
    return handleSas;
  }
  bool fakeMediaKey(KeyID) override
  {
    return false;
  }
  void enable() override
  {
    enabled = true;
  }
  void disable() override
  {
    enabled = false;
  }
  void enter() override
  {
  }
  bool canLeave() override
  {
    return true;
  }
  void leave() override
  {
  }
  bool setClipboard(ClipboardID, const IClipboard *) override
  {
    return true;
  }
  void checkClipboards() override
  {
  }
  void openScreensaver(bool) override
  {
  }
  void closeScreensaver() override
  {
  }
  void screensaver(bool) override
  {
  }
  void resetOptions() override
  {
  }
  void setOptions(const OptionsList &) override
  {
  }
  void setSequenceNumber(uint32_t) override
  {
  }
  std::string getSecureInputApp() const override
  {
    return {};
  }
  bool isPrimary() const override
  {
    return false;
  }

protected:
  void updateButtons() override
  {
  }
  IKeyState *getKeyState() const override
  {
    return nullptr;
  }
  void handleSystemEvent(const Event &) override
  {
  }
};
