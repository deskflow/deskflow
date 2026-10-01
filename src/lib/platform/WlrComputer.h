/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/PlatformComputer.h"
#include "platform/XDGPowerManager.h"

#include <bitset>
#include <chrono>
#include <climits>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct wl_buffer;
struct wl_compositor;
struct wl_display;
struct wl_keyboard;
struct wl_output;
struct wl_pointer;
struct wl_registry;
struct wl_seat;
struct wl_shm;
struct wl_surface;
struct wp_viewport;
struct wp_viewporter;
struct zwlr_layer_shell_v1;
struct zwlr_layer_surface_v1;
struct zwlr_virtual_pointer_manager_v1;
struct zwlr_virtual_pointer_v1;
struct zwp_virtual_keyboard_manager_v1;
struct zwp_virtual_keyboard_v1;
struct zwp_pointer_constraints_v1;
struct zwp_locked_pointer_v1;
struct zwp_relative_pointer_manager_v1;
struct zwp_relative_pointer_v1;
struct zwp_keyboard_shortcuts_inhibit_manager_v1;
struct zwp_keyboard_shortcuts_inhibitor_v1;
struct zxdg_output_manager_v1;
struct zxdg_output_v1;
struct ext_data_control_manager_v1;
struct ext_data_control_device_v1;
struct ext_data_control_offer_v1;
struct ext_data_control_source_v1;

class EventQueueTimer;

namespace deskflow {

class EiKeyState;
class EiClipboard;

//! Wayland computer for wlroots compositors (sway, Hyprland, river, ...)
/*!
Used when the compositor has no InputCapture/RemoteDesktop portal. Talks to the
compositor directly through wlroots protocols:

- primary (server): 1px layer-shell surfaces on the outer screen edges detect the
  cursor reaching an edge. While on a secondary computer, a transparent overlay
  surface locks the pointer and takes exclusive keyboard focus, so relative
  motion, buttons, scroll and keys are captured.
- secondary (client): input is injected with wlr virtual-pointer and
  virtual-keyboard.
- clipboard (both): ext-data-control, text and HTML only.
*/
class WlrComputer : public PlatformComputer
{
public:
  WlrComputer(bool isPrimary, IEventQueue *events);
  ~WlrComputer() override;

  //! True if the compositor has every protocol needed for this role
  static bool isSupported(bool isPrimary);

  // IComputer overrides
  void *getEventTarget() const final;
  bool getClipboard(ClipboardID id, IClipboard *) const override;
  void getShape(std::int32_t &x, std::int32_t &y, std::int32_t &width, std::int32_t &height) const override;
  void getCursorPos(std::int32_t &x, std::int32_t &y) const override;

  // IPrimaryComputer overrides
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

  // ISecondaryComputer overrides
  void fakeMouseButton(ButtonID id, bool press) override;
  void fakeMouseMove(std::int32_t x, std::int32_t y) override;
  void fakeMouseRelativeMove(std::int32_t dx, std::int32_t dy) const override;
  void fakeMouseWheel(ScrollDelta delta) const override;
  void fakeKey(std::uint32_t keycode, bool isDown) const;

  // IPlatformComputer overrides
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
  // IPlatformComputer overrides
  void handleSystemEvent(const Event &event) override;
  void updateButtons() override;
  IKeyState *getKeyState() const override;
  std::string getSecureInputApp() const override;

public:
  // Wayland listener state. Public only so the C listener callbacks in
  // WlrComputer.cpp can reach it; not part of the computer API.

  struct Output
  {
    WlrComputer *computer = nullptr;
    std::uint32_t name = 0;
    wl_output *output = nullptr;
    zxdg_output_v1 *xdgOutput = nullptr;
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t w = 0;
    std::int32_t h = 0;
  };

  enum class SurfaceRole
  {
    Barrier,
    Capture
  };

  struct Surface
  {
    WlrComputer *computer = nullptr;
    SurfaceRole role = SurfaceRole::Barrier;
    Output *output = nullptr;
    std::uint32_t side = 0; // barrier only, one of the deskflow side masks
    wl_surface *surface = nullptr;
    wp_viewport *viewport = nullptr;
    zwlr_layer_surface_v1 *layerSurface = nullptr;
  };

  void addGlobal(wl_registry *registry, std::uint32_t name, const char *interface, std::uint32_t version);
  void removeGlobal(std::uint32_t name);
  void onOutputDone(Output *output);
  void onSeatCapabilities(std::uint32_t caps);
  void onLayerSurfaceConfigure(Surface *surface, std::uint32_t serial, std::uint32_t w, std::uint32_t h);
  void onLayerSurfaceClosed(Surface *surface);
  void onPointerEnter(std::uint32_t serial, wl_surface *surface, double sx, double sy);
  void onPointerLeave(wl_surface *surface);
  void onPointerMotion(double sx, double sy);
  void onPointerButton(std::uint32_t button, bool pressed);
  void onPointerAxis(std::uint32_t axis, double value);
  void onPointerAxisSource(std::uint32_t source);
  void onPointerAxisDiscrete(std::uint32_t axis, std::int32_t steps);
  void onPointerFrame();
  void onRelativeMotion(double dx, double dy);
  void onKeyboardKeymap(std::uint32_t format, int fd, std::uint32_t size);
  void onKeyboardKey(std::uint32_t key, bool pressed);
  void onKeyboardRepeatInfo(std::int32_t rate, std::int32_t delay);
  void onDataOffer(ext_data_control_offer_v1 *offer);
  void onDataOfferMime(ext_data_control_offer_v1 *offer, const char *mime);
  void onSelection(ext_data_control_offer_v1 *offer);
  void onPrimarySelection(ext_data_control_offer_v1 *offer);
  void onDataSourceSend(ext_data_control_source_v1 *source, const char *mime, int fd);
  void onDataSourceCancelled(ext_data_control_source_v1 *source);

private:
  void connect();
  void disconnect();
  void bindSeatDevices();
  void updateShape();
  void createBarriers();
  void destroyBarriers();
  Surface *createLayerSurface(Output *output, SurfaceRole role, std::uint32_t side);
  void destroySurface(Surface *surface);
  void startCapture();
  void stopCapture();
  void hitBarrier(const Surface *barrier, double sx, double sy);
  Output *outputAt(std::int32_t x, std::int32_t y) const;
  void moveCursorTo(std::int32_t x, std::int32_t y);
  void uploadKeymap();
  void sendEvent(EventTypes type, void *data);
  void sendClipboardEvent(EventTypes type, ClipboardID id) const;
  bool onHotkey(KeyID key, bool isPressed, KeyModifierMask mask);
  void startKeyRepeat(KeyID key, KeyButton button, std::uint32_t keyval);
  void stopKeyRepeat();
  void sendKeyRepeat();
  void flush() const;

  bool m_isPrimary = false;
  IEventQueue *m_events = nullptr;

  wl_display *m_display = nullptr;
  wl_registry *m_registry = nullptr;
  wl_compositor *m_compositor = nullptr;
  wl_shm *m_shm = nullptr;
  wl_seat *m_seat = nullptr;
  std::uint32_t m_seatVersion = 0;
  wl_pointer *m_pointer = nullptr;
  wl_keyboard *m_keyboard = nullptr;
  wp_viewporter *m_viewporter = nullptr;
  zxdg_output_manager_v1 *m_xdgOutputManager = nullptr;
  zwlr_layer_shell_v1 *m_layerShell = nullptr;
  zwp_pointer_constraints_v1 *m_pointerConstraints = nullptr;
  zwp_relative_pointer_manager_v1 *m_relativePointerManager = nullptr;
  zwp_relative_pointer_v1 *m_relativePointer = nullptr;
  zwp_keyboard_shortcuts_inhibit_manager_v1 *m_shortcutsInhibitManager = nullptr;
  zwlr_virtual_pointer_manager_v1 *m_virtualPointerManager = nullptr;
  zwlr_virtual_pointer_v1 *m_virtualPointer = nullptr;
  zwp_virtual_keyboard_manager_v1 *m_virtualKeyboardManager = nullptr;
  zwp_virtual_keyboard_v1 *m_virtualKeyboard = nullptr;
  ext_data_control_manager_v1 *m_dataControlManager = nullptr;
  ext_data_control_device_v1 *m_dataDevice = nullptr;

  // a single transparent pixel, stretched over every surface with wp_viewport
  wl_buffer *m_transparentBuffer = nullptr;

  std::vector<std::unique_ptr<Output>> m_outputs;
  std::vector<std::unique_ptr<Surface>> m_barriers;

  // primary: overlay that holds the pointer lock while on a secondary computer
  Surface *m_capture = nullptr;
  zwp_locked_pointer_v1 *m_lockedPointer = nullptr;
  zwp_keyboard_shortcuts_inhibitor_v1 *m_shortcutsInhibitor = nullptr;
  Output *m_captureOutput = nullptr;

  // primary: which of our surfaces the pointer is over, if any
  wl_surface *m_pointerFocus = nullptr;
  // primary: a barrier the pointer was dropped onto when returning to this computer
  // doesn't count as an edge hit until the pointer leaves it once
  bool m_barriersArmed = true;
  std::chrono::steady_clock::time_point m_enteredAt;

  EiKeyState *m_keyState = nullptr;
  std::bitset<NumButtonIDs> m_buttons;

  // primary: key auto-repeat, which Wayland leaves to the client
  std::int32_t m_repeatRate = 25;
  std::int32_t m_repeatDelay = 600;
  EventQueueTimer *m_repeatTimer = nullptr;
  KeyID m_repeatKey = kKeyNone;
  KeyButton m_repeatButton = 0;
  bool m_repeatStarted = false;

  // primary: pointer frame accumulation
  double m_scrollX = 0;
  double m_scrollY = 0;
  std::int32_t m_scrollStepsX = 0;
  std::int32_t m_scrollStepsY = 0;
  bool m_scrollIsDiscrete = false;
  bool m_scrollIsFinger = false;
  double m_scrollRemainderX = 0;
  double m_scrollRemainderY = 0;
  double m_bufferDX = 0;
  double m_bufferDY = 0;

  std::uint32_t m_activeSides = 0;
  std::int32_t m_x = 0;
  std::int32_t m_y = 0;
  std::int32_t m_w = 1;
  std::int32_t m_h = 1;
  bool m_isShapeInitialized = false;

  bool m_isOnComputer = false;
  std::int32_t m_cursorX = 0;
  std::int32_t m_cursorY = 0;

  // clipboard
  EiClipboard *m_clipboard = nullptr;
  size_t m_maximumClipboardSize = INT_MAX;
  std::map<ext_data_control_offer_v1 *, std::vector<std::string>> m_offerMimes;
  ext_data_control_source_v1 *m_dataSource = nullptr;
  std::string m_sourceText;
  std::string m_sourceHtml;
  std::uint32_t m_sequenceNumber = 0;

  // hotkey id -> (key, modifiers); only seen while input is captured
  std::map<std::uint32_t, std::pair<KeyID, KeyModifierMask>> m_hotkeys;

  [[no_unique_address]] XDGPowerManager m_powerManager;
};

} // namespace deskflow
