/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

struct wl_display;
struct wl_output;
struct wl_registry;
struct wl_seat;
struct zxdg_output_manager_v1;
struct zxdg_output_v1;
struct zwlr_virtual_pointer_manager_v1;
struct zwlr_virtual_pointer_v1;

namespace deskflow {

// Direct wlroots virtual-pointer client used when a compositor does not offer
// the RemoteDesktop portal. Hyprland exposes this protocol natively.
class WlrVirtualPointer
{
public:
  WlrVirtualPointer();
  ~WlrVirtualPointer();

  bool ready() const;
  std::uint32_t width() const;
  std::uint32_t height() const;
  void motion(std::int32_t dx, std::int32_t dy) const;
  void motionAbsolute(std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height) const;
  void button(std::uint32_t code, bool pressed) const;
  void scroll(std::int32_t dx, std::int32_t dy) const;

  static void global(void *data, wl_registry *registry, std::uint32_t name, const char *interface, std::uint32_t version);
  static void globalRemove(void *, wl_registry *, std::uint32_t) {}
  static void outputGeometry(void *, wl_output *, std::int32_t, std::int32_t, std::int32_t, std::int32_t, std::int32_t,
                             const char *, const char *, std::int32_t) {}
  static void outputMode(void *data, wl_output *, std::uint32_t flags, std::int32_t width, std::int32_t height,
                         std::int32_t) noexcept;
  static void outputDone(void *, wl_output *) {}
  static void outputScale(void *data, wl_output *, std::int32_t scale) noexcept;
  static void outputName(void *, wl_output *, const char *) {}
  static void outputDescription(void *, wl_output *, const char *) {}
  static void xdgOutputLogicalPosition(void *data, zxdg_output_v1 *, std::int32_t x, std::int32_t y) noexcept;
  static void xdgOutputLogicalSize(void *data, zxdg_output_v1 *, std::int32_t width, std::int32_t height) noexcept;
  static void xdgOutputDone(void *, zxdg_output_v1 *) {}
  static void xdgOutputName(void *, zxdg_output_v1 *, const char *) {}
  static void xdgOutputDescription(void *, zxdg_output_v1 *, const char *) {}

private:
  struct OutputInfo
  {
    wl_output *output = nullptr;
    zxdg_output_v1 *xdgOutput = nullptr;
    std::int32_t width = 1;
    std::int32_t height = 1;
    std::int32_t scale = 1;
    std::int32_t logicalX = 0;
    std::int32_t logicalY = 0;
    std::int32_t logicalWidth = 0;
    std::int32_t logicalHeight = 0;
  };

  std::uint32_t time() const;

  wl_display *m_display = nullptr;
  wl_registry *m_registry = nullptr;
  std::vector<std::unique_ptr<OutputInfo>> m_outputs;
  wl_seat *m_seat = nullptr;
  zxdg_output_manager_v1 *m_xdgOutputManager = nullptr;
  zwlr_virtual_pointer_manager_v1 *m_manager = nullptr;
  zwlr_virtual_pointer_v1 *m_pointer = nullptr;
};

} // namespace deskflow
