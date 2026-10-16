/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/WlrComputer.h"

#include "base/DirectionTypes.h"
#include "base/Event.h"
#include "base/IEventQueue.h"
#include "base/IEventQueueBuffer.h"
#include "base/Log.h"
#include "common/Settings.h"
#include "deskflow/OptionTypes.h"
#include "mt/Thread.h"
#include "platform/EiClipboard.h"
#include "platform/EiKeyState.h"

#include "ext-data-control-v1-client-protocol.h"
#include "keyboard-shortcuts-inhibit-unstable-v1-client-protocol.h"
#include "pointer-constraints-unstable-v1-client-protocol.h"
#include "relative-pointer-unstable-v1-client-protocol.h"
#include "viewporter-client-protocol.h"
#include "virtual-keyboard-unstable-v1-client-protocol.h"
// the protocol names an argument "namespace", which is a C++ keyword
#define namespace layer_namespace
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#undef namespace
#include "wlr-virtual-pointer-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"

#include <wayland-client.h>

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <poll.h>
#include <queue>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

namespace deskflow {

namespace {

using enum DirectionMask;

constexpr auto kLeft = static_cast<std::uint32_t>(LeftMask);
constexpr auto kRight = static_cast<std::uint32_t>(RightMask);
constexpr auto kTop = static_cast<std::uint32_t>(TopMask);
constexpr auto kBottom = static_cast<std::uint32_t>(BottomMask);

// evdev, from linux/input-event-codes.h
constexpr std::uint32_t kBtnLeft = 0x110;
constexpr std::uint32_t kBtnRight = 0x111;
constexpr std::uint32_t kBtnMiddle = 0x112;
constexpr std::uint32_t kBtnSide = 0x113;
constexpr std::uint32_t kBtnExtra = 0x114;

// libinput reports 15 degrees of travel per wheel click
constexpr double kAxisUnitsPerClick = 15.0;
// touchpad scroll is in pixels, 10 pixels == 1 wheel click (same as EiComputer)
constexpr double kClicksPerPixel = 0.1;

// keep the cursor this far from a barrier when returning, so it doesn't bounce straight back
constexpr std::int32_t kReturnMargin = 2;
// a barrier entered this soon after returning is where the pointer was dropped, not an edge hit
constexpr auto kReturnGrace = std::chrono::milliseconds(300);

constexpr int kClipboardTimeoutMs = 1000;

const char *const kOwnMime = "application/x-synergy-owned";
const std::array kTextMimes = {"text/plain;charset=utf-8", "UTF8_STRING", "text/plain", "STRING", "TEXT"};
const char *const kHtmlMime = "text/html";

std::uint32_t nowMs()
{
  timespec ts{};
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<std::uint32_t>(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

//! Runs a sway IPC command, returns false if sway isn't there
bool swayCommand(const std::string &command)
{
  const char *path = getenv("SWAYSOCK");
  if (!path)
    return false;

  int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (fd < 0)
    return false;

  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
  if (::connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
    close(fd);
    return false;
  }

  // i3-ipc framing: magic, payload length, message type (0 = RUN_COMMAND), payload
  std::string message = "i3-ipc";
  const auto length = static_cast<std::uint32_t>(command.size());
  const std::uint32_t type = 0;
  message.append(reinterpret_cast<const char *>(&length), sizeof(length));
  message.append(reinterpret_cast<const char *>(&type), sizeof(type));
  message.append(command);

  bool ok = write(fd, message.data(), message.size()) == static_cast<ssize_t>(message.size());

  // wait for the reply so the command has run before we carry on
  pollfd pfd{fd, POLLIN, 0};
  if (ok && poll(&pfd, 1, 500) > 0) {
    char reply[512];
    ok = read(fd, reply, sizeof(reply)) > 0;
  }
  close(fd);
  return ok;
}

ButtonID mapButtonFromEvdev(std::uint32_t button)
{
  switch (button) {
  case kBtnLeft:
    return kButtonLeft;
  case kBtnRight:
    return kButtonRight;
  case kBtnMiddle:
    return kButtonMiddle;
  case kBtnSide:
    return kButtonExtra0;
  case kBtnExtra:
    return kButtonExtra1;
  default:
    return kButtonNone;
  }
}

std::uint32_t mapButtonToEvdev(ButtonID button)
{
  switch (button) {
  case kButtonLeft:
    return kBtnLeft;
  case kButtonMiddle:
    return kBtnMiddle;
  case kButtonRight:
    return kBtnRight;
  default:
    return kBtnLeft + (button - 1);
  }
}

int createAnonymousFile(size_t size)
{
  int fd = memfd_create("synergy", MFD_CLOEXEC);
  if (fd >= 0 && ftruncate(fd, static_cast<off_t>(size)) != 0) {
    close(fd);
    return -1;
  }
  return fd;
}

//! Reads one mime type of a clipboard offer, false on timeout, error or oversize
bool readOffer(
    wl_display *display, ext_data_control_offer_v1 *offer, const char *mime, size_t maxBytes, std::string &out
)
{
  int fds[2];
  if (pipe2(fds, O_CLOEXEC) != 0)
    return false;

  ext_data_control_offer_v1_receive(offer, mime, fds[1]);
  close(fds[1]);
  wl_display_flush(display);

  out.clear();
  bool ok = true;
  char buf[4096];
  for (;;) {
    pollfd pfd{fds[0], POLLIN, 0};
    if (poll(&pfd, 1, kClipboardTimeoutMs) <= 0) {
      LOG_WARN("timed out reading clipboard, mime: %s", mime);
      ok = false;
      break;
    }
    const auto n = read(fds[0], buf, sizeof(buf));
    if (n == 0)
      break;
    if (n < 0) {
      if (errno == EINTR || errno == EAGAIN)
        continue;
      ok = false;
      break;
    }
    out.append(buf, n);
    if (out.size() > maxBytes) {
      LOG_WARN("clipboard data is over the size limit, ignoring it");
      ok = false;
      break;
    }
  }
  close(fds[0]);
  return ok;
}

bool writeAll(int fd, const std::string &data)
{
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
  size_t written = 0;
  while (written < data.size()) {
    pollfd pfd{fd, POLLOUT, 0};
    if (poll(&pfd, 1, kClipboardTimeoutMs) <= 0)
      return false;
    const auto n = write(fd, data.data() + written, data.size() - written);
    if (n < 0 && errno != EAGAIN && errno != EINTR)
      return false;
    if (n > 0)
      written += n;
  }
  return true;
}

//! Event queue buffer that wakes on the Wayland connection
class WlrEventQueueBuffer : public IEventQueueBuffer
{
public:
  WlrEventQueueBuffer(wl_display *display, IEventQueue *events) : m_display{display}, m_events{events}
  {
    int fds[2];
    if (pipe2(fds, O_NONBLOCK | O_CLOEXEC) != 0)
      throw std::runtime_error("failed to create event queue pipe");
    m_pipeRead = fds[0];
    m_pipeWrite = fds[1];
  }

  ~WlrEventQueueBuffer() override
  {
    close(m_pipeRead);
    close(m_pipeWrite);
  }

  void init() override
  {
    // do nothing
  }

  void waitForEvent(double timeout) override
  {
    Thread::testCancel();

    // events already queued locally, let the computer dispatch them
    if (wl_display_prepare_read(m_display) != 0) {
      pushSystemEvent();
      return;
    }
    wl_display_flush(m_display);

    std::array pfds = {pollfd{wl_display_get_fd(m_display), POLLIN, 0}, pollfd{m_pipeRead, POLLIN, 0}};
    const int ms = (timeout < 0.0) ? -1 : static_cast<int>(1000.0 * timeout);

    if (poll(pfds.data(), pfds.size(), ms) > 0 && (pfds[0].revents & (POLLIN | POLLERR | POLLHUP))) {
      wl_display_read_events(m_display);
      pushSystemEvent();
    } else {
      wl_display_cancel_read(m_display);
    }

    if (pfds[1].revents & POLLIN) {
      char buf[64];
      while (read(m_pipeRead, buf, sizeof(buf)) > 0) {
        // drain, the data only exists to wake us up
      }
    }
    Thread::testCancel();
  }

  Type getEvent(Event &event, std::uint32_t &dataID) override
  {
    std::scoped_lock lock{m_mutex};
    const auto [isSystem, id] = m_queue.front();
    m_queue.pop();

    if (!isSystem) {
      dataID = id;
      return Type::User;
    }
    event = Event(EventTypes::System, m_events->getSystemTarget());
    return Type::System;
  }

  bool addEvent(std::uint32_t dataID) override
  {
    std::scoped_lock lock{m_mutex};
    m_queue.push({false, dataID});
    [[maybe_unused]] auto result = write(m_pipeWrite, "!", 1);
    return true;
  }

  bool isEmpty() const override
  {
    std::scoped_lock lock{m_mutex};
    return m_queue.empty();
  }

private:
  void pushSystemEvent()
  {
    std::scoped_lock lock{m_mutex};
    m_queue.push({true, 0U});
  }

  wl_display *m_display;
  IEventQueue *m_events;
  std::queue<std::pair<bool, std::uint32_t>> m_queue;
  int m_pipeRead = -1;
  int m_pipeWrite = -1;
  mutable std::mutex m_mutex;
};

// --- listeners -------------------------------------------------------------

WlrComputer *computerOf(void *data)
{
  return static_cast<WlrComputer *>(data);
}

const wl_registry_listener s_registryListener = {
    .global = [](void *data, wl_registry *registry, std::uint32_t name, const char *interface,
                 std::uint32_t version) { computerOf(data)->addGlobal(registry, name, interface, version); },
    .global_remove = [](void *data, wl_registry *, std::uint32_t name) { computerOf(data)->removeGlobal(name); },
};

const wl_output_listener s_outputListener = {
    .geometry = [](void *, wl_output *, std::int32_t, std::int32_t, std::int32_t, std::int32_t, std::int32_t,
                   const char *, const char *, std::int32_t) {},
    .mode = [](void *, wl_output *, std::uint32_t, std::int32_t, std::int32_t, std::int32_t) {},
    .done =
        [](void *data, wl_output *) {
          auto output = static_cast<WlrComputer::Output *>(data);
          output->computer->onOutputDone(output);
        },
    .scale = [](void *, wl_output *, std::int32_t) {},
    .name = [](void *, wl_output *, const char *) {},
    .description = [](void *, wl_output *, const char *) {},
};

const zxdg_output_v1_listener s_xdgOutputListener = {
    .logical_position =
        [](void *data, zxdg_output_v1 *, std::int32_t x, std::int32_t y) {
          auto output = static_cast<WlrComputer::Output *>(data);
          output->x = x;
          output->y = y;
        },
    .logical_size =
        [](void *data, zxdg_output_v1 *, std::int32_t w, std::int32_t h) {
          auto output = static_cast<WlrComputer::Output *>(data);
          output->w = w;
          output->h = h;
        },
    .done = [](void *, zxdg_output_v1 *) {},
    .name = [](void *, zxdg_output_v1 *, const char *) {},
    .description = [](void *, zxdg_output_v1 *, const char *) {},
};

const wl_seat_listener s_seatListener = {
    .capabilities = [](void *data, wl_seat *, std::uint32_t caps) { computerOf(data)->onSeatCapabilities(caps); },
    .name = [](void *, wl_seat *, const char *) {},
};

const wl_pointer_listener s_pointerListener = {
    .enter = [](void *data, wl_pointer *, std::uint32_t serial, wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy
             ) { computerOf(data)->onPointerEnter(serial, surface, wl_fixed_to_double(sx), wl_fixed_to_double(sy)); },
    .leave = [](void *data, wl_pointer *, std::uint32_t,
                wl_surface *surface) { computerOf(data)->onPointerLeave(surface); },
    .motion = [](void *data, wl_pointer *, std::uint32_t, wl_fixed_t sx,
                 wl_fixed_t sy) { computerOf(data)->onPointerMotion(wl_fixed_to_double(sx), wl_fixed_to_double(sy)); },
    .button = [](void *data, wl_pointer *, std::uint32_t, std::uint32_t, std::uint32_t button, std::uint32_t state
              ) { computerOf(data)->onPointerButton(button, state == WL_POINTER_BUTTON_STATE_PRESSED); },
    .axis = [](void *data, wl_pointer *, std::uint32_t, std::uint32_t axis,
               wl_fixed_t value) { computerOf(data)->onPointerAxis(axis, wl_fixed_to_double(value)); },
    .frame = [](void *data, wl_pointer *) { computerOf(data)->onPointerFrame(); },
    .axis_source = [](void *data, wl_pointer *,
                      std::uint32_t source) { computerOf(data)->onPointerAxisSource(source); },
    .axis_stop = [](void *, wl_pointer *, std::uint32_t, std::uint32_t) {},
    .axis_discrete = [](void *data, wl_pointer *, std::uint32_t axis,
                        std::int32_t discrete) { computerOf(data)->onPointerAxisDiscrete(axis, discrete * 120); },
    .axis_value120 = [](void *data, wl_pointer *, std::uint32_t axis,
                        std::int32_t value120) { computerOf(data)->onPointerAxisDiscrete(axis, value120); },
    .axis_relative_direction = [](void *, wl_pointer *, std::uint32_t, std::uint32_t) {},
};

const wl_keyboard_listener s_keyboardListener = {
    .keymap = [](void *data, wl_keyboard *, std::uint32_t format, std::int32_t fd,
                 std::uint32_t size) { computerOf(data)->onKeyboardKeymap(format, fd, size); },
    .enter = [](void *, wl_keyboard *, std::uint32_t, wl_surface *, wl_array *) {},
    .leave = [](void *, wl_keyboard *, std::uint32_t, wl_surface *) {},
    .key = [](void *data, wl_keyboard *, std::uint32_t, std::uint32_t, std::uint32_t key,
              std::uint32_t state) { computerOf(data)->onKeyboardKey(key, state == WL_KEYBOARD_KEY_STATE_PRESSED); },
    .modifiers = [](void *, wl_keyboard *, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t,
                    std::uint32_t) {},
    .repeat_info = [](void *data, wl_keyboard *, std::int32_t rate,
                      std::int32_t delay) { computerOf(data)->onKeyboardRepeatInfo(rate, delay); },
};

const zwp_relative_pointer_v1_listener s_relativePointerListener = {
    .relative_motion = [](void *data, zwp_relative_pointer_v1 *, std::uint32_t, std::uint32_t, wl_fixed_t dx,
                          wl_fixed_t dy, wl_fixed_t, wl_fixed_t) {
      computerOf(data)->onRelativeMotion(wl_fixed_to_double(dx), wl_fixed_to_double(dy));
    },
};

const zwlr_layer_surface_v1_listener s_layerSurfaceListener = {
    .configure =
        [](void *data, zwlr_layer_surface_v1 *, std::uint32_t serial, std::uint32_t w, std::uint32_t h) {
          auto surface = static_cast<WlrComputer::Surface *>(data);
          surface->computer->onLayerSurfaceConfigure(surface, serial, w, h);
        },
    .closed =
        [](void *data, zwlr_layer_surface_v1 *) {
          auto surface = static_cast<WlrComputer::Surface *>(data);
          surface->computer->onLayerSurfaceClosed(surface);
        },
};

const ext_data_control_offer_v1_listener s_offerListener = {
    .offer = [](void *data, ext_data_control_offer_v1 *offer, const char *mime) {
      computerOf(data)->onDataOfferMime(offer, mime);
    },
};

const ext_data_control_device_v1_listener s_dataDeviceListener = {
    .data_offer =
        [](void *data, ext_data_control_device_v1 *, ext_data_control_offer_v1 *offer) {
          ext_data_control_offer_v1_add_listener(offer, &s_offerListener, data);
          computerOf(data)->onDataOffer(offer);
        },
    .selection = [](void *data, ext_data_control_device_v1 *,
                    ext_data_control_offer_v1 *offer) { computerOf(data)->onSelection(offer); },
    .finished = [](void *, ext_data_control_device_v1 *) { LOG_WARN("clipboard data device was destroyed"); },
    .primary_selection = [](void *data, ext_data_control_device_v1 *,
                            ext_data_control_offer_v1 *offer) { computerOf(data)->onPrimarySelection(offer); },
};

const ext_data_control_source_v1_listener s_dataSourceListener = {
    .send = [](void *data, ext_data_control_source_v1 *source, const char *mime,
               std::int32_t fd) { computerOf(data)->onDataSourceSend(source, mime, fd); },
    .cancelled = [](void *data,
                    ext_data_control_source_v1 *source) { computerOf(data)->onDataSourceCancelled(source); },
};

// --- support probe ---------------------------------------------------------

struct Probe
{
  std::vector<std::string> interfaces;
};

const wl_registry_listener s_probeListener = {
    .global = [](void *data, wl_registry *, std::uint32_t, const char *interface,
                 std::uint32_t) { static_cast<Probe *>(data)->interfaces.emplace_back(interface); },
    .global_remove = [](void *, wl_registry *, std::uint32_t) {},
};

} // namespace

// --- WlrComputer -------------------------------------------------------------

bool WlrComputer::isSupported(bool isPrimary)
{
  wl_display *display = wl_display_connect(nullptr);
  if (!display)
    return false;

  Probe probe;
  wl_registry *registry = wl_display_get_registry(display);
  wl_registry_add_listener(registry, &s_probeListener, &probe);
  wl_display_roundtrip(display);
  wl_registry_destroy(registry);
  wl_display_disconnect(display);

  const auto has = [&probe](const char *name) {
    return std::ranges::find(probe.interfaces, name) != probe.interfaces.end();
  };

  if (!has(wl_seat_interface.name) || !has(zxdg_output_manager_v1_interface.name))
    return false;

  if (isPrimary) {
    return has(wl_compositor_interface.name) && has(wl_shm_interface.name) && has(wp_viewporter_interface.name) &&
           has(zwlr_layer_shell_v1_interface.name) && has(zwp_pointer_constraints_v1_interface.name) &&
           has(zwp_relative_pointer_manager_v1_interface.name);
  }
  return has(zwlr_virtual_pointer_manager_v1_interface.name) && has(zwp_virtual_keyboard_manager_v1_interface.name);
}

WlrComputer::WlrComputer(bool isPrimary, IEventQueue *events)
    : PlatformComputer{events},
      m_isPrimary{isPrimary},
      m_events{events},
      m_isOnComputer{isPrimary}
{
  m_keyState = new EiKeyState([this](std::uint32_t keycode, bool isDown) { fakeKey(keycode, isDown); }, events);
  m_clipboard = new EiClipboard(kClipboardClipboard);

  connect();

  m_events->addHandler(EventTypes::System, m_events->getSystemTarget(), [this](const auto &e) {
    handleSystemEvent(e);
  });

  if (Settings::value(Settings::Core::PreventSleep).toBool()) {
    m_powerManager.disableSleep();
  }
}

WlrComputer::~WlrComputer()
{
  m_events->adoptBuffer(nullptr);
  m_events->removeHandler(EventTypes::System, m_events->getSystemTarget());
  stopKeyRepeat();
  disconnect();
  delete m_keyState;
  delete m_clipboard;
}

void WlrComputer::connect()
{
  m_display = wl_display_connect(nullptr);
  if (!m_display)
    throw std::runtime_error("failed to connect to the wayland display");

  m_registry = wl_display_get_registry(m_display);
  wl_registry_add_listener(m_registry, &s_registryListener, this);
  wl_display_roundtrip(m_display);

  if (!m_seat || !m_xdgOutputManager)
    throw std::runtime_error("compositor has no seat or xdg-output");

  for (const auto &output : m_outputs) {
    if (!output->xdgOutput) {
      output->xdgOutput = zxdg_output_manager_v1_get_xdg_output(m_xdgOutputManager, output->output);
      zxdg_output_v1_add_listener(output->xdgOutput, &s_xdgOutputListener, output.get());
    }
  }

  if (m_isPrimary) {
    const int fd = createAnonymousFile(4);
    if (fd < 0)
      throw std::runtime_error("failed to create shm buffer");
    wl_shm_pool *pool = wl_shm_create_pool(m_shm, fd, 4);
    m_transparentBuffer = wl_shm_pool_create_buffer(pool, 0, 1, 1, 4, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);
  } else {
    m_virtualPointer = zwlr_virtual_pointer_manager_v1_create_virtual_pointer(m_virtualPointerManager, m_seat);
    m_virtualKeyboard = zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(m_virtualKeyboardManager, m_seat);
  }

  if (m_dataControlManager) {
    m_dataDevice = ext_data_control_manager_v1_get_data_device(m_dataControlManager, m_seat);
    ext_data_control_device_v1_add_listener(m_dataDevice, &s_dataDeviceListener, this);
  } else {
    LOG_WARN("compositor has no ext-data-control, clipboard sharing is disabled");
  }

  // outputs, seat capabilities, then the keymap of the new keyboard
  wl_display_roundtrip(m_display);
  wl_display_roundtrip(m_display);

  if (!m_isPrimary)
    uploadKeymap();

  updateShape();

  m_events->adoptBuffer(nullptr);
  m_events->adoptBuffer(new WlrEventQueueBuffer(m_display, m_events));
  flush();
}

void WlrComputer::disconnect()
{
  if (!m_display)
    return;

  stopCapture();
  destroyBarriers();

  for (auto &[offer, mimes] : m_offerMimes) {
    (void)mimes;
    ext_data_control_offer_v1_destroy(offer);
  }
  m_offerMimes.clear();
  if (m_dataSource)
    ext_data_control_source_v1_destroy(m_dataSource);
  if (m_dataDevice)
    ext_data_control_device_v1_destroy(m_dataDevice);
  if (m_virtualPointer)
    zwlr_virtual_pointer_v1_destroy(m_virtualPointer);
  if (m_virtualKeyboard)
    zwp_virtual_keyboard_v1_destroy(m_virtualKeyboard);
  if (m_relativePointer)
    zwp_relative_pointer_v1_destroy(m_relativePointer);
  if (m_pointer)
    wl_pointer_release(m_pointer);
  if (m_keyboard)
    wl_keyboard_release(m_keyboard);
  if (m_transparentBuffer)
    wl_buffer_destroy(m_transparentBuffer);
  for (const auto &output : m_outputs) {
    if (output->xdgOutput)
      zxdg_output_v1_destroy(output->xdgOutput);
    wl_output_release(output->output);
  }
  m_outputs.clear();

  wl_display_flush(m_display);
  wl_display_disconnect(m_display);
  m_display = nullptr;
}

void WlrComputer::addGlobal(wl_registry *registry, std::uint32_t name, const char *interface, std::uint32_t version)
{
  const auto is = [interface](const wl_interface &candidate) { return strcmp(interface, candidate.name) == 0; };
  const auto bind = [registry, name, version](const wl_interface &iface, std::uint32_t maxVersion) {
    return wl_registry_bind(registry, name, &iface, std::min(version, maxVersion));
  };

  if (is(wl_compositor_interface)) {
    m_compositor = static_cast<wl_compositor *>(bind(wl_compositor_interface, 4));
  } else if (is(wl_shm_interface)) {
    m_shm = static_cast<wl_shm *>(bind(wl_shm_interface, 1));
  } else if (is(wl_seat_interface) && !m_seat) {
    // ponytail: first seat only, multi-seat setups are rare
    m_seatVersion = std::min(version, 8u);
    m_seat = static_cast<wl_seat *>(bind(wl_seat_interface, 8));
    wl_seat_add_listener(m_seat, &s_seatListener, this);
  } else if (is(wl_output_interface)) {
    auto output = std::make_unique<Output>();
    output->computer = this;
    output->name = name;
    output->output = static_cast<wl_output *>(bind(wl_output_interface, 4));
    wl_output_add_listener(output->output, &s_outputListener, output.get());
    if (m_xdgOutputManager) {
      output->xdgOutput = zxdg_output_manager_v1_get_xdg_output(m_xdgOutputManager, output->output);
      zxdg_output_v1_add_listener(output->xdgOutput, &s_xdgOutputListener, output.get());
    }
    m_outputs.push_back(std::move(output));
  } else if (is(zxdg_output_manager_v1_interface)) {
    m_xdgOutputManager = static_cast<zxdg_output_manager_v1 *>(bind(zxdg_output_manager_v1_interface, 3));
  } else if (is(wp_viewporter_interface)) {
    m_viewporter = static_cast<wp_viewporter *>(bind(wp_viewporter_interface, 1));
  } else if (is(zwlr_layer_shell_v1_interface)) {
    m_layerShell = static_cast<zwlr_layer_shell_v1 *>(bind(zwlr_layer_shell_v1_interface, 4));
  } else if (is(zwp_pointer_constraints_v1_interface)) {
    m_pointerConstraints = static_cast<zwp_pointer_constraints_v1 *>(bind(zwp_pointer_constraints_v1_interface, 1));
  } else if (is(zwp_relative_pointer_manager_v1_interface)) {
    m_relativePointerManager =
        static_cast<zwp_relative_pointer_manager_v1 *>(bind(zwp_relative_pointer_manager_v1_interface, 1));
  } else if (is(zwp_keyboard_shortcuts_inhibit_manager_v1_interface)) {
    m_shortcutsInhibitManager = static_cast<zwp_keyboard_shortcuts_inhibit_manager_v1 *>(
        bind(zwp_keyboard_shortcuts_inhibit_manager_v1_interface, 1)
    );
  } else if (is(zwlr_virtual_pointer_manager_v1_interface)) {
    m_virtualPointerManager =
        static_cast<zwlr_virtual_pointer_manager_v1 *>(bind(zwlr_virtual_pointer_manager_v1_interface, 2));
  } else if (is(zwp_virtual_keyboard_manager_v1_interface)) {
    m_virtualKeyboardManager =
        static_cast<zwp_virtual_keyboard_manager_v1 *>(bind(zwp_virtual_keyboard_manager_v1_interface, 1));
  } else if (is(ext_data_control_manager_v1_interface)) {
    m_dataControlManager = static_cast<ext_data_control_manager_v1 *>(bind(ext_data_control_manager_v1_interface, 1));
  }
}

void WlrComputer::removeGlobal(std::uint32_t name)
{
  const auto it = std::ranges::find_if(m_outputs, [name](const auto &output) { return output->name == name; });
  if (it == m_outputs.end())
    return;

  Output *output = it->get();
  LOG_DEBUG("output removed at %d,%d", output->x, output->y);
  if (m_captureOutput == output) {
    stopCapture();
    m_captureOutput = nullptr;
  }
  destroyBarriers();
  if (output->xdgOutput)
    zxdg_output_v1_destroy(output->xdgOutput);
  wl_output_release(output->output);
  m_outputs.erase(it);
  updateShape();
}

void WlrComputer::onOutputDone(Output *output)
{
  LOG_DEBUG("output %dx%d at %d,%d", output->w, output->h, output->x, output->y);
  // the pointer lock is tied to an output that may have just moved
  if (m_capture && m_captureOutput == output) {
    stopCapture();
    startCapture();
  }
  updateShape();
}

void WlrComputer::onSeatCapabilities(std::uint32_t caps)
{
  const bool hasPointer = caps & WL_SEAT_CAPABILITY_POINTER;
  const bool hasKeyboard = caps & WL_SEAT_CAPABILITY_KEYBOARD;

  if (m_isPrimary && hasPointer && !m_pointer) {
    m_pointer = wl_seat_get_pointer(m_seat);
    wl_pointer_add_listener(m_pointer, &s_pointerListener, this);
    m_relativePointer = zwp_relative_pointer_manager_v1_get_relative_pointer(m_relativePointerManager, m_pointer);
    zwp_relative_pointer_v1_add_listener(m_relativePointer, &s_relativePointerListener, this);
  } else if (!hasPointer && m_pointer) {
    stopCapture();
    zwp_relative_pointer_v1_destroy(m_relativePointer);
    m_relativePointer = nullptr;
    wl_pointer_release(m_pointer);
    m_pointer = nullptr;
  }

  if (hasKeyboard && !m_keyboard) {
    m_keyboard = wl_seat_get_keyboard(m_seat);
    wl_keyboard_add_listener(m_keyboard, &s_keyboardListener, this);
  } else if (!hasKeyboard && m_keyboard) {
    wl_keyboard_release(m_keyboard);
    m_keyboard = nullptr;
  }
}

void WlrComputer::updateShape()
{
  std::int32_t x1 = INT_MAX;
  std::int32_t y1 = INT_MAX;
  std::int32_t x2 = INT_MIN;
  std::int32_t y2 = INT_MIN;
  for (const auto &output : m_outputs) {
    if (output->w <= 0 || output->h <= 0)
      continue;
    x1 = std::min(x1, output->x);
    y1 = std::min(y1, output->y);
    x2 = std::max(x2, output->x + output->w);
    y2 = std::max(y2, output->y + output->h);
  }

  if (x1 > x2) {
    LOG_DEBUG("no outputs with a logical size yet");
    return;
  }

  const bool changed = x1 != m_x || y1 != m_y || x2 - x1 != m_w || y2 - y1 != m_h;
  m_x = x1;
  m_y = y1;
  m_w = x2 - x1;
  m_h = y2 - y1;
  LOG_DEBUG("logical screen size: %dx%d at %d,%d", m_w, m_h, m_x, m_y);

  if (!m_isShapeInitialized) {
    m_cursorX = m_x + m_w / 2;
    m_cursorY = m_y + m_h / 2;
    m_isShapeInitialized = true;
  } else if (changed) {
    m_cursorX = std::clamp(m_cursorX, m_x, m_x + m_w - 1);
    m_cursorY = std::clamp(m_cursorY, m_y, m_y + m_h - 1);
  }

  if (m_isPrimary) {
    // outputs can change size without moving the union, so always rebuild
    destroyBarriers();
    createBarriers();
  }

  if (changed)
    sendEvent(EventTypes::ComputerShapeChanged, nullptr);
}

// --- primary: barriers and capture ----------------------------------------

WlrComputer::Surface *WlrComputer::createLayerSurface(Output *output, SurfaceRole role, std::uint32_t side)
{
  auto surface = std::make_unique<Surface>();
  surface->computer = this;
  surface->role = role;
  surface->output = output;
  surface->side = side;
  surface->surface = wl_compositor_create_surface(m_compositor);
  surface->layerSurface = zwlr_layer_shell_v1_get_layer_surface(
      m_layerShell, surface->surface, output->output, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "synergy"
  );
  zwlr_layer_surface_v1_add_listener(surface->layerSurface, &s_layerSurfaceListener, surface.get());

  std::uint32_t anchor = 0;
  std::uint32_t w = 0;
  std::uint32_t h = 0;
  const std::uint32_t horizontal = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
  const std::uint32_t vertical = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
  if (role == SurfaceRole::Capture) {
    anchor = horizontal | vertical;
  } else if (side == kLeft) {
    anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | vertical;
    w = 1;
  } else if (side == kRight) {
    anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | vertical;
    w = 1;
  } else if (side == kTop) {
    anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | horizontal;
    h = 1;
  } else {
    anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | horizontal;
    h = 1;
  }

  zwlr_layer_surface_v1_set_anchor(surface->layerSurface, anchor);
  zwlr_layer_surface_v1_set_size(surface->layerSurface, w, h);
  // don't get pushed inward by bars, the barrier must sit on the real edge
  zwlr_layer_surface_v1_set_exclusive_zone(surface->layerSurface, -1);
  zwlr_layer_surface_v1_set_keyboard_interactivity(
      surface->layerSurface, role == SurfaceRole::Capture ? ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE
                                                          : ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE
  );
  wl_surface_commit(surface->surface);

  Surface *result = surface.get();
  if (role == SurfaceRole::Barrier) {
    m_barriers.push_back(std::move(surface));
  } else {
    surface.release();
  }
  return result;
}

void WlrComputer::destroySurface(Surface *surface)
{
  if (m_pointerFocus == surface->surface)
    m_pointerFocus = nullptr;
  if (surface->viewport)
    wp_viewport_destroy(surface->viewport);
  zwlr_layer_surface_v1_destroy(surface->layerSurface);
  wl_surface_destroy(surface->surface);
}

void WlrComputer::onLayerSurfaceConfigure(Surface *surface, std::uint32_t serial, std::uint32_t w, std::uint32_t h)
{
  zwlr_layer_surface_v1_ack_configure(surface->layerSurface, serial);
  if (w == 0 || h == 0)
    return;

  if (!surface->viewport)
    surface->viewport = wp_viewporter_get_viewport(m_viewporter, surface->surface);
  wp_viewport_set_destination(surface->viewport, static_cast<std::int32_t>(w), static_cast<std::int32_t>(h));
  wl_surface_attach(surface->surface, m_transparentBuffer, 0, 0);
  wl_surface_damage_buffer(surface->surface, 0, 0, 1, 1);
  wl_surface_commit(surface->surface);
}

void WlrComputer::onLayerSurfaceClosed(Surface *surface)
{
  LOG_DEBUG("compositor closed a layer surface");
  if (surface == m_capture) {
    stopCapture();
    return;
  }
  // barriers are rebuilt on the next output change
  const auto it = std::ranges::find_if(m_barriers, [surface](const auto &b) { return b.get() == surface; });
  if (it != m_barriers.end()) {
    destroySurface(it->get());
    m_barriers.erase(it);
  }
}

void WlrComputer::createBarriers()
{
  if (!m_layerShell || m_activeSides == 0)
    return;

  // only output edges on the outside of the desktop, an inner edge leads to another output
  for (const auto &output : m_outputs) {
    if (output->w <= 0 || output->h <= 0)
      continue;
    if ((m_activeSides & kLeft) && output->x == m_x)
      createLayerSurface(output.get(), SurfaceRole::Barrier, kLeft);
    if ((m_activeSides & kRight) && output->x + output->w == m_x + m_w)
      createLayerSurface(output.get(), SurfaceRole::Barrier, kRight);
    if ((m_activeSides & kTop) && output->y == m_y)
      createLayerSurface(output.get(), SurfaceRole::Barrier, kTop);
    if ((m_activeSides & kBottom) && output->y + output->h == m_y + m_h)
      createLayerSurface(output.get(), SurfaceRole::Barrier, kBottom);
  }
  flush();
}

void WlrComputer::destroyBarriers()
{
  for (const auto &barrier : m_barriers)
    destroySurface(barrier.get());
  m_barriers.clear();
}

WlrComputer::Output *WlrComputer::outputAt(std::int32_t x, std::int32_t y) const
{
  for (const auto &output : m_outputs) {
    if (x >= output->x && x < output->x + output->w && y >= output->y && y < output->y + output->h)
      return output.get();
  }
  return m_outputs.empty() ? nullptr : m_outputs.front().get();
}

void WlrComputer::startCapture()
{
  if (m_capture || !m_pointer)
    return;

  if (!m_captureOutput)
    m_captureOutput = outputAt(m_cursorX, m_cursorY);
  if (!m_captureOutput)
    return;

  LOG_DEBUG("capturing input on output at %d,%d", m_captureOutput->x, m_captureOutput->y);
  m_capture = createLayerSurface(m_captureOutput, SurfaceRole::Capture, 0);
  // takes effect once the pointer is over the capture surface
  m_lockedPointer = zwp_pointer_constraints_v1_lock_pointer(
      m_pointerConstraints, m_capture->surface, m_pointer, nullptr, ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT
  );
  if (m_shortcutsInhibitManager) {
    m_shortcutsInhibitor = zwp_keyboard_shortcuts_inhibit_manager_v1_inhibit_shortcuts(
        m_shortcutsInhibitManager, m_capture->surface, m_seat
    );
  } else {
    LOG_WARN("compositor can't inhibit shortcuts, its own key bindings still apply on secondary computers");
  }
  m_bufferDX = 0;
  m_bufferDY = 0;
  flush();
}

void WlrComputer::stopCapture()
{
  if (!m_capture)
    return;

  LOG_DEBUG("releasing input capture");
  if (m_lockedPointer) {
    // compositors other than sway may warp here when the lock goes away
    const auto *output = m_capture->output;
    zwp_locked_pointer_v1_set_cursor_position_hint(
        m_lockedPointer, wl_fixed_from_int(std::clamp(m_cursorX - output->x, 0, output->w - 1)),
        wl_fixed_from_int(std::clamp(m_cursorY - output->y, 0, output->h - 1))
    );
    wl_surface_commit(m_capture->surface);
    zwp_locked_pointer_v1_destroy(m_lockedPointer);
    m_lockedPointer = nullptr;
  }
  if (m_shortcutsInhibitor) {
    zwp_keyboard_shortcuts_inhibitor_v1_destroy(m_shortcutsInhibitor);
    m_shortcutsInhibitor = nullptr;
  }
  destroySurface(m_capture);
  delete m_capture;
  m_capture = nullptr;
  m_captureOutput = nullptr;
  stopKeyRepeat();
  flush();
}

void WlrComputer::moveCursorTo(std::int32_t x, std::int32_t y)
{
  // keep clear of barriers, or the cursor would bounce straight back
  if (m_activeSides & kLeft)
    x = std::max(x, m_x + kReturnMargin);
  if (m_activeSides & kRight)
    x = std::min(x, m_x + m_w - 1 - kReturnMargin);
  if (m_activeSides & kTop)
    y = std::max(y, m_y + kReturnMargin);
  if (m_activeSides & kBottom)
    y = std::min(y, m_y + m_h - 1 - kReturnMargin);

  // the unlock must reach the compositor before the warp
  wl_display_roundtrip(m_display);

  // ponytail: Wayland has no warp, so this is sway-only; elsewhere the lock's cursor hint is all we have
  if (!swayCommand("seat * cursor set " + std::to_string(x) + " " + std::to_string(y)))
    LOG_DEBUG("sway ipc not available, relying on the pointer lock hint to place the cursor");
}

void WlrComputer::hitBarrier(const Surface *barrier, double sx, double sy)
{
  const auto *output = barrier->output;
  std::int32_t x = output->x + static_cast<std::int32_t>(sx);
  std::int32_t y = output->y + static_cast<std::int32_t>(sy);

  if (barrier->side == kLeft)
    x = m_x;
  else if (barrier->side == kRight)
    x = m_x + m_w - 1;
  else if (barrier->side == kTop)
    y = m_y;
  else
    y = m_y + m_h - 1;

  x = std::clamp(x, m_x, m_x + m_w - 1);
  y = std::clamp(y, m_y, m_y + m_h - 1);

  LOG_DEBUG("cursor reached the edge at %d,%d", x, y);
  m_cursorX = x;
  m_cursorY = y;
  m_captureOutput = barrier->output;
  sendEvent(EventTypes::PrimaryComputerMotionOnPrimary, MotionInfo::alloc(x, y));
}

void WlrComputer::onPointerEnter(std::uint32_t serial, wl_surface *surface, double sx, double sy)
{
  m_pointerFocus = surface;

  if (m_capture && surface == m_capture->surface) {
    // hide the cursor while it's on another computer
    wl_pointer_set_cursor(m_pointer, serial, nullptr, 0, 0);
    flush();
    return;
  }

  const auto it = std::ranges::find_if(m_barriers, [surface](const auto &b) { return b->surface == surface; });
  if (it == m_barriers.end() || !m_isOnComputer)
    return;

  if (std::chrono::steady_clock::now() - m_enteredAt < kReturnGrace) {
    LOG_DEBUG("ignoring barrier the cursor was returned onto");
    m_barriersArmed = false;
    return;
  }
  m_barriersArmed = true;
  hitBarrier(it->get(), sx, sy);
}

void WlrComputer::onPointerLeave(wl_surface *surface)
{
  if (m_pointerFocus == surface)
    m_pointerFocus = nullptr;
  m_barriersArmed = true;
}

void WlrComputer::onPointerMotion(double sx, double sy)
{
  if (!m_isOnComputer || !m_barriersArmed || !m_pointerFocus)
    return;

  // still pushing against the edge, e.g. after the server refused to switch
  const auto focus = m_pointerFocus;
  const auto it = std::ranges::find_if(m_barriers, [focus](const auto &b) { return b->surface == focus; });
  if (it != m_barriers.end())
    hitBarrier(it->get(), sx, sy);
}

void WlrComputer::onPointerButton(std::uint32_t button, bool pressed)
{
  if (m_isOnComputer)
    return;

  const auto id = mapButtonFromEvdev(button);
  if (id == kButtonNone) {
    LOG_DEBUG("event: button not recognized: 0x%x", button);
    return;
  }

  const auto mask = m_keyState->pollActiveModifiers();
  LOG_VERBOSE("event: button %s button=%d mask=0x%x", pressed ? "press" : "release", id, mask);
  m_buttons.set(id, pressed);
  sendEvent(
      pressed ? EventTypes::PrimaryComputerButtonDown : EventTypes::PrimaryComputerButtonUp, ButtonInfo::alloc(id, mask)
  );
}

void WlrComputer::onPointerAxis(std::uint32_t axis, double value)
{
  (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL ? m_scrollX : m_scrollY) += value;
}

void WlrComputer::onPointerAxisSource(std::uint32_t source)
{
  m_scrollIsFinger = source == WL_POINTER_AXIS_SOURCE_FINGER || source == WL_POINTER_AXIS_SOURCE_CONTINUOUS;
}

void WlrComputer::onPointerAxisDiscrete(std::uint32_t axis, std::int32_t steps)
{
  m_scrollIsDiscrete = true;
  (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL ? m_scrollStepsX : m_scrollStepsY) += steps;
}

void WlrComputer::onPointerFrame()
{
  if (!m_isOnComputer) {
    std::int32_t dx = 0;
    std::int32_t dy = 0;
    if (m_scrollIsDiscrete) {
      // already in 120ths of a click
      dx = m_scrollStepsX;
      dy = m_scrollStepsY;
    } else if (m_scrollX != 0 || m_scrollY != 0) {
      // smooth scroll: only send whole clicks, trunc so -0.3 doesn't become a click
      const double factor = m_scrollIsFinger ? kClicksPerPixel : 1.0 / kAxisUnitsPerClick;
      m_scrollRemainderX += m_scrollX * factor;
      m_scrollRemainderY += m_scrollY * factor;
      const double clicksX = std::trunc(m_scrollRemainderX);
      const double clicksY = std::trunc(m_scrollRemainderY);
      m_scrollRemainderX -= clicksX;
      m_scrollRemainderY -= clicksY;
      dx = static_cast<std::int32_t>(clicksX) * s_scrollDelta;
      dy = static_cast<std::int32_t>(clicksY) * s_scrollDelta;
    }

    // wayland scrolls down/right for positive values, deskflow the other way
    if (dx != 0 || dy != 0)
      sendEvent(EventTypes::PrimaryComputerWheel, WheelInfo::alloc(-dx, -dy));
  }

  m_scrollX = 0;
  m_scrollY = 0;
  m_scrollStepsX = 0;
  m_scrollStepsY = 0;
  m_scrollIsDiscrete = false;
  m_scrollIsFinger = false;
}

void WlrComputer::onRelativeMotion(double dx, double dy)
{
  if (m_isOnComputer || !m_capture)
    return;

  m_bufferDX += dx;
  m_bufferDY += dy;
  const auto pixelDx = static_cast<std::int32_t>(m_bufferDX);
  const auto pixelDy = static_cast<std::int32_t>(m_bufferDY);
  m_bufferDX -= pixelDx;
  m_bufferDY -= pixelDy;

  LOG_VERBOSE("event: motion on secondary x=%d y=%d", pixelDx, pixelDy);
  // sent even when under a pixel, so the server knows a slow-moving mouse hasn't stopped
  sendEvent(EventTypes::PrimaryComputerMotionOnSecondary, MotionInfo::alloc(pixelDx, pixelDy));
}

// --- keyboard --------------------------------------------------------------

void WlrComputer::onKeyboardKeymap(std::uint32_t format, int fd, std::uint32_t size)
{
  if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
    LOG_WARN("compositor sent a keymap that isn't xkb, keeping the default");
    close(fd);
    return;
  }

  const auto before = m_keyState->keymapAsString();
  m_keyState->init(fd, size);
  close(fd);

  // our own virtual keyboard can echo the keymap back, don't loop on it
  if (m_keyState->keymapAsString() == before)
    return;

  LOG_DEBUG("keyboard keymap changed");
  m_keyState->updateKeyMap();
  if (!m_isPrimary)
    uploadKeymap();
}

void WlrComputer::onKeyboardRepeatInfo(std::int32_t rate, std::int32_t delay)
{
  m_repeatRate = rate;
  m_repeatDelay = delay;
}

void WlrComputer::onKeyboardKey(std::uint32_t key, bool pressed)
{
  if (m_isOnComputer || !m_capture)
    return;

  const std::uint32_t keyval = key + 8;
  const KeyID keyid = m_keyState->mapKeyFromKeyval(keyval);
  const auto button = static_cast<KeyButton>(keyval);

  m_keyState->updateXkbState(keyval, pressed);
  const KeyModifierMask mask = m_keyState->pollActiveModifiers();
  LOG_VERBOSE("event: key %s keycode=%d keyid=%d mask=0x%x", pressed ? "press" : "release", key, keyid, mask);

  if (!pressed && m_repeatButton == button)
    stopKeyRepeat();

  if (onHotkey(keyid, pressed, mask))
    return;

  if (keyid == kKeyNone)
    return;

  m_keyState->sendKeyEvent(getEventTarget(), pressed, false, keyid, mask, 1, button);
  if (pressed)
    startKeyRepeat(keyid, button, keyval);
}

void WlrComputer::startKeyRepeat(KeyID key, KeyButton button, std::uint32_t keyval)
{
  stopKeyRepeat();
  if (m_repeatRate <= 0 || !m_keyState->keyRepeats(keyval))
    return;

  m_repeatKey = key;
  m_repeatButton = button;
  m_repeatStarted = false;
  m_repeatTimer = m_events->newOneShotTimer(m_repeatDelay / 1000.0, nullptr);
  m_events->addHandler(EventTypes::Timer, m_repeatTimer, [this](const auto &) { sendKeyRepeat(); });
}

void WlrComputer::sendKeyRepeat()
{
  m_keyState->sendKeyEvent(
      getEventTarget(), true, true, m_repeatKey, m_keyState->pollActiveModifiers(), 1, m_repeatButton
  );

  if (m_repeatStarted)
    return;

  // swap the delay timer for one at the repeat rate
  const auto key = m_repeatKey;
  const auto button = m_repeatButton;
  stopKeyRepeat();
  m_repeatKey = key;
  m_repeatButton = button;
  m_repeatStarted = true;
  m_repeatTimer = m_events->newTimer(1.0 / m_repeatRate, nullptr);
  m_events->addHandler(EventTypes::Timer, m_repeatTimer, [this](const auto &) { sendKeyRepeat(); });
}

void WlrComputer::stopKeyRepeat()
{
  if (m_repeatTimer) {
    m_events->removeHandler(EventTypes::Timer, m_repeatTimer);
    m_events->deleteTimer(m_repeatTimer);
    m_repeatTimer = nullptr;
  }
  m_repeatKey = kKeyNone;
  m_repeatButton = 0;
  m_repeatStarted = false;
}

bool WlrComputer::onHotkey(KeyID key, bool isPressed, KeyModifierMask mask)
{
  // ponytail: like EiComputer, hotkeys only work while input is captured
  for (const auto &[id, hotkey] : m_hotkeys) {
    if (hotkey.first == key && hotkey.second == mask) {
      sendEvent(
          isPressed ? EventTypes::PrimaryComputerHotkeyDown : EventTypes::PrimaryComputerHotkeyUp, HotKeyInfo::alloc(id)
      );
      return true;
    }
  }
  return false;
}

void WlrComputer::uploadKeymap()
{
  if (!m_virtualKeyboard)
    return;

  const auto keymap = m_keyState->keymapAsString();
  const int fd = createAnonymousFile(keymap.size() + 1);
  if (fd < 0) {
    LOG_ERR("failed to create keymap file");
    return;
  }
  if (write(fd, keymap.c_str(), keymap.size() + 1) != static_cast<ssize_t>(keymap.size() + 1)) {
    LOG_ERR("failed to write keymap file");
    close(fd);
    return;
  }
  zwp_virtual_keyboard_v1_keymap(
      m_virtualKeyboard, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, fd, static_cast<std::uint32_t>(keymap.size() + 1)
  );
  close(fd);
  flush();
}

// --- secondary: injection --------------------------------------------------

void WlrComputer::fakeMouseButton(ButtonID button, bool press)
{
  if (!m_virtualPointer)
    return;
  zwlr_virtual_pointer_v1_button(
      m_virtualPointer, nowMs(), mapButtonToEvdev(button),
      press ? WL_POINTER_BUTTON_STATE_PRESSED : WL_POINTER_BUTTON_STATE_RELEASED
  );
  zwlr_virtual_pointer_v1_frame(m_virtualPointer);
  flush();
}

void WlrComputer::fakeMouseMove(std::int32_t x, std::int32_t y)
{
  // We get one motion event before enter() with the target position
  m_cursorX = x;
  m_cursorY = y;
  if (!m_isOnComputer || !m_virtualPointer)
    return;

  // absolute virtual pointer motion spans the whole output layout
  const auto px = static_cast<std::uint32_t>(std::clamp(x - m_x, 0, m_w - 1));
  const auto py = static_cast<std::uint32_t>(std::clamp(y - m_y, 0, m_h - 1));
  zwlr_virtual_pointer_v1_motion_absolute(
      m_virtualPointer, nowMs(), px, py, static_cast<std::uint32_t>(m_w), static_cast<std::uint32_t>(m_h)
  );
  zwlr_virtual_pointer_v1_frame(m_virtualPointer);
  flush();
}

void WlrComputer::fakeMouseRelativeMove(std::int32_t dx, std::int32_t dy) const
{
  if (!m_virtualPointer)
    return;
  zwlr_virtual_pointer_v1_motion(m_virtualPointer, nowMs(), wl_fixed_from_int(dx), wl_fixed_from_int(dy));
  zwlr_virtual_pointer_v1_frame(m_virtualPointer);
  flush();
}

void WlrComputer::fakeMouseWheel(ScrollDelta delta) const
{
  if (!m_virtualPointer)
    return;

  delta = applyScrollModifier(delta);
  const auto time = nowMs();
  zwlr_virtual_pointer_v1_axis_source(m_virtualPointer, WL_POINTER_AXIS_SOURCE_WHEEL);

  // deskflow scrolls up/left for positive values, wayland the other way
  const auto sendAxis = [this, time](std::uint32_t axis, std::int32_t value) {
    if (value == 0)
      return;
    const double clicks = -static_cast<double>(value) / s_scrollDelta;
    const auto amount = wl_fixed_from_double(clicks * kAxisUnitsPerClick);
    if (value % s_scrollDelta == 0)
      zwlr_virtual_pointer_v1_axis_discrete(m_virtualPointer, time, axis, amount, static_cast<std::int32_t>(clicks));
    else
      zwlr_virtual_pointer_v1_axis(m_virtualPointer, time, axis, amount);
  };
  sendAxis(WL_POINTER_AXIS_HORIZONTAL_SCROLL, delta.x);
  sendAxis(WL_POINTER_AXIS_VERTICAL_SCROLL, delta.y);
  zwlr_virtual_pointer_v1_frame(m_virtualPointer);
  flush();
}

void WlrComputer::fakeKey(std::uint32_t keycode, bool isDown) const
{
  if (!m_virtualKeyboard)
    return;

  m_keyState->updateXkbState(keycode + 8, isDown);
  zwp_virtual_keyboard_v1_key(
      m_virtualKeyboard, nowMs(), keycode, isDown ? WL_KEYBOARD_KEY_STATE_PRESSED : WL_KEYBOARD_KEY_STATE_RELEASED
  );

  std::uint32_t depressed;
  std::uint32_t latched;
  std::uint32_t locked;
  std::uint32_t group;
  m_keyState->serializeModifiers(depressed, latched, locked, group);
  zwp_virtual_keyboard_v1_modifiers(m_virtualKeyboard, depressed, latched, locked, group);
  flush();
}

// --- clipboard -------------------------------------------------------------

void WlrComputer::onDataOffer(ext_data_control_offer_v1 *offer)
{
  if (offer)
    m_offerMimes[offer] = {};
}

void WlrComputer::onDataOfferMime(ext_data_control_offer_v1 *offer, const char *mime)
{
  m_offerMimes[offer].emplace_back(mime);
}

void WlrComputer::onSelection(ext_data_control_offer_v1 *offer)
{
  if (!offer)
    return;

  const auto mimes = std::move(m_offerMimes[offer]);
  m_offerMimes.erase(offer);
  const auto offered = [&mimes](const char *mime) { return std::ranges::find(mimes, mime) != mimes.end(); };

  if (offered(kOwnMime)) {
    ext_data_control_offer_v1_destroy(offer);
    return;
  }

  const size_t maxBytes = m_maximumClipboardSize * 1024;
  std::string text;
  std::string html;
  bool ok = true;
  for (const char *mime : kTextMimes) {
    if (offered(mime)) {
      ok = readOffer(m_display, offer, mime, maxBytes, text);
      break;
    }
  }
  if (ok && offered(kHtmlMime))
    readOffer(m_display, offer, kHtmlMime, maxBytes, html);
  ext_data_control_offer_v1_destroy(offer);

  if (text.empty() && html.empty()) {
    LOG_DEBUG("clipboard changed but has no text or html");
    return;
  }

  LOG_DEBUG("clipboard changed, text=%zu html=%zu bytes", text.size(), html.size());
  m_clipboard->open(0);
  m_clipboard->empty();
  if (!text.empty())
    m_clipboard->add(IClipboard::Format::Text, text);
  if (!html.empty())
    m_clipboard->add(IClipboard::Format::HTML, html);
  m_clipboard->close();
  sendClipboardEvent(EventTypes::ClipboardGrabbed, kClipboardClipboard);
}

void WlrComputer::onPrimarySelection(ext_data_control_offer_v1 *offer)
{
  // ponytail: only the clipboard is shared, not the primary selection
  if (!offer)
    return;
  m_offerMimes.erase(offer);
  ext_data_control_offer_v1_destroy(offer);
}

void WlrComputer::onDataSourceSend(ext_data_control_source_v1 *, const char *mime, int fd)
{
  const auto &data = strcmp(mime, kHtmlMime) == 0 ? m_sourceHtml : m_sourceText;
  if (!writeAll(fd, data))
    LOG_WARN("failed to send clipboard data, mime: %s", mime);
  close(fd);
}

void WlrComputer::onDataSourceCancelled(ext_data_control_source_v1 *source)
{
  if (source == m_dataSource)
    m_dataSource = nullptr;
  ext_data_control_source_v1_destroy(source);
}

bool WlrComputer::getClipboard(ClipboardID id, IClipboard *clipboard) const
{
  if (id != kClipboardClipboard)
    return false;
  return IClipboard::copy(clipboard, m_clipboard);
}

bool WlrComputer::setClipboard(ClipboardID id, const IClipboard *clipboard)
{
  // ponytail: text and html only, images and the primary selection aren't shared
  if (!clipboard || id != kClipboardClipboard)
    return false;

  if (!IClipboard::copy(m_clipboard, clipboard))
    return false;

  m_clipboard->open(0);
  m_sourceText = m_clipboard->has(IClipboard::Format::Text) ? m_clipboard->get(IClipboard::Format::Text) : "";
  m_sourceHtml = m_clipboard->has(IClipboard::Format::HTML) ? m_clipboard->get(IClipboard::Format::HTML) : "";
  m_clipboard->close();

  if (!m_dataDevice || (m_sourceText.empty() && m_sourceHtml.empty()))
    return true;

  auto source = ext_data_control_manager_v1_create_data_source(m_dataControlManager);
  ext_data_control_source_v1_add_listener(source, &s_dataSourceListener, this);
  ext_data_control_source_v1_offer(source, kOwnMime);
  if (!m_sourceText.empty()) {
    for (const char *mime : kTextMimes)
      ext_data_control_source_v1_offer(source, mime);
  }
  if (!m_sourceHtml.empty())
    ext_data_control_source_v1_offer(source, kHtmlMime);
  ext_data_control_device_v1_set_selection(m_dataDevice, source);

  if (m_dataSource)
    ext_data_control_source_v1_destroy(m_dataSource);
  m_dataSource = source;
  flush();
  return true;
}

void WlrComputer::checkClipboards()
{
  // changes arrive as data-control selection events
}

void WlrComputer::sendClipboardEvent(EventTypes type, ClipboardID id) const
{
  auto *info = static_cast<ClipboardInfo *>(malloc(sizeof(ClipboardInfo)));
  if (!info) {
    LOG_ERR("malloc failed for ClipboardInfo");
    return;
  }
  info->m_id = id;
  info->m_sequenceNumber = m_sequenceNumber;
  m_events->addEvent(Event(type, getEventTarget(), info));
}

// --- IComputer / IPlatformComputer ----------------------------------------------

void WlrComputer::handleSystemEvent(const Event &)
{
  if (!m_display)
    return;

  if (wl_display_dispatch_pending(m_display) < 0) {
    LOG_ERR("lost the wayland connection: %s", strerror(wl_display_get_error(m_display)));
    m_display = nullptr; // ponytail: leaked on purpose, the process is quitting
    m_events->addEvent(Event(EventTypes::Quit));
    return;
  }
  flush();
}

void WlrComputer::flush() const
{
  if (m_display)
    wl_display_flush(m_display);
}

void *WlrComputer::getEventTarget() const
{
  return const_cast<void *>(static_cast<const void *>(this));
}

void WlrComputer::sendEvent(EventTypes type, void *data)
{
  m_events->addEvent(Event(type, getEventTarget(), data));
}

void WlrComputer::getShape(std::int32_t &x, std::int32_t &y, std::int32_t &w, std::int32_t &h) const
{
  x = m_x;
  y = m_y;
  w = m_w;
  h = m_h;
}

void WlrComputer::getCursorPos(std::int32_t &x, std::int32_t &y) const
{
  x = m_cursorX;
  y = m_cursorY;
}

void WlrComputer::reconfigure(std::uint32_t activeSides)
{
  LOG_DEBUG("active sides: %s (0x%02x)", sidesMaskToString(activeSides).c_str(), activeSides);
  m_activeSides = activeSides;
  if (m_isPrimary && m_display) {
    destroyBarriers();
    createBarriers();
  }
}

std::uint32_t WlrComputer::activeSides()
{
  return m_activeSides;
}

void WlrComputer::warpCursor(std::int32_t x, std::int32_t y)
{
  m_cursorX = x;
  m_cursorY = y;
}

std::uint32_t WlrComputer::registerHotKey(KeyID key, KeyModifierMask mask)
{
  static std::uint32_t s_nextId = 0;
  const auto id = ++s_nextId;
  m_hotkeys[id] = {key, mask};
  return id;
}

void WlrComputer::unregisterHotKey(std::uint32_t id)
{
  m_hotkeys.erase(id);
}

void WlrComputer::fakeInputBegin()
{
  // nothing to do
}

void WlrComputer::fakeInputEnd()
{
  // nothing to do
}

std::int32_t WlrComputer::getJumpZoneSize() const
{
  return 1;
}

bool WlrComputer::isAnyMouseButtonDown(std::uint32_t &buttonID) const
{
  if (m_buttons.none())
    return false;
  buttonID = std::countr_zero(m_buttons.to_ulong());
  return true;
}

void WlrComputer::getCursorCenter(std::int32_t &x, std::int32_t &y) const
{
  x = m_x + m_w / 2;
  y = m_y + m_h / 2;
}

void WlrComputer::enable()
{
  // nothing to do
}

void WlrComputer::disable()
{
  // nothing to do
}

void WlrComputer::enter()
{
  m_isOnComputer = true;
  if (m_isPrimary) {
    stopCapture();
    m_enteredAt = std::chrono::steady_clock::now();
    moveCursorTo(m_cursorX, m_cursorY);
    // no more button events once capture is released, so drop any held state
    updateButtons();
  } else {
    fakeMouseMove(m_cursorX, m_cursorY);
  }
}

bool WlrComputer::canLeave()
{
  return !m_isPrimary || m_pointer != nullptr;
}

void WlrComputer::leave()
{
  m_isOnComputer = false;
  if (m_isPrimary)
    startCapture();
}

void WlrComputer::openScreensaver(bool)
{
  // not supported
}

void WlrComputer::closeScreensaver()
{
  // not supported
}

void WlrComputer::screensaver(bool)
{
  // not supported
}

void WlrComputer::resetOptions()
{
  // no wlroots-specific options
}

void WlrComputer::setOptions(const OptionsList &options)
{
  for (auto it = options.begin(); it != options.end(); ++it) {
    if (*it == kOptionClipboardSharingSize) {
      ++it;
      if (it == options.end())
        break;
      m_maximumClipboardSize = *it;
    }
  }
}

void WlrComputer::setSequenceNumber(std::uint32_t seqNum)
{
  m_sequenceNumber = seqNum;
}

bool WlrComputer::isPrimary() const
{
  return m_isPrimary;
}

void WlrComputer::updateButtons()
{
  // held buttons can't be polled, so resyncing means assuming everything is released
  m_buttons.reset();
}

IKeyState *WlrComputer::getKeyState() const
{
  return m_keyState;
}

std::string WlrComputer::getSecureInputApp() const
{
  throw std::runtime_error("get security input app not implemented");
}

} // namespace deskflow
