/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/EiScreen.h"

#include "base/IEventQueue.h"
#include "base/Log.h"
#include "common/Constants.h"
#include "common/Settings.h"
#include "deskflow/App.h"
#include "deskflow/IScreen.h"
#include "platform/EiClipboard.h"
#include "platform/EiEventQueueBuffer.h"
#include "platform/EiKeyState.h"
#include "platform/PortalInputCapture.h"
#include "platform/PortalRemoteDesktop.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <vector>

// Values are in pixels
struct ScrollRemainder
{
  double x;
  double y;
};

namespace deskflow {

EiScreen::EiScreen(bool isPrimary, IEventQueue *events, bool usePortal, bool invertScrolling)
    : PlatformScreen{events, invertScrolling},
      m_isPrimary{isPrimary},
      m_events{events},
      m_w{1},
      m_h{1},
      m_isOnScreen{isPrimary}
{
  initEi();
  m_keyState = new EiKeyState(this, events);
  m_clipboard = new EiClipboard();
  // install event handlers
  m_events->addHandler(EventTypes::System, m_events->getSystemTarget(), [this](const auto &e) {
    handleSystemEvent(e);
  });

  if (usePortal) {
    m_events->addHandler(EventTypes::EIConnected, getEventTarget(), [this](const auto &e) {
      handleConnectedToEisEvent(e);
    });
    if (isPrimary) {
      m_portalInputCapture = new PortalInputCapture(this, m_events);
    } else {
      m_events->addHandler(EventTypes::EISessionClosed, getEventTarget(), [this](const auto &) {
        handlePortalSessionClosed();
      });
      m_portalRemoteDesktop = new PortalRemoteDesktop(this, m_events);
    }
  } else {
    // Note: socket backend does not support reconnections
    auto rc = ei_setup_backend_socket(m_ei, nullptr);
    if (rc != 0) {
      LOG_ERR("ei init error: %s", strerror(-rc));
      throw std::runtime_error("failed to init ei context");
    }
  }

  // disable sleep if the flag is set
  if (Settings::value(Settings::Core::PreventSleep).toBool()) {
    m_powerManager.disableSleep();
  }
}

EiScreen::~EiScreen()
{
  m_events->adoptBuffer(nullptr);
  m_events->removeHandler(EventTypes::System, m_events->getSystemTarget());

  cleanupEi();

  delete m_keyState;
  delete m_clipboard;

  delete m_portalRemoteDesktop;
}

void EiScreen::eiLogEvent(ei_log_priority priority, const char *message) const
{
  switch (priority) {
  case EI_LOG_PRIORITY_DEBUG:
    LOG_DEBUG1("ei: %s", message);
    break;
  case EI_LOG_PRIORITY_INFO:
    LOG_INFO("ei: %s", message);
    break;
  case EI_LOG_PRIORITY_WARNING:
    LOG_WARN("ei: %s", message);
    break;
  case EI_LOG_PRIORITY_ERROR:
    LOG_ERR("ei: %s", message);
    break;
  default:
    LOG_PRINT("ei: %s", message);
    break;
  }
}

void EiScreen::initEi()
{
  if (m_isPrimary) {
    m_ei = ei_new_receiver(nullptr); // we receive from the display server
  } else {
    m_ei = ei_new_sender(nullptr); // we send to the display server
  }
  ei_set_user_data(m_ei, this);
  ei_log_set_priority(m_ei, EI_LOG_PRIORITY_DEBUG);
  ei_log_set_handler(m_ei, handleEiLogEvent);
  std::string configName = kAppId;
  ei_configure_name(m_ei, configName.append(" client").c_str());

  // install the platform event queue
  m_events->adoptBuffer(nullptr);
  m_events->adoptBuffer(new EiEventQueueBuffer(m_ei, m_events));
}

void EiScreen::cleanupEi()
{
  if (m_eiPointer) {
    free(ei_device_get_user_data(m_eiPointer));
    ei_device_set_user_data(m_eiPointer, nullptr);
    m_eiPointer = ei_device_unref(m_eiPointer);
  }
  if (m_eiKeyboard) {
    free(ei_device_get_user_data(m_eiKeyboard));
    ei_device_set_user_data(m_eiKeyboard, nullptr);
    m_eiKeyboard = ei_device_unref(m_eiKeyboard);
  }
  if (m_eiAbs) {
    free(ei_device_get_user_data(m_eiAbs));
    ei_device_set_user_data(m_eiAbs, nullptr);
    m_eiAbs = ei_device_unref(m_eiAbs);
  }
  m_eiSeat = ei_seat_unref(m_eiSeat);
  for (auto it = m_eiDevices.begin(); it != m_eiDevices.end(); it++) {
    free(ei_device_get_user_data(*it));
    ei_device_set_user_data(*it, nullptr);
    ei_device_unref(*it);
  }
  m_eiDevices.clear();
  m_ei = ei_unref(m_ei);
}

void *EiScreen::getEventTarget() const
{
  return const_cast<void *>(static_cast<const void *>(this));
}

bool EiScreen::getClipboard(ClipboardID id, IClipboard *clipboard) const
{
  if (!m_clipboard || !m_clipboard->isAvailable()) {
    return false;
  }

  IClipboard *sourceClipboard = m_clipboard->getClipboard(id);
  if (!sourceClipboard) {
    return false;
  }

  return IClipboard::copy(clipboard, sourceClipboard);
}

void EiScreen::getShape(int32_t &x, int32_t &y, int32_t &w, int32_t &h) const
{
  x = m_x;
  y = m_y;
  w = m_w;
  h = m_h;
}

void EiScreen::getCursorPos(int32_t &x, int32_t &y) const
{
  x = m_cursorX;
  y = m_cursorY;
}

void EiScreen::reconfigure(uint32_t activeSides)
{
  const static auto sidesText = sidesMaskToString(activeSides);
  LOG_DEBUG("active sides: %s (0x%02x)", sidesText.c_str(), activeSides);
  m_activeSides = activeSides;
}

std::uint32_t EiScreen::activeSides()
{
  return m_activeSides;
}

void EiScreen::warpCursor(int32_t x, int32_t y)
{
  m_cursorX = x;
  m_cursorY = y;
}

std::uint32_t EiScreen::registerHotKey(KeyID key, KeyModifierMask mask)
{
  static std::uint32_t next_id;
  std::uint32_t id = std::min(++next_id, 1u);

  // Bug: id rollover means duplicate hotkey ids. Oh well.

  auto set = m_hotkeys.find(key);
  if (set == m_hotkeys.end()) {
    m_hotkeys.try_emplace(key, HotKeySet{key});
    set = m_hotkeys.find(key);
  }
  set->second.addItem(HotKeyItem(mask, id));

  return id;
}

void EiScreen::unregisterHotKey(uint32_t id)
{
  for (auto &[key, set] : m_hotkeys) {
    (void)key;
    if (set.removeById(id)) {
      break;
    }
  }
}

void EiScreen::fakeInputBegin()
{
  // FIXME -- not implemented
}

void EiScreen::fakeInputEnd()
{
  // FIXME -- not implemented
}

std::int32_t EiScreen::getJumpZoneSize() const
{
  return 1;
}

bool EiScreen::isAnyMouseButtonDown(uint32_t &) const
{
  return false;
}

void EiScreen::getCursorCenter(int32_t &x, int32_t &y) const
{
  x = m_x + m_w / 2;
  y = m_y + m_h / 2;
}

void EiScreen::fakeMouseButton(ButtonID button, bool press)
{
  uint32_t code;

  if (!m_eiPointer)
    return;

  switch (button) {
  case kButtonLeft:
    code = 0x110; // BTN_LEFT
    break;
  case kButtonMiddle:
    code = 0x112; // BTN_MIDDLE
    break;
  case kButtonRight:
    code = 0x111; // BTN_RIGHT
    break;
  default:
    code = 0x110 + (button - 1);
    break;
  }

  ei_device_button_button(m_eiPointer, code, press);
  ei_device_frame(m_eiPointer, ei_now(m_ei));
}

void EiScreen::fakeMouseMove(int32_t x, int32_t y)
{
  // We get one motion event before enter() with the target position
  if (!m_isOnScreen) {
    m_cursorX = x;
    m_cursorY = y;
    return;
  }

  if (!m_eiAbs)
    return;

  ei_device_pointer_motion_absolute(m_eiAbs, x, y);
  ei_device_frame(m_eiAbs, ei_now(m_ei));
}

void EiScreen::fakeMouseRelativeMove(int32_t dx, int32_t dy) const
{
  if (!m_eiPointer)
    return;

  ei_device_pointer_motion(m_eiPointer, dx, dy);
  ei_device_frame(m_eiPointer, ei_now(m_ei));
}

void EiScreen::fakeMouseWheel(int32_t xDelta, int32_t yDelta) const
{
  if (!m_eiPointer)
    return;

  xDelta = mapClientScrollDirection(xDelta);
  yDelta = mapClientScrollDirection(yDelta);
  // libei and deskflow seem to use opposite directions, so we have
  // to send EI the opposite of the value received if we want to remain
  // compatible with other platforms (including X11).
  ei_device_scroll_discrete(m_eiPointer, -xDelta, -yDelta);
  ei_device_frame(m_eiPointer, ei_now(m_ei));
}

void EiScreen::fakeKey(uint32_t keycode, bool isDown) const
{
  if (!m_eiKeyboard)
    return;

  auto xkbKeycode = keycode + 8;
  m_keyState->updateXkbState(xkbKeycode, isDown);
  ei_device_keyboard_key(m_eiKeyboard, keycode, isDown);
  ei_device_frame(m_eiKeyboard, ei_now(m_ei));
}

void EiScreen::enable()
{
  // Nothing really to be done here
  if (m_clipboard && m_clipboard->isAvailable()) {
    m_clipboard->startMonitoring();
  }
}

void EiScreen::disable()
{
  if (m_clipboard && m_clipboard->isAvailable()) {
    m_clipboard->stopMonitoring();
  }
}

void EiScreen::enter()
{
  m_isOnScreen = true;
  if (!m_isPrimary) {
    ++m_sequenceNumber;
    if (m_eiPointer) {
      ei_device_start_emulating(m_eiPointer, m_sequenceNumber);
    }
    if (m_eiKeyboard) {
      ei_device_start_emulating(m_eiKeyboard, m_sequenceNumber);
    }
    if (m_eiAbs) {
      ei_device_start_emulating(m_eiAbs, m_sequenceNumber);
      fakeMouseMove(m_cursorX, m_cursorY);
    }
  } else {
    LOG_DEBUG("releasing input capture at x=%i y=%i", m_cursorX, m_cursorY);
    m_portalInputCapture->release(m_cursorX, m_cursorY);
  }
}

bool EiScreen::canLeave()
{
  return true;
}

void EiScreen::leave()
{
  if (!m_isPrimary) {
    if (m_eiPointer) {
      ei_device_stop_emulating(m_eiPointer);
    }
    if (m_eiKeyboard) {
      ei_device_stop_emulating(m_eiKeyboard);
    }
    if (m_eiAbs) {
      ei_device_stop_emulating(m_eiAbs);
    }
  }

  m_isOnScreen = false;
}

bool EiScreen::setClipboard(ClipboardID id, const IClipboard *clipboard)
{
  if (!clipboard || !m_clipboard || !m_clipboard->isAvailable()) {
    return false;
  }

  IClipboard *targetClipboard = m_clipboard->getClipboard(id);
  if (!targetClipboard) {
    return false;
  }

  return IClipboard::copy(targetClipboard, clipboard);
}

void EiScreen::checkClipboards()
{
  // do nothing, we're always up to date
  if (!m_clipboard || !m_clipboard->isAvailable()) {
    return;
  }

  if (m_clipboard->hasChanged()) {
    // Send clipboard change events for all clipboard types
    for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
      sendClipboardEvent(EventTypes::ClipboardChanged, id);
    }
    m_clipboard->resetChanged();
  }
}

void EiScreen::openScreensaver(bool notify)
{
  // FIXME
}

void EiScreen::closeScreensaver()
{
  // FIXME
}

void EiScreen::screensaver(bool activate)
{
  // FIXME
}

void EiScreen::resetOptions()
{
  // Should reset options to neutral, see setOptions().
  // We don't have ei-specific options, nothing to do here
}

void EiScreen::setOptions(const OptionsList &options)
{
  // We don't have ei-specific options, nothing to do here
}

bool EiScreen::isPrimary() const
{
  return m_isPrimary;
}

void EiScreen::updateShape()
{
  m_w = 1;
  m_h = 1;
  m_x = std::numeric_limits<uint32_t>::max();
  m_y = std::numeric_limits<uint32_t>::max();
  for (auto it = m_eiDevices.begin(); it != m_eiDevices.end(); it++) {
    auto idx = 0;
    struct ei_region *r;
    while ((r = ei_device_get_region(*it, idx++)) != nullptr) {
      m_x = std::min(ei_region_get_x(r), m_x);
      m_y = std::min(ei_region_get_y(r), m_y);
      m_w = std::max(ei_region_get_x(r) + ei_region_get_width(r), m_w);
      m_h = std::max(ei_region_get_y(r) + ei_region_get_height(r), m_h);
    }
  }

  LOG_DEBUG("logical output size: %dx%d@%d.%d", m_w, m_h, m_x, m_y);
  m_cursorX = m_x + m_w / 2;
  m_cursorY = m_y + m_h / 2;

  sendEvent(EventTypes::ScreenShapeChanged, nullptr);
}

void EiScreen::addDevice(struct ei_device *device)
{
  LOG_DEBUG("adding device %s", ei_device_get_name(device));

  // Noteworthy: EI in principle supports multiple devices with multiple
  // capabilities, so there may be more than one logical pointer (or even
  // multiple seats). Supporting this is ... tricky so for now we go the easy
  // route: one device for each capability. Note this may be the same device
  // if the first device comes with multiple capabilities.

  if (!m_eiPointer && ei_device_has_capability(device, EI_DEVICE_CAP_POINTER) &&
      ei_device_has_capability(device, EI_DEVICE_CAP_BUTTON) &&
      ei_device_has_capability(device, EI_DEVICE_CAP_SCROLL)) {
    m_eiPointer = ei_device_ref(device);
  }

  if (!m_eiKeyboard && ei_device_has_capability(device, EI_DEVICE_CAP_KEYBOARD)) {
    m_eiKeyboard = ei_device_ref(device);

    if (auto keymap = ei_device_keyboard_get_keymap(device);
        keymap && ei_keymap_get_type(keymap) == EI_KEYMAP_TYPE_XKB) {
      int fd = ei_keymap_get_fd(keymap);
      size_t len = ei_keymap_get_size(keymap);
      m_keyState->init(fd, len);
    } else {
      // We rely on the EIS implementation to give us a keymap, otherwise we
      // really have no idea what a keycode means (other than it's linux/input.h
      // code) Where the EIS implementation does not tell us, we just default to
      // whatever libxkbcommon thinks is default. At least this way we can
      // influence with env vars what we get
      LOG_WARN("keyboard device %s does not have a keymap, we are guessing", ei_device_get_name(device));
      m_keyState->initDefaultKeymap();
    }
    m_keyState->updateKeyMap();
  }

  if (!m_eiAbs && ei_device_has_capability(device, EI_DEVICE_CAP_POINTER_ABSOLUTE) &&
      ei_device_has_capability(device, EI_DEVICE_CAP_BUTTON) &&
      ei_device_has_capability(device, EI_DEVICE_CAP_SCROLL)) {
    m_eiAbs = ei_device_ref(device);
  }

  m_eiDevices.emplace_back(ei_device_ref(device));

  updateShape();
}

void EiScreen::removeDevice(struct ei_device *device)
{
  LOG_DEBUG("removing device %s", ei_device_get_name(device));

  if (device == m_eiPointer)
    m_eiPointer = ei_device_unref(m_eiPointer);
  if (device == m_eiKeyboard)
    m_eiKeyboard = ei_device_unref(m_eiKeyboard);
  if (device == m_eiAbs)
    m_eiAbs = ei_device_unref(m_eiAbs);

  for (auto it = m_eiDevices.begin(); it != m_eiDevices.end(); it++) {
    if (*it == device) {
      m_eiDevices.erase(it);
      ei_device_unref(device);
      break;
    }
  }

  updateShape();
}

void EiScreen::sendEvent(EventTypes type, void *data)
{
  m_events->addEvent(Event(type, getEventTarget(), data));
}

void EiScreen::sendClipboardEvent(EventTypes type, ClipboardID id) const
{
  auto *info = static_cast<ClipboardInfo *>(malloc(sizeof(ClipboardInfo)));
  if (info == nullptr) {
    LOG_ERR("malloc failed for ClipboardInfo");
    return;
  }
  info->m_id = id;
  info->m_sequenceNumber = m_sequenceNumber;

  // Use const_cast to call non-const sendEvent from const method
  const_cast<EiScreen *>(this)->sendEvent(type, info);
}

void EiScreen::setSequenceNumber(uint32_t seqNum)
{
  m_sequenceNumber = seqNum;
}

ButtonID EiScreen::mapButtonFromEvdev(ei_event *event) const
{
  switch (ei_event_button_get_button(event)) {
  case 0x110:
    return kButtonLeft;
  case 0x111:
    return kButtonRight;
  case 0x112:
    return kButtonMiddle;
  case 0x113:
    return kButtonExtra0;
  case 0x114:
    return kButtonExtra1;
  default:
    return kButtonNone;
  }

  return kButtonNone;
}

bool EiScreen::onHotkey(KeyID keyid, bool isPressed, KeyModifierMask mask)
{
  auto it = m_hotkeys.find(keyid);

  if (it == m_hotkeys.end()) {
    return false;
  }

  // Note: our mask (see on_key_event) only contains some modifiers
  // but we don't put a limitation on modifiers in the hotkeys. So some
  // key combinations may not work correctly, more effort is needed here.
  if (auto id = it->second.findByMask(mask); id != 0) {
    EventTypes type = isPressed ? EventTypes::PrimaryScreenHotkeyDown : EventTypes::PrimaryScreenHotkeyUp;
    sendEvent(type, HotKeyInfo::alloc(id));
    return true;
  }

  return false;
}

void EiScreen::onKeyEvent(ei_event *event)
{
  auto keycode = ei_event_keyboard_get_key(event);
  uint32_t keyval = keycode + 8;
  bool pressed = ei_event_keyboard_get_key_is_press(event);
  KeyID keyid = m_keyState->mapKeyFromKeyval(keyval);
  auto keybutton = static_cast<KeyButton>(keyval);

  m_keyState->updateXkbState(keyval, pressed);
  KeyModifierMask mask = m_keyState->pollActiveModifiers();

  LOG_DEBUG1("event: key %s keycode=%d keyid=%d mask=0x%x", pressed ? "press" : "release", keycode, keyid, mask);

  if (m_isPrimary && onHotkey(keyid, pressed, mask)) {
    return;
  }

  if (keyid != kKeyNone) {
    m_keyState->sendKeyEvent(getEventTarget(), pressed, false, keyid, mask, 1, keybutton);
  }
}

void EiScreen::onButtonEvent(ei_event *event)
{
  assert(m_isPrimary);

  auto buttonID = mapButtonFromEvdev(event);
  bool pressed = ei_event_button_get_is_press(event);
  KeyModifierMask mask = m_keyState->pollActiveModifiers();

  LOG_DEBUG1("event: button %s button=%d mask=0x%x", pressed ? "press" : "release", buttonID, mask);

  if (buttonID == kButtonNone) {
    LOG_DEBUG("event: button not recognized");
    return;
  }

  auto eventType = pressed ? EventTypes::PrimaryScreenButtonDown : EventTypes::PrimaryScreenButtonUp;

  sendEvent(eventType, ButtonInfo::alloc(buttonID, mask));
}

void EiScreen::onPointerScrollEvent(ei_event *event)
{
  // Ratio of 10 pixels == one wheel click because that's what mutter/gtk
  // use (for historical reasons).
  static const int s_pixelsPerWheelClick = 10;
  // Our logical wheel clicks are multiples 120, so we
  // convert between the two and keep the remainders because
  // we will very likely get subpixel scroll events.
  // This means a single pixel is 120/s_pixelToWheelRation in wheel values.
  const int s_pixelToWheelRatio = 120 / s_pixelsPerWheelClick;

  assert(m_isPrimary);

  auto dx = ei_event_scroll_get_dx(event);
  auto dy = ei_event_scroll_get_dy(event);
  struct ei_device *device = ei_event_get_device(event);

  LOG_DEBUG1("event: scroll (%.2f, %.2f)", dx, dy);

  auto *remainder = static_cast<struct ScrollRemainder *>(ei_device_get_user_data(device));
  if (!remainder) {
    remainder = new ScrollRemainder();
    ei_device_set_user_data(device, remainder);
  }

  dx += remainder->x;
  dy += remainder->y;

  double x;
  double y;
  double rx = modf(dx, &x);
  double ry = modf(dy, &y);

  assert(!std::isnan(x) && !std::isinf(x));
  assert(!std::isnan(y) && !std::isinf(y));

  // libei and deskflow seem to use opposite directions, so we have
  // to send the opposite of the value reported by EI if we want to
  // remain compatible with other platforms (including X11).
  if (x != 0 || y != 0)
    sendEvent(
        EventTypes::PrimaryScreenWheel,
        WheelInfo::alloc((int32_t)-x * s_pixelToWheelRatio, (int32_t)-y * s_pixelToWheelRatio)
    );

  remainder->x = rx;
  remainder->y = ry;
}

void EiScreen::onPointerScrollDiscreteEvent(ei_event *event)
{
  // both libei and deskflow use multiples of 120 to represent
  // one scroll wheel click event so we can just forward things
  // as-is.

  assert(m_isPrimary);

  auto dx = ei_event_scroll_get_discrete_dx(event);
  auto dy = ei_event_scroll_get_discrete_dy(event);

  LOG_DEBUG1("event: scroll discrete (%d, %d)", dx, dy);

  // libei and deskflow seem to use opposite directions, so we have
  // to send the opposite of the value reported by EI if we want to
  // remain compatible with other platforms (including X11).
  sendEvent(EventTypes::PrimaryScreenWheel, WheelInfo::alloc(-dx, -dy));
}

void EiScreen::onMotionEvent(ei_event *event)
{
  assert(m_isPrimary);

  auto dx = ei_event_pointer_get_dx(event);
  auto dy = ei_event_pointer_get_dy(event);

  if (m_isOnScreen) {
    LOG_DEBUG("event: motion on primary x=%i y=%i)", m_cursorX, m_cursorY);
    sendEvent(EventTypes::PrimaryScreenMotionOnPrimary, MotionInfo::alloc(m_cursorX, m_cursorY));
    if (m_portalInputCapture->isActive()) {
      m_portalInputCapture->release();
    }
  } else {
    m_bufferDX += dx;
    m_bufferDY += dy;
    auto pixelDx = static_cast<std::int32_t>(m_bufferDX);
    auto pixelDy = static_cast<std::int32_t>(m_bufferDY);
    if (pixelDx || pixelDy) {
      LOG_DEBUG1("event: motion on secondary x=%d y=%d", pixelDx, pixelDy);
      sendEvent(EventTypes::PrimaryScreenMotionOnSecondary, MotionInfo::alloc(pixelDx, pixelDy));
      m_bufferDX -= pixelDx;
      m_bufferDY -= pixelDy;
    }
  }
}

void EiScreen::onAbsMotionEvent(const ei_event *) const
{
  assert(m_isPrimary);
}

void EiScreen::handleConnectedToEisEvent(const Event &event)
{
  int fd = static_cast<EiConnectInfo *>(event.getData())->m_fd;
  LOG_DEBUG("eis connection established, fd=%d", fd);

  auto rc = ei_setup_backend_fd(m_ei, fd);
  if (rc != 0) {
    LOG_WARN("failed to set up ei: %s", strerror(-rc));
  }
}

void EiScreen::handlePortalSessionClosed()
{
  // Portal may or may not EI_EVENT_DISCONNECT us before sending the DBus Closed
  // signal. Let's clean up either way.
  LOG_DEBUG("eis screen handling portal session closed");
  cleanupEi();
  initEi();
}

void EiScreen::handleSystemEvent(const Event &)
{
  std::scoped_lock lock{m_mutex};

  // Only one ei_dispatch per system event, see the comment in
  // EiEventQueueBuffer::addEvent
  ei_dispatch(m_ei);
  struct ei_event *event;

  while ((event = ei_get_event(m_ei)) != nullptr) {
    auto type = ei_event_get_type(event);
    auto seat = ei_event_get_seat(event);
    auto device = ei_event_get_device(event);

    switch (type) {
    case EI_EVENT_CONNECT:
      LOG_DEBUG("connected to eis");
      break;
    case EI_EVENT_SEAT_ADDED:
      if (!m_eiSeat) {
        m_eiSeat = ei_seat_ref(seat);
        ei_seat_bind_capabilities(
            m_eiSeat, EI_DEVICE_CAP_POINTER, EI_DEVICE_CAP_POINTER_ABSOLUTE, EI_DEVICE_CAP_KEYBOARD,
            EI_DEVICE_CAP_BUTTON, EI_DEVICE_CAP_SCROLL, nullptr
        );
        LOG_DEBUG("ei: using seat %s", ei_seat_get_name(m_eiSeat));
        // we don't care about touch
      }
      break;
    case EI_EVENT_DEVICE_ADDED:
      if (seat == m_eiSeat) {
        addDevice(device);
      } else {
        LOG_INFO("seat %s is ignored", ei_seat_get_name(m_eiSeat));
      }
      break;
    case EI_EVENT_DEVICE_REMOVED:
      removeDevice(device);
      break;
    case EI_EVENT_SEAT_REMOVED:
      if (seat == m_eiSeat) {
        m_eiSeat = ei_seat_unref(m_eiSeat);
      }
      break;
    case EI_EVENT_DISCONNECT:
      // We're using libei which emulates the various seat/device remove events
      // so by the time we get here our EiScreen should be in a neutral state.
      //
      // We must release the xdg-portal InputCapture in case it is still active
      // so that the cursor is usable and not stuck on the deskflow server.
      LOG_WARN("disconnected from eis, will afterwards commence attempt to reconnect");
      if (m_isPrimary) {
        LOG_DEBUG("re-allocating portal input capture connection and releasing active captures");
        if (m_portalInputCapture) {
          if (m_portalInputCapture->isActive()) {
            m_portalInputCapture->release();
          }
          delete m_portalInputCapture;
          m_portalInputCapture = new PortalInputCapture(this, this->m_events);
        }
      }
      this->handlePortalSessionClosed();
      break;
    case EI_EVENT_DEVICE_PAUSED:
      LOG_DEBUG("device %s is paused", ei_device_get_name(device));
      break;
    case EI_EVENT_DEVICE_RESUMED:
      LOG_DEBUG("device %s is resumed", ei_device_get_name(device));
      if (!m_isPrimary && m_isOnScreen) {
        ei_device_start_emulating(device, ++m_sequenceNumber);
      }
      break;
    case EI_EVENT_KEYBOARD_MODIFIERS:
      // FIXME
      break;

    // events below are for a receiver context (barriers)
    case EI_EVENT_FRAME:
      break;
    case EI_EVENT_DEVICE_START_EMULATING:
      LOG_DEBUG("device %s started emulating", ei_device_get_name(device));
      break;
    case EI_EVENT_DEVICE_STOP_EMULATING:
      LOG_DEBUG("device %s stopped emulating", ei_device_get_name(device));
      break;
    case EI_EVENT_KEYBOARD_KEY:
      onKeyEvent(event);
      break;
    case EI_EVENT_BUTTON_BUTTON:
      onButtonEvent(event);
      break;
    case EI_EVENT_POINTER_MOTION:
      onMotionEvent(event);
      break;
    case EI_EVENT_POINTER_MOTION_ABSOLUTE:
      onAbsMotionEvent(event);
      break;
    case EI_EVENT_TOUCH_UP:
      break;
    case EI_EVENT_TOUCH_MOTION:
      break;
    case EI_EVENT_TOUCH_DOWN:
      break;
    case EI_EVENT_SCROLL_DELTA:
      onPointerScrollEvent(event);
      break;
    case EI_EVENT_SCROLL_DISCRETE:
      onPointerScrollDiscreteEvent(event);
      break;
    case EI_EVENT_SCROLL_STOP:
    case EI_EVENT_SCROLL_CANCEL:
      break;
    default:
      break;
    }
    ei_event_unref(event);
  }
}

void EiScreen::updateButtons()
{
  // libei relies on the EIS implementation to keep our button count correct,
  // so there's not much we need to/can do here.
}

IKeyState *EiScreen::getKeyState() const
{
  return m_keyState;
}

std::string EiScreen::getSecureInputApp() const
{
  throw std::runtime_error("get security input app not implemented");
}

EiScreen::HotKeyItem::HotKeyItem(std::uint32_t mask, std::uint32_t id) : mask(mask), id(id)
{
  // Todo: Implement
}

EiScreen::HotKeySet::HotKeySet(KeyID key) : m_id(key)
{
  // Todo: Implement
}

bool EiScreen::HotKeySet::removeById(std::uint32_t id)
{
  for (auto it = m_set.begin(); it != m_set.end(); ++it) {
    if (it->id == id) {
      m_set.erase(it);
      return true;
    }
  }
  return false;
}

void EiScreen::HotKeySet::addItem(HotKeyItem item)
{
  m_set.push_back(item);
}

std::uint32_t EiScreen::HotKeySet::findByMask(std::uint32_t mask) const
{
  for (const auto &item : m_set) {
    if (item.mask == mask) {
      return item.id;
    }
  }
  return 0;
}

} // namespace deskflow
