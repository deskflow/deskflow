/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/WlrVirtualPointer.h"

#include "base/Log.h"
#include "wlr-virtual-pointer-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <limits>
#include <wayland-client.h>

namespace deskflow {

namespace {
constexpr std::uint32_t kPointerButtonPressed = 1;
constexpr std::uint32_t kPointerButtonReleased = 0;
constexpr std::uint32_t kAxisVertical = 0;
constexpr std::uint32_t kAxisHorizontal = 1;
constexpr std::uint32_t kAxisSourceWheel = 0;
constexpr std::int32_t kDeskflowWheelDelta = 120;
constexpr std::int32_t kWaylandWheelDelta = 15;

const wl_registry_listener s_registryListener = {
    .global = WlrVirtualPointer::global,
    .global_remove = WlrVirtualPointer::globalRemove,
};

const wl_output_listener s_outputListener = {
    WlrVirtualPointer::outputGeometry,
    WlrVirtualPointer::outputMode,
    WlrVirtualPointer::outputDone,
    WlrVirtualPointer::outputScale,
    WlrVirtualPointer::outputName,
    WlrVirtualPointer::outputDescription,
};

const zxdg_output_v1_listener s_xdgOutputListener = {
    WlrVirtualPointer::xdgOutputLogicalPosition,
    WlrVirtualPointer::xdgOutputLogicalSize,
    WlrVirtualPointer::xdgOutputDone,
    WlrVirtualPointer::xdgOutputName,
    WlrVirtualPointer::xdgOutputDescription,
};
} // namespace

WlrVirtualPointer::WlrVirtualPointer()
{
  m_display = wl_display_connect(nullptr);
  if (!m_display) {
    LOG_WARN("wlroots virtual pointer: failed to connect to Wayland display");
    return;
  }

  m_registry = wl_display_get_registry(m_display);
  wl_registry_add_listener(m_registry, &s_registryListener, this);
  wl_display_roundtrip(m_display);

  if (!m_manager || !m_seat) {
    LOG_WARN("wlroots virtual pointer: compositor does not expose required globals");
    return;
  }

  if (m_xdgOutputManager) {
    for (const auto &output : m_outputs) {
      output->xdgOutput = zxdg_output_manager_v1_get_xdg_output(m_xdgOutputManager, output->output);
      zxdg_output_v1_add_listener(output->xdgOutput, &s_xdgOutputListener, output.get());
    }
    wl_display_roundtrip(m_display);
  }

  m_pointer = zwlr_virtual_pointer_manager_v1_create_virtual_pointer(m_manager, m_seat);
  wl_display_roundtrip(m_display);
  if (!m_pointer)
    LOG_WARN("wlroots virtual pointer: failed to create virtual pointer");
  else
    LOG_INFO("using wlroots virtual pointer for Hyprland input emulation");
}

WlrVirtualPointer::~WlrVirtualPointer()
{
  if (m_pointer)
    zwlr_virtual_pointer_v1_destroy(m_pointer);
  if (m_manager)
    zwlr_virtual_pointer_manager_v1_destroy(m_manager);
  for (const auto &output : m_outputs) {
    if (output->xdgOutput)
      zxdg_output_v1_destroy(output->xdgOutput);
    wl_output_destroy(output->output);
  }
  if (m_xdgOutputManager)
    zxdg_output_manager_v1_destroy(m_xdgOutputManager);
  if (m_seat)
    wl_seat_destroy(m_seat);
  if (m_registry)
    wl_registry_destroy(m_registry);
  if (m_display)
    wl_display_disconnect(m_display);
}

bool WlrVirtualPointer::ready() const
{
  return m_pointer != nullptr;
}

std::uint32_t WlrVirtualPointer::width() const
{
  bool haveLogicalGeometry = !m_outputs.empty();
  std::int32_t left = std::numeric_limits<std::int32_t>::max();
  std::int32_t right = std::numeric_limits<std::int32_t>::min();
  for (const auto &output : m_outputs) {
    if (output->logicalWidth <= 0) {
      haveLogicalGeometry = false;
      break;
    }
    left = std::min(left, output->logicalX);
    right = std::max(right, output->logicalX + output->logicalWidth);
  }
  if (haveLogicalGeometry)
    return std::max(right - left, 1);

  std::uint32_t width = 0;
  for (const auto &output : m_outputs) {
    const auto scale = std::max(output->scale, 1);
    width += std::max(output->width / scale, 1);
  }
  return std::max(width, 1u);
}

std::uint32_t WlrVirtualPointer::height() const
{
  bool haveLogicalGeometry = !m_outputs.empty();
  std::int32_t top = std::numeric_limits<std::int32_t>::max();
  std::int32_t bottom = std::numeric_limits<std::int32_t>::min();
  for (const auto &output : m_outputs) {
    if (output->logicalHeight <= 0) {
      haveLogicalGeometry = false;
      break;
    }
    top = std::min(top, output->logicalY);
    bottom = std::max(bottom, output->logicalY + output->logicalHeight);
  }
  if (haveLogicalGeometry)
    return std::max(bottom - top, 1);

  std::uint32_t height = 0;
  for (const auto &output : m_outputs) {
    const auto scale = std::max(output->scale, 1);
    height = std::max(height, static_cast<std::uint32_t>(std::max(output->height / scale, 1)));
  }
  return std::max(height, 1u);
}

void WlrVirtualPointer::global(
    void *data, wl_registry *registry, std::uint32_t name, const char *interface, std::uint32_t version
)
{
  auto *self = static_cast<WlrVirtualPointer *>(data);
  if (std::strcmp(interface, wl_seat_interface.name) == 0 && !self->m_seat) {
    self->m_seat = static_cast<wl_seat *>(wl_registry_bind(registry, name, &wl_seat_interface, std::min(version, 7u)));
  } else if (std::strcmp(interface, wl_output_interface.name) == 0) {
    auto output = std::make_unique<OutputInfo>();
    output->output = static_cast<wl_output *>(wl_registry_bind(registry, name, &wl_output_interface, std::min(version, 4u)));
    wl_output_add_listener(output->output, &s_outputListener, output.get());
    self->m_outputs.push_back(std::move(output));
  } else if (std::strcmp(interface, zwlr_virtual_pointer_manager_v1_interface.name) == 0 && !self->m_manager) {
    self->m_manager = static_cast<zwlr_virtual_pointer_manager_v1 *>(
        wl_registry_bind(registry, name, &zwlr_virtual_pointer_manager_v1_interface, std::min(version, 2u))
    );
  } else if (std::strcmp(interface, zxdg_output_manager_v1_interface.name) == 0 && !self->m_xdgOutputManager) {
    self->m_xdgOutputManager = static_cast<zxdg_output_manager_v1 *>(
        wl_registry_bind(registry, name, &zxdg_output_manager_v1_interface, std::min(version, 3u))
    );
  }
}

void WlrVirtualPointer::outputMode(
    void *data, wl_output *, std::uint32_t flags, std::int32_t width, std::int32_t height, std::int32_t
) noexcept
{
  auto *output = static_cast<OutputInfo *>(data);
  if ((flags & WL_OUTPUT_MODE_CURRENT) != 0) {
    output->width = std::max(width, 1);
    output->height = std::max(height, 1);
  }
}

void WlrVirtualPointer::outputScale(void *data, wl_output *, std::int32_t scale) noexcept
{
  static_cast<OutputInfo *>(data)->scale = std::max(scale, 1);
}

void WlrVirtualPointer::xdgOutputLogicalPosition(void *data, zxdg_output_v1 *, std::int32_t x, std::int32_t y) noexcept
{
  auto *output = static_cast<OutputInfo *>(data);
  output->logicalX = x;
  output->logicalY = y;
}

void WlrVirtualPointer::xdgOutputLogicalSize(
    void *data, zxdg_output_v1 *, std::int32_t width, std::int32_t height
) noexcept
{
  auto *output = static_cast<OutputInfo *>(data);
  output->logicalWidth = width;
  output->logicalHeight = height;
}

std::uint32_t WlrVirtualPointer::time() const
{
  using namespace std::chrono;
  return static_cast<std::uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

void WlrVirtualPointer::motion(std::int32_t dx, std::int32_t dy) const
{
  if (!m_pointer)
    return;
  LOG_DEBUG("wlroots virtual pointer relative motion dx=%d dy=%d", dx, dy);
  zwlr_virtual_pointer_v1_motion(m_pointer, time(), wl_fixed_from_int(dx), wl_fixed_from_int(dy));
  zwlr_virtual_pointer_v1_frame(m_pointer);
  wl_display_flush(m_display);
}

void WlrVirtualPointer::motionAbsolute(std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height) const
{
  if (!m_pointer)
    return;
  const auto extentX = std::max(width, 1u);
  const auto extentY = std::max(height, 1u);
  zwlr_virtual_pointer_v1_motion_absolute(
      m_pointer, time(), std::clamp(x, 0, static_cast<int>(extentX - 1)), std::clamp(y, 0, static_cast<int>(extentY - 1)),
      extentX, extentY
  );
  zwlr_virtual_pointer_v1_frame(m_pointer);
  wl_display_flush(m_display);
}

void WlrVirtualPointer::button(std::uint32_t code, bool pressed) const
{
  if (!m_pointer)
    return;
  zwlr_virtual_pointer_v1_button(m_pointer, time(), code, pressed ? kPointerButtonPressed : kPointerButtonReleased);
  zwlr_virtual_pointer_v1_frame(m_pointer);
  wl_display_flush(m_display);
}

void WlrVirtualPointer::scroll(std::int32_t dx, std::int32_t dy) const
{
  if (!m_pointer)
    return;

  const auto toDiscreteSteps = [](std::int32_t delta) {
    if (delta == 0)
      return 0;
    const auto steps = delta / kDeskflowWheelDelta;
    return steps == 0 ? (delta < 0 ? -1 : 1) : steps;
  };

  const auto xSteps = toDiscreteSteps(dx);
  const auto ySteps = toDiscreteSteps(dy);
  zwlr_virtual_pointer_v1_axis_source(m_pointer, kAxisSourceWheel);
  if (ySteps)
    zwlr_virtual_pointer_v1_axis_discrete(
        m_pointer, time(), kAxisVertical, wl_fixed_from_int(ySteps * kWaylandWheelDelta), ySteps
    );
  if (xSteps)
    zwlr_virtual_pointer_v1_axis_discrete(
        m_pointer, time(), kAxisHorizontal, wl_fixed_from_int(xSteps * kWaylandWheelDelta), xSteps
    );
  zwlr_virtual_pointer_v1_frame(m_pointer);
  wl_display_flush(m_display);
}

} // namespace deskflow
