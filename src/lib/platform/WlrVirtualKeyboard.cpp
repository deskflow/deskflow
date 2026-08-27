/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/WlrVirtualKeyboard.h"

#include "base/Log.h"
#include "virtual-keyboard-unstable-v1-client-protocol.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

namespace deskflow {

namespace {
constexpr std::uint32_t kKeyPressed = 1;
constexpr std::uint32_t kKeyReleased = 0;

const wl_registry_listener s_registryListener = {
    .global = WlrVirtualKeyboard::global,
    .global_remove = WlrVirtualKeyboard::globalRemove,
};
} // namespace

WlrVirtualKeyboard::WlrVirtualKeyboard()
{
  m_display = wl_display_connect(nullptr);
  if (!m_display) {
    LOG_WARN("wlroots virtual keyboard: failed to connect to Wayland display");
    return;
  }

  m_registry = wl_display_get_registry(m_display);
  wl_registry_add_listener(m_registry, &s_registryListener, this);
  wl_display_roundtrip(m_display);

  if (!m_manager || !m_seat) {
    LOG_WARN("wlroots virtual keyboard: compositor does not expose required globals");
    return;
  }

  m_keyboard = zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(m_manager, m_seat);
  if (!m_keyboard || !setDefaultKeymap()) {
    if (m_keyboard) {
      zwp_virtual_keyboard_v1_destroy(m_keyboard);
      m_keyboard = nullptr;
    }
    LOG_WARN("wlroots virtual keyboard: failed to create keyboard or set keymap");
    return;
  }

  wl_display_roundtrip(m_display);
  LOG_INFO("using wlroots virtual keyboard for Hyprland input emulation");
}

WlrVirtualKeyboard::~WlrVirtualKeyboard()
{
  if (m_keyboard)
    zwp_virtual_keyboard_v1_destroy(m_keyboard);
  if (m_manager)
    zwp_virtual_keyboard_manager_v1_destroy(m_manager);
  if (m_seat)
    wl_seat_destroy(m_seat);
  if (m_registry)
    wl_registry_destroy(m_registry);
  if (m_display)
    wl_display_disconnect(m_display);
  if (m_xkbState)
    xkb_state_unref(m_xkbState);
  if (m_xkbKeymap)
    xkb_keymap_unref(m_xkbKeymap);
  if (m_xkbContext)
    xkb_context_unref(m_xkbContext);
}

bool WlrVirtualKeyboard::ready() const
{
  return m_keyboard != nullptr;
}

void WlrVirtualKeyboard::global(
    void *data, wl_registry *registry, std::uint32_t name, const char *interface, std::uint32_t version
)
{
  auto *self = static_cast<WlrVirtualKeyboard *>(data);
  if (std::strcmp(interface, wl_seat_interface.name) == 0 && !self->m_seat) {
    self->m_seat = static_cast<wl_seat *>(wl_registry_bind(registry, name, &wl_seat_interface, std::min(version, 7u)));
  } else if (std::strcmp(interface, zwp_virtual_keyboard_manager_v1_interface.name) == 0 && !self->m_manager) {
    self->m_manager = static_cast<zwp_virtual_keyboard_manager_v1 *>(
        wl_registry_bind(registry, name, &zwp_virtual_keyboard_manager_v1_interface, 1)
    );
  }
}

bool WlrVirtualKeyboard::setDefaultKeymap()
{
  m_xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (!m_xkbContext)
    return false;
  m_xkbKeymap = xkb_keymap_new_from_names(m_xkbContext, nullptr, XKB_KEYMAP_COMPILE_NO_FLAGS);
  if (!m_xkbKeymap) {
    xkb_context_unref(m_xkbContext);
    m_xkbContext = nullptr;
    return false;
  }

  auto *text = xkb_keymap_get_as_string(m_xkbKeymap, XKB_KEYMAP_FORMAT_TEXT_V1);
  if (!text) {
    xkb_keymap_unref(m_xkbKeymap);
    m_xkbKeymap = nullptr;
    xkb_context_unref(m_xkbContext);
    m_xkbContext = nullptr;
    return false;
  }
  const auto size = std::strlen(text) + 1;
  char filename[] = "/tmp/deskflow-keymap-XXXXXX";
  const auto fd = mkstemp(filename);
  bool success = fd >= 0;
  if (success) {
    unlink(filename);
    size_t offset = 0;
    while (offset < size) {
      const auto written = write(fd, text + offset, size - offset);
      if (written <= 0) {
        success = false;
        break;
      }
      offset += static_cast<size_t>(written);
    }
    success = success && lseek(fd, 0, SEEK_SET) >= 0;
  }
  if (success) {
    zwp_virtual_keyboard_v1_keymap(
        m_keyboard, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, fd, static_cast<std::uint32_t>(size)
    );
    wl_display_flush(m_display);
  }
  if (fd >= 0)
    close(fd);
  free(text);
  if (!success)
    return false;

  m_xkbState = xkb_state_new(m_xkbKeymap);
  return m_xkbState != nullptr;
}

std::uint32_t WlrVirtualKeyboard::time() const
{
  using namespace std::chrono;
  return static_cast<std::uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

void WlrVirtualKeyboard::key(std::uint32_t keycode, bool pressed)
{
  if (!m_keyboard || !m_xkbState)
    return;
  zwp_virtual_keyboard_v1_key(m_keyboard, time(), keycode, pressed ? kKeyPressed : kKeyReleased);
  xkb_state_update_key(m_xkbState, keycode + 8, pressed ? XKB_KEY_DOWN : XKB_KEY_UP);
  zwp_virtual_keyboard_v1_modifiers(
      m_keyboard,
      xkb_state_serialize_mods(m_xkbState, XKB_STATE_MODS_DEPRESSED),
      xkb_state_serialize_mods(m_xkbState, XKB_STATE_MODS_LATCHED),
      xkb_state_serialize_mods(m_xkbState, XKB_STATE_MODS_LOCKED),
      xkb_state_serialize_layout(m_xkbState, XKB_STATE_LAYOUT_EFFECTIVE)
  );
  wl_display_flush(m_display);
}

} // namespace deskflow
