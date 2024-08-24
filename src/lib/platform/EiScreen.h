/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2022 Peter Hutterer, Olivier Fourdan
 * Copyright (C) 2024 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "config.h"
#include "synergy/KeyMap.h"
#include "synergy/PlatformScreen.h"

#include <libei.h>
#include <mutex>
#include <set>
#include <vector>

struct ei;
struct ei_event;
struct ei_seat;
struct ei_device;

namespace synergy {

class EiClipboard;
class EiKeyState;
class PortalRemoteDesktop;
class PortalInputCapture;

//! Implementation of IPlatformScreen for X11
class EiScreen : public PlatformScreen {
public:
  EiScreen(bool is_primary, IEventQueue *events, bool use_portal);
  ~EiScreen();

  // IScreen overrides
  void *getEventTarget() const override;
  bool getClipboard(ClipboardID id, IClipboard *) const override;
  void getShape(
      std::int32_t &x, std::int32_t &y, std::int32_t &width,
      std::int32_t &height) const override;
  void getCursorPos(std::int32_t &x, std::int32_t &y) const override;

  // IPrimaryScreen overrides
  void reconfigure(std::uint32_t activeSides) override;
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
  void fakeKey(std::uint32_t keycode, bool is_down) const;

  // IPlatformScreen overrides
  void enable() override;
  void disable() override;
  void enter() override;
  bool leave() override;
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
  void handleSystemEvent(const Event &event, void *) override;
  void handleConnectedToEisEvent(const Event &event);
  void handlePortalSessionClosed(const Event &event);
  void updateButtons() override;
  IKeyState *getKeyState() const override;

  void updateShape();
  void addDevice(ei_device *device);
  void removeDevice(ei_device *device);

private:
  void initEi();
  void cleanupEi();
  void sendEvent(Event::Type type, void *data);
  ButtonID mapButtonFromEvdev(ei_event *event) const;
  void onKeyEvent(ei_event *event);
  void onButtonEvent(ei_event *event);
  void sendWheelEvents(
      ei_device *device, const int threshold, double dx, double dy,
      bool is_discrete);
  void onPointerScrollEvent(ei_event *event);
  void onPointerScrollDiscreteEvent(ei_event *event);
  void onMotionEvent(ei_event *event);
  void onAbsMotionEvent(ei_event *event);
  bool onHotkey(KeyID key, bool is_press, KeyModifierMask mask);

  void handleEiLogEvent(
      ei *ei, ei_log_priority priority, const char *message,
      ei_log_context *context);

  static void cbHandleEiLogEvent(
      ei *ei, ei_log_priority priority, const char *message,
      ei_log_context *context) {
    auto screen = reinterpret_cast<EiScreen *>(ei_get_user_data(ei));
    screen->handleEiLogEvent(ei, priority, message, context);
  }

private:
  // true if screen is being used as a primary screen, false otherwise
  bool is_primary_ = false;
  IEventQueue *events_ = nullptr;

  // keyboard stuff
  EiKeyState *key_state_ = nullptr;

  std::vector<ei_device *> ei_devices_;

  ei *ei_ = nullptr;
  ei_seat *ei_seat_ = nullptr;
  ei_device *ei_pointer_ = nullptr;
  ei_device *ei_keyboard_ = nullptr;
  ei_device *ei_abs_ = nullptr;

  std::uint32_t sequence_number_ = 0;

  std::uint32_t x_ = 0;
  std::uint32_t y_ = 0;
  std::uint32_t w_ = 0;
  std::uint32_t h_ = 0;

  // true if mouse has entered the screen
  bool is_on_screen_;

  // server: last pointer position
  // client: position sent before enter()
  std::int32_t cursor_x_ = 0;
  std::int32_t cursor_y_ = 0;

  double buffer_dx = 0;
  double buffer_dy = 0;

  mutable std::mutex mutex_;

  PortalRemoteDesktop *portal_remote_desktop_ = nullptr;
  PortalInputCapture *portal_input_capture_ = nullptr;

  struct HotKeyItem {
  public:
    HotKeyItem(std::uint32_t mask, std::uint32_t id);
    bool operator<(const HotKeyItem &other) const {
      return mask_ < other.mask_;
    };

  public:
    std::uint32_t mask_ = 0;
    std::uint32_t id_ = 0; // for registering the hotkey
  };

  class HotKeySet {
  public:
    HotKeySet(KeyID keyid);
    KeyID keyid() const { return id_; };
    bool remove_by_id(std::uint32_t id);
    void add_item(HotKeyItem item);
    std::uint32_t find_by_mask(std::uint32_t mask) const;

  private:
    KeyID id_ = 0;
    std::vector<HotKeyItem> set_;
  };

  using HotKeyMap = std::map<KeyID, HotKeySet>;

  HotKeyMap hotkeys_;
};

} // namespace synergy
