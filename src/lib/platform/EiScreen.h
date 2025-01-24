/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/KeyMap.h"
#include "deskflow/PlatformScreen.h"

#include <libei.h>
#include <mutex>
#include <set>
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

//! Implementation of IPlatformScreen for X11
class EiScreen : public PlatformScreen
{
public:
  EiScreen(bool is_primary, IEventQueue *events, bool use_portal);
  ~EiScreen();

  // IScreen overrides
  void *getEventTarget() const override;
  bool getClipboard(ClipboardID id, IClipboard *) const override;
  void getShape(std::int32_t &x, std::int32_t &y, std::int32_t &width, std::int32_t &height) const override;
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
  void handleSystemEvent(const Event &event, void *) override;
  void updateButtons() override;
  IKeyState *getKeyState() const override;
  std::string getSecureInputApp() const override;

  void update_shape();
  void add_device(ei_device *device);
  void remove_device(ei_device *device);

private:
  void init_ei();
  void cleanup_ei();
  void sendEvent(Event::Type type, void *data);
  ButtonID map_button_from_evdev(ei_event *event) const;
  void on_key_event(ei_event *event);
  void on_button_event(ei_event *event);
  void send_wheel_events(ei_device *device, const int threshold, double dx, double dy, bool is_discrete);
  void on_pointer_scroll_event(ei_event *event);
  void on_pointer_scroll_discrete_event(ei_event *event);
  void on_motion_event(ei_event *event);
  void on_abs_motion_event(ei_event *event);
  bool on_hotkey(KeyID key, bool is_press, KeyModifierMask mask);
  void handle_ei_log_event(ei *ei, ei_log_priority priority, const char *message, ei_log_context *context);
  void handle_connected_to_eis_event(const Event &event, void *);
  void handle_portal_session_closed(const Event &event, void *);

  static void cb_handle_ei_log_event(ei *ei, ei_log_priority priority, const char *message, ei_log_context *context)
  {
    auto screen = reinterpret_cast<EiScreen *>(ei_get_user_data(ei));
    screen->handle_ei_log_event(ei, priority, message, context);
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

  struct HotKeyItem
  {
  public:
    HotKeyItem(std::uint32_t mask, std::uint32_t id);
    bool operator<(const HotKeyItem &other) const
    {
      return mask_ < other.mask_;
    };

  public:
    std::uint32_t mask_ = 0;
    std::uint32_t id_ = 0; // for registering the hotkey
  };

  class HotKeySet
  {
  public:
    HotKeySet(KeyID keyid);
    KeyID keyid() const
    {
      return id_;
    };
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

} // namespace deskflow
