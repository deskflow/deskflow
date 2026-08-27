/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstdint>

struct wl_display;
struct wl_registry;
struct wl_seat;
struct zwp_virtual_keyboard_manager_v1;
struct zwp_virtual_keyboard_v1;
struct xkb_context;
struct xkb_keymap;
struct xkb_state;

namespace deskflow {

// Direct wlroots virtual-keyboard client used with WlrVirtualPointer when a
// compositor does not provide the RemoteDesktop portal.
class WlrVirtualKeyboard
{
public:
  WlrVirtualKeyboard();
  ~WlrVirtualKeyboard();

  bool ready() const;
  void key(std::uint32_t keycode, bool pressed);

  static void global(void *data, wl_registry *registry, std::uint32_t name, const char *interface, std::uint32_t version);
  static void globalRemove(void *, wl_registry *, std::uint32_t) {}

private:
  bool setDefaultKeymap();
  std::uint32_t time() const;

  wl_display *m_display = nullptr;
  wl_registry *m_registry = nullptr;
  wl_seat *m_seat = nullptr;
  zwp_virtual_keyboard_manager_v1 *m_manager = nullptr;
  zwp_virtual_keyboard_v1 *m_keyboard = nullptr;
  xkb_context *m_xkbContext = nullptr;
  xkb_keymap *m_xkbKeymap = nullptr;
  xkb_state *m_xkbState = nullptr;
};

} // namespace deskflow
