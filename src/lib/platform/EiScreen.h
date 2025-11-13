/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/IScreen.h"
#include "deskflow/PlatformScreen.h"
#include "platform/XDGPowerManager.h"

#include <libei.h>
#include <map>
#include <mutex>
#include <vector>

struct ei;
struct ei_event;
struct ei_seat;
struct ei_device;

namespace deskflow {

class EiClipboard;
class EiKeyState;
class PortalRemoteDesktop;
class PortalInputCapture;

using ClipboardInfo = IScreen::ClipboardInfo;

//! Implementation of IPlatformScreen for X11
class EiScreen : public PlatformScreen
{
public:
  EiScreen(bool isPrimary, IEventQueue *events, bool usePortal, bool invertScrolling = false);
  ~EiScreen() override;

  // IScreen overrides
  void *getEventTarget() const final;
  bool getClipboard(ClipboardID id, IClipboard *) const override;
  void getShape(std::int32_t &x, std::int32_t &y, std::int32_t &width, std::int32_t &height) const override;
  void getCursorPos(std::int32_t &x, std::int32_t &y) const override;

  // IPrimaryScreen overrides
  void reconfigure(std::uint32_t activeSides) override;
  std::uint32_t activeSides() override;
  void warpCursor(std::int32_t x, std::int32_t y) override;
  std::uint32_t registerHotKey(KeyID key, KeyModifierMask mask) override;
  void unregisterHotKey(std::uint32_t id) override;
  void fakeInputBegin() override;
  void fakeInputEnd() override;
  std::int32_t getJumpZoneSize() const override;
  bool isAnyMouseButtonDown(std::uint32_t &buttonID) const override;
  void getCursorCenter(std::int32_t &x, std::int32_t &y) const override;

  // ISecondaryScreen overrides
  void fakeMouseButton(ButtonID id, bool press) override;
  void fakeMouseMove(std::int32_t x, std::int32_t y) override;
  void fakeMouseRelativeMove(std::int32_t dx, std::int32_t dy) const override;
  void fakeMouseWheel(std::int32_t xDelta, std::int32_t yDelta) const override;
  void fakeKey(std::uint32_t keycode, bool isDown) const;

  // IPlatformScreen overrides
  void enable() override;
  void disable() override;
  void enter() override;
  bool canLeave() override;
  void leave() override;
  bool setClipboard(ClipboardID, const IClipboard *) override;
  void checkClipboards() override;
  void openScreensaver(bool notify) override;
  void closeScreensaver() override;
  void screensaver(bool activate) override;
  void resetOptions() override;
  void setOptions(const OptionsList &options) override;
  void setSequenceNumber(std::uint32_t) override;
  bool isPrimary() const override;

protected:
  // IPlatformScreen overrides
  void handleSystemEvent(const Event &event) override;
  void updateButtons() override;
  IKeyState *getKeyState() const override;
  std::string getSecureInputApp() const override;

  void updateShape();
  void addDevice(ei_device *device);
  void removeDevice(ei_device *device);

private:
  void initEi();
  void cleanupEi();
  void sendEvent(EventTypes type, void *data);
  void sendClipboardEvent(EventTypes type, ClipboardID id) const;
  ButtonID mapButtonFromEvdev(ei_event *event) const;
  void onKeyEvent(ei_event *event);
  void onButtonEvent(ei_event *event);
  void sendWheelEvents(ei_device *device, const int threshold, double dx, double dy, bool is_discrete);
  void onPointerScrollEvent(ei_event *event);
  void onPointerScrollDiscreteEvent(ei_event *event);
  void onMotionEvent(ei_event *event);
  void onAbsMotionEvent(const ei_event *) const;
  bool onHotkey(KeyID key, bool isPressed, KeyModifierMask mask);
  void eiLogEvent(ei_log_priority priority, const char *message) const;

  void handleConnectedToEisEvent(const Event &event);
  void handlePortalSessionClosed();

  static void handleEiLogEvent(ei *ei, const ei_log_priority priority, const char *message, ei_log_context *)
  {
    auto screen = static_cast<EiScreen *>(ei_get_user_data(ei));
    screen->eiLogEvent(priority, message);
  }

private:
  // true if screen is being used as a primary screen, false otherwise
  bool m_isPrimary = false;
  IEventQueue *m_events = nullptr;

  // keyboard stuff
  EiKeyState *m_keyState = nullptr;

  // clipboard stuff
  EiClipboard *m_clipboard = nullptr;

  std::vector<ei_device *> m_eiDevices;

  ei *m_ei = nullptr;
  ei_seat *m_eiSeat = nullptr;
  ei_device *m_eiPointer = nullptr;
  ei_device *m_eiKeyboard = nullptr;
  ei_device *m_eiAbs = nullptr;

  std::uint32_t m_sequenceNumber = 0;

  std::uint32_t m_activeSides = 0;
  std::uint32_t m_x = 0;
  std::uint32_t m_y = 0;
  std::uint32_t m_w = 0;
  std::uint32_t m_h = 0;

  // true if mouse has entered the screen
  bool m_isOnScreen;

  // server: last pointer position
  // client: position sent before enter()
  std::int32_t m_cursorX = 0;
  std::int32_t m_cursorY = 0;

  double m_bufferDX = 0;
  double m_bufferDY = 0;

  mutable std::mutex m_mutex;

  PortalRemoteDesktop *m_portalRemoteDesktop = nullptr;
  PortalInputCapture *m_portalInputCapture = nullptr;

  struct HotKeyItem
  {
  public:
    HotKeyItem(std::uint32_t mask, std::uint32_t id);
    auto operator<=>(const HotKeyItem &other) const
    {
      return mask <=> other.mask;
    }

  public:
    std::uint32_t mask = 0;
    std::uint32_t id = 0; // for registering the hotkey
  };

  class HotKeySet
  {
  public:
    explicit HotKeySet(KeyID keyid);
    KeyID keyid() const
    {
      return m_id;
    }
    bool removeById(std::uint32_t id);
    void addItem(HotKeyItem item);
    std::uint32_t findByMask(std::uint32_t mask) const;

  private:
    KeyID m_id = 0;
    std::vector<HotKeyItem> m_set;
  };

  using HotKeyMap = std::map<KeyID, HotKeySet>;

  HotKeyMap m_hotkeys;
  [[no_unique_address]] XDGPowerManager m_powerManager;
};

} // namespace deskflow
