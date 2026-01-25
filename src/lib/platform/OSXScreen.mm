/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXScreen.h"

#include "arch/Arch.h"
#include "arch/ArchException.h"
#include "base/EventQueue.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "base/TMethodJob.h"
#include "client/Client.h"
#include "common/Settings.h"
#include "deskflow/ClientApp.h"
#include "deskflow/Clipboard.h"
#include "deskflow/DisplayInvalidException.h"
#include "deskflow/KeyMap.h"
#include "mt/CondVar.h"
#include "mt/Lock.h"
#include "mt/Mutex.h"
#include "mt/Thread.h"
#include "platform/OSXClipboard.h"
#include "platform/OSXEventQueueBuffer.h"
#include "platform/OSXKeyState.h"
#include "platform/OSXMediaKeySupport.h"
#include "platform/OSXPasteboardPeeker.h"
#include "platform/OSXScreenSaver.h"

#include <AppKit/NSEvent.h>
#include <AvailabilityMacros.h>
#include <IOKit/hidsystem/event_status_driver.h>
#include <libproc.h>
#include <mach-o/dyld.h>
#include <math.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

// The following creates a section that tells Mac OS X
// that it is OK to let us inject input in the login screen.
// Just the name of the section is important, not its contents.
__attribute__((used)) __attribute__((section("__CGPreLoginApp,__cgpreloginapp"))) static const char magic_section[] =
    "";
////////////////////////////////////////////////////////////

// This isn't in any Apple SDK that I know of as of yet.
enum
{
  kDeskflowEventMouseScroll = 11,
  kDeskflowMouseScrollAxisX = 'saxx',
  kDeskflowMouseScrollAxisY = 'saxy'
};

static const double kCarbonLoopWaitTimeout = 10.0;

int getSecureInputEventPID();
std::string getProcessName(int pid);

// TODO: upgrade deprecated function usage in these functions.
void setZeroSuppressionInterval();
void avoidSupression();
void logCursorVisibility();
void avoidHesitatingCursor();

//
// OSXScreen
//

bool OSXScreen::s_testedForGHOM = false;
bool OSXScreen::s_hasGHOM = false;

OSXScreen::OSXScreen(IEventQueue *events, bool isPrimary, bool enableLangSync, bool invertScrolling)
    : PlatformScreen(events, invertScrolling),
      m_isPrimary(isPrimary),
      m_isOnScreen(m_isPrimary),
      m_cursorPosValid(false),
      MouseButtonEventMap(NumButtonIDs),
      m_cursorHidden(false),
      m_keyState(nullptr),
      m_sequenceNumber(0),
      m_screensaver(nullptr),
      m_screensaverNotify(false),
      m_ownClipboard(false),
      m_clipboardTimer(nullptr),
      m_hiddenWindow(nullptr),
      m_userInputWindow(nullptr),
      m_switchEventHandlerRef(0),
      m_pmMutex(new Mutex),
      m_pmWatchThread(nullptr),
      m_pmThreadReady(new CondVar<bool>(m_pmMutex, false)),
      m_pmRootPort(0),
      m_activeModifierHotKey(0),
      m_activeModifierHotKeyMask(0),
      m_eventTapPort(nullptr),
      m_eventTapRLSR(nullptr),
      m_lastClickTime(0),
      m_clickState(1),
      m_lastSingleClickXCursor(0),
      m_lastSingleClickYCursor(0),
      m_events(events),
      m_impl(nullptr)
{
  m_displayID = CGMainDisplayID();
  if (!updateScreenShape(m_displayID, 0)) {
    throw DisplayInvalidException("failed to initialize screen shape");
  }

  try {
    m_screensaver = new OSXScreenSaver(m_events, getEventTarget());
    m_keyState = new OSXKeyState(m_events, AppUtil::instance().getKeyboardLayoutList(), enableLangSync);

    if (Settings::value(Settings::Core::PreventSleep).toBool()) {
      m_powerManager.disableSleep();
    }

    // only needed when running as a server.
    if (m_isPrimary) {
      // we can't pass options to show the dialog, this must be done by the gui.
      if (!AXIsProcessTrusted()) {
        throw std::runtime_error("assistive devices does not trust this process, allow it in system settings.");
      }
    }

    // install display manager notification handler
    CGDisplayRegisterReconfigurationCallback(displayReconfigurationCallback, this);

    // install fast user switching event handler
    EventTypeSpec switchEventTypes[2];
    switchEventTypes[0].eventClass = kEventClassSystem;
    switchEventTypes[0].eventKind = kEventSystemUserSessionDeactivated;
    switchEventTypes[1].eventClass = kEventClassSystem;
    switchEventTypes[1].eventKind = kEventSystemUserSessionActivated;
    EventHandlerUPP switchEventHandler = NewEventHandlerUPP(userSwitchCallback);
    InstallApplicationEventHandler(switchEventHandler, 2, switchEventTypes, this, &m_switchEventHandlerRef);
    DisposeEventHandlerUPP(switchEventHandler);

    constructMouseButtonEventMap();

    // watch for requests to sleep
    m_events->addHandler(EventTypes::OsxScreenConfirmSleep, getEventTarget(), [this](const auto &e) {
      handleConfirmSleep(e);
    });

    // create thread for monitoring system power state.
    *m_pmThreadReady = false;
    m_carbonLoopMutex = new Mutex();
    m_carbonLoopReady = new CondVar<bool>(m_carbonLoopMutex, false);
    LOG_DEBUG("starting watchSystemPowerThread");
    m_pmWatchThread = new Thread(new TMethodJob<OSXScreen>(this, &OSXScreen::watchSystemPowerThread));
  } catch (...) {
    m_events->removeHandler(EventTypes::OsxScreenConfirmSleep, getEventTarget());
    if (m_switchEventHandlerRef != 0) {
      RemoveEventHandler(m_switchEventHandlerRef);
    }

    CGDisplayRemoveReconfigurationCallback(displayReconfigurationCallback, this);

    delete m_keyState;
    delete m_screensaver;
    throw;
  }

  // install event handlers
  m_events->addHandler(EventTypes::System, m_events->getSystemTarget(), [this](const auto &e) {
    handleSystemEvent(e);
  });

  // install the platform event queue
  m_events->adoptBuffer(new OSXEventQueueBuffer(m_events));
}

OSXScreen::~OSXScreen()
{
  disable();

  m_events->adoptBuffer(nullptr);
  m_events->removeHandler(EventTypes::System, m_events->getSystemTarget());

  if (m_pmWatchThread) {
    // make sure the thread has setup the runloop.
    {
      Lock lock(m_pmMutex);
      while (!(bool)*m_pmThreadReady) {
        m_pmThreadReady->wait();
      }
    }

    // now exit the thread's runloop and wait for it to exit
    LOG_DEBUG("stopping watchSystemPowerThread");
    CFRunLoopStop(m_pmRunloop);
    m_pmWatchThread->wait();
    delete m_pmWatchThread;
    m_pmWatchThread = nullptr;
  }
  delete m_pmThreadReady;
  delete m_pmMutex;

  m_events->removeHandler(EventTypes::OsxScreenConfirmSleep, getEventTarget());

  RemoveEventHandler(m_switchEventHandlerRef);

  CGDisplayRemoveReconfigurationCallback(displayReconfigurationCallback, this);

  delete m_keyState;
  delete m_screensaver;

  delete m_carbonLoopMutex;
  delete m_carbonLoopReady;
}

void *OSXScreen::getEventTarget() const
{
  return const_cast<OSXScreen *>(this);
}

bool OSXScreen::getClipboard(ClipboardID, IClipboard *dst) const
{
  Clipboard::copy(dst, &m_pasteboard);
  return true;
}

void OSXScreen::getShape(int32_t &x, int32_t &y, int32_t &w, int32_t &h) const
{
  x = m_x;
  y = m_y;
  w = m_w;
  h = m_h;
}

void OSXScreen::getCursorPos(int32_t &x, int32_t &y) const
{
  CGEventRef event = CGEventCreate(nullptr);
  CGPoint mouse = CGEventGetLocation(event);
  x = mouse.x;
  y = mouse.y;
  m_cursorPosValid = true;
  m_xCursor = x;
  m_yCursor = y;
  CFRelease(event);
}

void OSXScreen::reconfigure(uint32_t activeSides)
{
  const static auto sidesText = sidesMaskToString(activeSides);
  LOG_DEBUG("active sides: %s (0x%02x)", sidesText.c_str(), activeSides);
  m_activeSides = activeSides;
}

uint32_t OSXScreen::activeSides()
{
  return m_activeSides;
}

void OSXScreen::warpCursor(int32_t x, int32_t y)
{
  // move cursor without generating events
  CGPoint pos;
  pos.x = x;
  pos.y = y;
  CGWarpMouseCursorPosition(pos);

  // save new cursor position
  m_xCursor = x;
  m_yCursor = y;
  m_cursorPosValid = true;
}

void OSXScreen::fakeInputBegin()
{
  // FIXME -- not implemented
}

void OSXScreen::fakeInputEnd()
{
  // FIXME -- not implemented
}

int32_t OSXScreen::getJumpZoneSize() const
{
  return 1;
}

bool OSXScreen::isAnyMouseButtonDown(uint32_t &buttonID) const
{
  if (m_buttonState.test(0)) {
    buttonID = kButtonLeft;
    return true;
  }

  return (GetCurrentButtonState() != 0);
}

void OSXScreen::getCursorCenter(int32_t &x, int32_t &y) const
{
  x = m_xCenter;
  y = m_yCenter;
}

uint32_t OSXScreen::registerHotKey(KeyID key, KeyModifierMask mask)
{
  // get mac virtual key and modifier mask matching deskflow key and mask
  uint32_t macKey, macMask;
  if (!m_keyState->mapDeskflowHotKeyToMac(key, mask, macKey, macMask)) {
    LOG_DEBUG("could not map hotkey id=%04x mask=%04x", key, mask);
    return 0;
  }

  // choose hotkey id
  uint32_t id;
  if (!m_oldHotKeyIDs.empty()) {
    id = m_oldHotKeyIDs.back();
    m_oldHotKeyIDs.pop_back();
  } else {
    id = m_hotKeys.size() + 1;
  }

  // if this hot key has modifiers only then we'll handle it specially
  EventHotKeyRef ref = nullptr;
  bool okay;
  if (key == kKeyNone) {
    if (m_modifierHotKeys.count(mask) > 0) {
      // already registered
      okay = false;
    } else {
      m_modifierHotKeys[mask] = id;
      okay = true;
    }
  } else {
    EventHotKeyID hkid = {'SNRG', (uint32_t)id};
    OSStatus status = RegisterEventHotKey(macKey, macMask, hkid, GetApplicationEventTarget(), 0, &ref);
    okay = (status == noErr);
    m_hotKeyToIDMap[HotKeyItem(macKey, macMask)] = id;
  }

  if (!okay) {
    m_oldHotKeyIDs.push_back(id);
    m_hotKeyToIDMap.erase(HotKeyItem(macKey, macMask));
    LOG_WARN(
        "failed to register hotkey %s (id=%04x mask=%04x)", deskflow::KeyMap::formatKey(key, mask).c_str(), key, mask
    );
    return 0;
  }

  m_hotKeys.insert(std::make_pair(id, HotKeyItem(ref, macKey, macMask)));

  LOG_DEBUG(
      "registered hotkey %s (id=%04x mask=%04x) as id=%d", deskflow::KeyMap::formatKey(key, mask).c_str(), key, mask, id
  );
  return id;
}

void OSXScreen::unregisterHotKey(uint32_t id)
{
  // look up hotkey
  HotKeyMap::iterator i = m_hotKeys.find(id);
  if (i == m_hotKeys.end()) {
    return;
  }

  // unregister with OS
  bool okay;
  if (i->second.getRef() != nullptr) {
    okay = (UnregisterEventHotKey(i->second.getRef()) == noErr);
  } else {
    okay = false;
    // XXX -- this is inefficient
    for (ModifierHotKeyMap::iterator j = m_modifierHotKeys.begin(); j != m_modifierHotKeys.end(); ++j) {
      if (j->second == id) {
        m_modifierHotKeys.erase(j);
        okay = true;
        break;
      }
    }
  }
  if (!okay) {
    LOG_WARN("failed to unregister hotkey id=%d", id);
  } else {
    LOG_DEBUG("unregistered hotkey id=%d", id);
  }

  // discard hot key from map and record old id for reuse
  m_hotKeyToIDMap.erase(i->second);
  m_hotKeys.erase(i);
  m_oldHotKeyIDs.push_back(id);
  if (m_activeModifierHotKey == id) {
    m_activeModifierHotKey = 0;
    m_activeModifierHotKeyMask = 0;
  }
}

void OSXScreen::constructMouseButtonEventMap()
{
  const CGEventType source[NumButtonIDs][3] = {
      {kCGEventLeftMouseUp, kCGEventLeftMouseDragged, kCGEventLeftMouseDown},
      {kCGEventRightMouseUp, kCGEventRightMouseDragged, kCGEventRightMouseDown},
      {kCGEventOtherMouseUp, kCGEventOtherMouseDragged, kCGEventOtherMouseDown},
      {kCGEventOtherMouseUp, kCGEventOtherMouseDragged, kCGEventOtherMouseDown},
      {kCGEventOtherMouseUp, kCGEventOtherMouseDragged, kCGEventOtherMouseDown}
  };

  for (uint16_t button = 0; button < NumButtonIDs; button++) {
    MouseButtonEventMapType new_map;
    for (uint16_t state = (uint32_t)kMouseButtonUp; state < kMouseButtonStateMax; state++) {
      CGEventType curEvent = source[button][state];
      new_map[state] = curEvent;
    }
    MouseButtonEventMap[button] = new_map;
  }
}

void OSXScreen::postMouseEvent(CGPoint &pos) const
{
  // check if cursor position is valid on the client display configuration
  // stkamp@users.sourceforge.net
  CGDisplayCount displayCount = 0;
  CGGetDisplaysWithPoint(pos, 0, nullptr, &displayCount);
  if (displayCount == 0) {
    // cursor position invalid - clamp to bounds of last valid display.
    // find the last valid display using the last cursor position.
    displayCount = 0;
    CGDirectDisplayID displayID;
    CGGetDisplaysWithPoint(CGPointMake(m_xCursor, m_yCursor), 1, &displayID, &displayCount);
    if (displayCount != 0) {
      CGRect displayRect = CGDisplayBounds(displayID);
      if (pos.x < displayRect.origin.x) {
        pos.x = displayRect.origin.x;
      } else if (pos.x > displayRect.origin.x + displayRect.size.width - 1) {
        pos.x = displayRect.origin.x + displayRect.size.width - 1;
      }
      if (pos.y < displayRect.origin.y) {
        pos.y = displayRect.origin.y;
      } else if (pos.y > displayRect.origin.y + displayRect.size.height - 1) {
        pos.y = displayRect.origin.y + displayRect.size.height - 1;
      }
    }
  }

  CGEventType type = kCGEventMouseMoved;

  int8_t button = m_buttonState.getFirstButtonDown();
  if (button != -1) {
    MouseButtonEventMapType thisButtonType = MouseButtonEventMap[button];
    type = thisButtonType[kMouseButtonDragged];
  }

  CGEventRef event = CGEventCreateMouseEvent(nullptr, type, pos, static_cast<CGMouseButton>(button));

  // Dragging events also need the click state
  CGEventSetIntegerValueField(event, kCGMouseEventClickState, m_clickState);

  // Fix for sticky keys
  CGEventFlags modifiers = m_keyState->getModifierStateAsOSXFlags();
  CGEventSetFlags(event, modifiers);

  // Set movement deltas to fix issues with certain 3D programs
  SInt64 deltaX = pos.x;
  deltaX -= m_xCursor;

  SInt64 deltaY = pos.y;
  deltaY -= m_yCursor;

  CGEventSetIntegerValueField(event, kCGMouseEventDeltaX, deltaX);
  CGEventSetIntegerValueField(event, kCGMouseEventDeltaY, deltaY);

  double deltaFX = deltaX;
  double deltaFY = deltaY;

  CGEventSetDoubleValueField(event, kCGMouseEventDeltaX, deltaFX);
  CGEventSetDoubleValueField(event, kCGMouseEventDeltaY, deltaFY);

  CGEventPost(kCGHIDEventTap, event);

  CFRelease(event);
}

void OSXScreen::fakeMouseButton(ButtonID id, bool press)
{
  // Buttons are indexed from one, but the button down array is indexed from zero
  uint32_t index = mapDeskflowButtonToMac(id) - kButtonLeft;
  if (index >= NumButtonIDs) {
    return;
  }

  CGPoint pos;
  if (!m_cursorPosValid) {
    int32_t x, y;
    getCursorPos(x, y);
  }
  pos.x = m_xCursor;
  pos.y = m_yCursor;

  // variable used to detect mouse coordinate differences between
  // old & new mouse clicks. Used in double click detection.
  int32_t xDiff = m_xCursor - m_lastSingleClickXCursor;
  int32_t yDiff = m_yCursor - m_lastSingleClickYCursor;
  double diff = sqrt(xDiff * xDiff + yDiff * yDiff);
  // max sqrt(x^2 + y^2) difference allowed to double click
  // since we don't have double click distance in NX APIs
  // we define our own defaults.
  const double maxDiff = sqrt(2) + 0.0001;

  double clickTime = [NSEvent doubleClickInterval];

  // As long as the click is within the time window and distance window
  // increase clickState (double click, triple click, etc)
  // This will allow for higher than triple click but the quartz documenation
  // does not specify that this should be limited to triple click
  if (press) {
    if ((Arch::time() - m_lastClickTime) <= clickTime && diff <= maxDiff) {
      m_clickState++;
    } else {
      m_clickState = 1;
    }

    m_lastClickTime = Arch::time();
  }

  if (m_clickState == 1) {
    m_lastSingleClickXCursor = m_xCursor;
    m_lastSingleClickYCursor = m_yCursor;
  }

  EMouseButtonState state = press ? kMouseButtonDown : kMouseButtonUp;

  LOG_DEBUG1("faking mouse button id: %d press: %s", index, press ? "pressed" : "released");

  MouseButtonEventMapType thisButtonMap = MouseButtonEventMap[index];
  CGEventType type = thisButtonMap[state];

  CGEventRef event = CGEventCreateMouseEvent(nullptr, type, pos, static_cast<CGMouseButton>(index));

  CGEventSetIntegerValueField(event, kCGMouseEventClickState, m_clickState);

  // Fix for sticky keys
  CGEventFlags modifiers = m_keyState->getModifierStateAsOSXFlags();
  CGEventSetFlags(event, modifiers);

  m_buttonState.set(index, state);
  CGEventPost(kCGHIDEventTap, event);

  CFRelease(event);
}

void OSXScreen::fakeMouseMove(int32_t x, int32_t y)
{
  // synthesize event
  CGPoint pos;
  pos.x = x;
  pos.y = y;
  postMouseEvent(pos);

  // save new cursor position
  m_xCursor = static_cast<int32_t>(pos.x);
  m_yCursor = static_cast<int32_t>(pos.y);
  m_cursorPosValid = true;
}

void OSXScreen::fakeMouseRelativeMove(int32_t dx, int32_t dy) const
{
  // OS X does not appear to have a fake relative mouse move function.
  // simulate it by getting the current mouse position and adding to
  // that.  this can yield the wrong answer but there's not much else
  // we can do.

  // get current position
  CGEventRef event = CGEventCreate(nullptr);
  CGPoint oldPos = CGEventGetLocation(event);
  CFRelease(event);

  // synthesize event
  CGPoint pos;
  m_xCursor = static_cast<int32_t>(oldPos.x);
  m_yCursor = static_cast<int32_t>(oldPos.y);
  pos.x = oldPos.x + dx;
  pos.y = oldPos.y + dy;
  postMouseEvent(pos);

  // we now assume we don't know the current cursor position
  m_cursorPosValid = false;
}

void OSXScreen::fakeMouseWheel(int32_t xDelta, int32_t yDelta) const
{
  if (xDelta != 0 || yDelta != 0) {
    // create a scroll event, post it and release it.  not sure if kCGScrollEventUnitLine
    // is the right choice here over kCGScrollEventUnitPixel
    CGEventRef scrollEvent = CGEventCreateScrollWheelEvent(
        nullptr, kCGScrollEventUnitLine, 2, mapScrollWheelFromDeskflow(yDelta), mapScrollWheelFromDeskflow(xDelta)
    );

    // Fix for sticky keys
    CGEventFlags modifiers = m_keyState->getModifierStateAsOSXFlags();
    CGEventSetFlags(scrollEvent, modifiers);

    CGEventPost(kCGHIDEventTap, scrollEvent);
    CFRelease(scrollEvent);
  }
}

void OSXScreen::showCursor()
{
  LOG_DEBUG("showing cursor");

  CFStringRef propertyString = CFStringCreateWithCString(nullptr, "SetsCursorInBackground", kCFStringEncodingMacRoman);

  CGSSetConnectionProperty(_CGSDefaultConnection(), _CGSDefaultConnection(), propertyString, kCFBooleanTrue);

  CFRelease(propertyString);

  CGError error = CGDisplayShowCursor(m_displayID);
  if (error != kCGErrorSuccess) {
    LOG_ERR("failed to show cursor, error=%d", error);
  }

  // appears to fix "mouse randomly not showing" bug
  CGAssociateMouseAndMouseCursorPosition(true);

  logCursorVisibility();

  m_cursorHidden = false;
}

void OSXScreen::hideCursor()
{
  LOG_DEBUG("hiding cursor");

  CFStringRef propertyString = CFStringCreateWithCString(nullptr, "SetsCursorInBackground", kCFStringEncodingMacRoman);

  CGSSetConnectionProperty(_CGSDefaultConnection(), _CGSDefaultConnection(), propertyString, kCFBooleanTrue);

  CFRelease(propertyString);

  CGError error = CGDisplayHideCursor(m_displayID);
  if (error != kCGErrorSuccess) {
    LOG_ERR("failed to hide cursor, error=%d", error);
  }

  // appears to fix "mouse randomly not hiding" bug
  CGAssociateMouseAndMouseCursorPosition(true);

  logCursorVisibility();

  m_cursorHidden = true;
}

void OSXScreen::enable()
{
  // watch the clipboard
  m_clipboardTimer = m_events->newTimer(1.0, nullptr);
  m_events->addHandler(EventTypes::Timer, m_clipboardTimer, [this](const auto &) { checkClipboards(); });

  if (m_isPrimary) {
    // FIXME -- start watching jump zones

    // kCGEventTapOptionDefault = 0x00000000 (Missing in 10.4, so specified literally)
    m_eventTapPort = CGEventTapCreate(
        kCGHIDEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault, kCGEventMaskForAllEvents, handleCGInputEvent,
        this
    );
  } else {
    // FIXME -- prevent system from entering power save mode

    hideCursor();

    // warp the mouse to the cursor center
    fakeMouseMove(m_xCenter, m_yCenter);

    // there may be a better way to do this, but we register an event handler even if we're
    // not on the primary display (acting as a client). This way, if a local event comes in
    // (either keyboard or mouse), we can make sure to show the cursor if we've hidden it.
    m_eventTapPort = CGEventTapCreate(
        kCGHIDEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault, kCGEventMaskForAllEvents,
        handleCGInputEventSecondary, this
    );
  }

  if (m_eventTapPort) {
    m_eventTapRLSR = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, m_eventTapPort, 0);
    if (m_eventTapRLSR) {
      CFRunLoopAddSource(CFRunLoopGetCurrent(), m_eventTapRLSR, kCFRunLoopDefaultMode);
    } else {
      LOG_ERR("failed to create a CFRunLoopSourceRef for the quartz event tap");
    }
  } else {
    LOG_ERR("failed to create quartz event tap");
  }
}

void OSXScreen::disable()
{
  showCursor();

  // FIXME -- stop watching jump zones, stop capturing input

  if (m_eventTapRLSR) {
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), m_eventTapRLSR, kCFRunLoopDefaultMode);
    CFRelease(m_eventTapRLSR);
    m_eventTapRLSR = nullptr;
  }

  if (m_eventTapPort) {
    CGEventTapEnable(m_eventTapPort, false);
    CFRelease(m_eventTapPort);
    m_eventTapPort = nullptr;
  }
  // FIXME -- allow system to enter power saving mode

  // uninstall clipboard timer
  if (m_clipboardTimer != nullptr) {
    m_events->removeHandler(EventTypes::Timer, m_clipboardTimer);
    m_events->deleteTimer(m_clipboardTimer);
    m_clipboardTimer = nullptr;
  }

  m_isOnScreen = m_isPrimary;
}

void OSXScreen::enter()
{
  m_isOnScreen = true;
  showCursor();

  if (m_isPrimary) {
    setZeroSuppressionInterval();
  } else {
    // reset buttons
    m_buttonState.reset();

    // wakes the client screen
    io_registry_entry_t entry =
        IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/IOResources/IODisplayWrangler");

    if (entry != MACH_PORT_NULL) {
      IORegistryEntrySetCFProperty(entry, CFSTR("IORequestIdle"), kCFBooleanFalse);
      IOObjectRelease(entry);
    }

    avoidSupression();
  }
}

bool OSXScreen::canLeave()
{
  return true;
}

void OSXScreen::leave()
{
  hideCursor();

  if (m_isPrimary) {
    avoidHesitatingCursor();
  }

  // now off screen
  m_isOnScreen = false;
}

bool OSXScreen::setClipboard(ClipboardID, const IClipboard *src)
{
  if (src != nullptr) {
    LOG_DEBUG("setting clipboard");
    Clipboard::copy(&m_pasteboard, src);
  }
  return true;
}

void OSXScreen::checkClipboards()
{
  LOG_DEBUG2("checking clipboard");
  if (m_pasteboard.synchronize()) {
    LOG_DEBUG("clipboard changed");
    sendClipboardEvent(EventTypes::ClipboardGrabbed, kClipboardClipboard);
    sendClipboardEvent(EventTypes::ClipboardGrabbed, kClipboardSelection);
  }
}

void OSXScreen::openScreensaver(bool notify)
{
  m_screensaverNotify = notify;
  if (!m_screensaverNotify) {
    m_screensaver->disable();
  }
}

void OSXScreen::closeScreensaver()
{
  if (!m_screensaverNotify) {
    m_screensaver->enable();
  }
}

void OSXScreen::screensaver(bool activate)
{
  if (activate) {
    m_screensaver->activate();
  } else {
    m_screensaver->deactivate();
  }
}

void OSXScreen::resetOptions()
{
  // no options
}

void OSXScreen::setOptions(const OptionsList &)
{
  // no options
}

void OSXScreen::setSequenceNumber(uint32_t seqNum)
{
  m_sequenceNumber = seqNum;
}

bool OSXScreen::isPrimary() const
{
  return m_isPrimary;
}

void OSXScreen::sendEvent(EventTypes type, void *data) const
{
  m_events->addEvent(Event(type, getEventTarget(), data));
}

void OSXScreen::sendClipboardEvent(EventTypes type, ClipboardID id) const
{
  ClipboardInfo *info = (ClipboardInfo *)malloc(sizeof(ClipboardInfo));
  info->m_id = id;
  info->m_sequenceNumber = m_sequenceNumber;
  sendEvent(type, info);
}

void OSXScreen::handleSystemEvent(const Event &event)
{
  EventRef *carbonEvent = static_cast<EventRef *>(event.getData());
  assert(carbonEvent != nullptr);

  uint32_t eventClass = GetEventClass(*carbonEvent);

  switch (eventClass) {
  case kEventClassMouse:
    switch (GetEventKind(*carbonEvent)) {
    case kDeskflowEventMouseScroll: {
      OSStatus r;
      long xScroll;
      long yScroll;

      // get scroll amount
      r = GetEventParameter(
          *carbonEvent, kDeskflowMouseScrollAxisX, typeSInt32, nullptr, sizeof(xScroll), nullptr, &xScroll
      );
      if (r != noErr) {
        xScroll = 0;
      }
      r = GetEventParameter(
          *carbonEvent, kDeskflowMouseScrollAxisY, typeSInt32, nullptr, sizeof(yScroll), nullptr, &yScroll
      );
      if (r != noErr) {
        yScroll = 0;
      }

      if (xScroll != 0 || yScroll != 0) {
        onMouseWheel(-mapScrollWheelToDeskflow(xScroll), mapScrollWheelToDeskflow(yScroll));
      }
    }
    }
    break;

  case kEventClassKeyboard:
    switch (GetEventKind(*carbonEvent)) {
    case kEventHotKeyPressed:
    case kEventHotKeyReleased:
      onHotKey(*carbonEvent);
      break;
    }

    break;

  case kEventClassWindow:
    // 2nd param was formerly GetWindowEventTarget(m_userInputWindow) which is 32-bit only,
    // however as m_userInputWindow is never initialized to anything we can take advantage of
    // the fact that GetWindowEventTarget(nullptr) == nullptr
    SendEventToEventTarget(*carbonEvent, nullptr);
    switch (GetEventKind(*carbonEvent)) {
    case kEventWindowActivated:
      LOG_DEBUG1("window activated");
      break;

    case kEventWindowDeactivated:
      LOG_DEBUG1("window deactivated");
      break;

    case kEventWindowFocusAcquired:
      LOG_DEBUG1("focus acquired");
      break;

    case kEventWindowFocusRelinquish:
      LOG_DEBUG1("focus released");
      break;
    }
    break;

  default:
    SendEventToEventTarget(*carbonEvent, GetEventDispatcherTarget());
    break;
  }
}

bool OSXScreen::onMouseMove()
{
  // when we receive a mouse-move event, it is possible it was queued for a period
  // and that the mouse has already moved again since then.  to handle this, we need
  // to query the current mouse position rather than using the position in the event.
  CGEventRef event = CGEventCreate(NULL);
  CGPoint pos = CGEventGetLocation(event);
  CFRelease(event);
  CGFloat mx = pos.x;
  CGFloat my = pos.y;

  LOG_DEBUG2("mouse move %+f,%+f", mx, my);

  CGFloat x = mx - m_xCursor;
  CGFloat y = my - m_yCursor;

  if ((x == 0 && y == 0) || (mx == m_xCenter && mx == m_yCenter)) {
    return true;
  }

  // save position to compute delta of next motion
  m_xCursor = (int32_t)mx;
  m_yCursor = (int32_t)my;

  if (m_isOnScreen) {
    // motion on primary screen
    sendEvent(EventTypes::PrimaryScreenMotionOnPrimary, MotionInfo::alloc(m_xCursor, m_yCursor));
  } else {
    // motion on secondary screen.  warp mouse back to
    // center.
    warpCursor(m_xCenter, m_yCenter);

    // examine the motion.  if it's about the distance
    // from the center of the screen to an edge then
    // it's probably a bogus motion that we want to
    // ignore (see warpCursorNoFlush() for a further
    // description).
    static int32_t bogusZoneSize = 10;
    if (-x + bogusZoneSize > m_xCenter - m_x || x + bogusZoneSize > m_x + m_w - m_xCenter ||
        -y + bogusZoneSize > m_yCenter - m_y || y + bogusZoneSize > m_y + m_h - m_yCenter) {
      LOG_DEBUG("dropped bogus motion %+d,%+d", x, y);
    } else {
      // send motion
      // Accumulate together the move into the running total
      static CGFloat m_xFractionalMove = 0;
      static CGFloat m_yFractionalMove = 0;

      m_xFractionalMove += x;
      m_yFractionalMove += y;

      // Return the integer part
      int32_t intX = (int32_t)m_xFractionalMove;
      int32_t intY = (int32_t)m_yFractionalMove;

      // And keep only the fractional part
      m_xFractionalMove -= intX;
      m_yFractionalMove -= intY;
      sendEvent(EventTypes::PrimaryScreenMotionOnSecondary, MotionInfo::alloc(intX, intY));
    }
  }

  return true;
}

bool OSXScreen::onMouseButton(bool pressed, uint16_t macButton)
{
  // Buttons 2 and 3 are inverted on the mac
  ButtonID button = mapMacButtonToDeskflow(macButton);

  if (pressed) {
    LOG_DEBUG1("event: button press button=%d", button);
    if (button != kButtonNone) {
      KeyModifierMask mask = m_keyState->getActiveModifiers();
      sendEvent(EventTypes::PrimaryScreenButtonDown, ButtonInfo::alloc(button, mask));
    }
  } else {
    LOG_DEBUG1("event: button release button=%d", button);
    if (button != kButtonNone) {
      KeyModifierMask mask = m_keyState->getActiveModifiers();
      sendEvent(EventTypes::PrimaryScreenButtonUp, ButtonInfo::alloc(button, mask));
    }
  }

  return true;
}

bool OSXScreen::onMouseWheel(int32_t xDelta, int32_t yDelta) const
{
  LOG_DEBUG1("event: button wheel delta=%+d,%+d", xDelta, yDelta);
  sendEvent(EventTypes::PrimaryScreenWheel, WheelInfo::alloc(xDelta, yDelta));
  return true;
}

void OSXScreen::displayReconfigurationCallback(
    CGDirectDisplayID displayID, CGDisplayChangeSummaryFlags flags, void *inUserData
)
{
  OSXScreen *screen = (OSXScreen *)inUserData;

  // Closing or opening the lid when an external monitor is
  // connected causes an kCGDisplayBeginConfigurationFlag event
  CGDisplayChangeSummaryFlags mask = kCGDisplayBeginConfigurationFlag | kCGDisplayMovedFlag | kCGDisplaySetModeFlag |
                                     kCGDisplayAddFlag | kCGDisplayRemoveFlag | kCGDisplayEnabledFlag |
                                     kCGDisplayDisabledFlag | kCGDisplayMirrorFlag | kCGDisplayUnMirrorFlag |
                                     kCGDisplayDesktopShapeChangedFlag;

  LOG_DEBUG1("event: display was reconfigured: %x %x %x", flags, mask, flags & mask);

  if (flags & mask) { /* Something actually did change */
    LOG_DEBUG1("event: screen changed shape; refreshing dimensions");
    if (!screen->updateScreenShape(displayID, flags)) {
      LOG_ERR("failed to update screen shape during display reconfiguration");
    }
  }
}

bool OSXScreen::onKey(CGEventRef event)
{
  CGEventType eventKind = CGEventGetType(event);

  // get the key and active modifiers
  uint32_t virtualKey = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
  CGEventFlags macMask = CGEventGetFlags(event);
  LOG_DEBUG1("event: Key event kind: %d, keycode=%d", eventKind, virtualKey);

  // Special handling to track state of modifiers
  if (eventKind == kCGEventFlagsChanged) {
    // get old and new modifier state
    KeyModifierMask oldMask = getActiveModifiers();
    KeyModifierMask newMask = m_keyState->mapModifiersFromOSX(macMask);
    m_keyState->handleModifierKeys(getEventTarget(), oldMask, newMask);

    // if the current set of modifiers exactly matches a modifiers-only
    // hot key then generate a hot key down event.
    if (m_activeModifierHotKey == 0) {
      if (m_modifierHotKeys.count(newMask) > 0) {
        m_activeModifierHotKey = m_modifierHotKeys[newMask];
        m_activeModifierHotKeyMask = newMask;
        m_events->addEvent(
            Event(EventTypes::PrimaryScreenHotkeyDown, getEventTarget(), HotKeyInfo::alloc(m_activeModifierHotKey))
        );
      }
    }

    // if a modifiers-only hot key is active and should no longer be
    // then generate a hot key up event.
    else if (m_activeModifierHotKey != 0) {
      KeyModifierMask mask = (newMask & m_activeModifierHotKeyMask);
      if (mask != m_activeModifierHotKeyMask) {
        m_events->addEvent(
            Event(EventTypes::PrimaryScreenHotkeyUp, getEventTarget(), HotKeyInfo::alloc(m_activeModifierHotKey))
        );
        m_activeModifierHotKey = 0;
        m_activeModifierHotKeyMask = 0;
      }
    }

    return true;
  }

  // check for hot key
  HotKeyToIDMap::const_iterator i =
      m_hotKeyToIDMap.find(HotKeyItem(virtualKey, m_keyState->mapModifiersToCarbon(macMask) & 0xff00u));
  if (i != m_hotKeyToIDMap.end()) {
    uint32_t id = i->second;

    // determine event type
    EventTypes type;
    if (eventKind == kCGEventKeyDown) {
      type = EventTypes::PrimaryScreenHotkeyDown;
    } else if (eventKind == kCGEventKeyUp) {
      type = EventTypes::PrimaryScreenHotkeyUp;
    } else {
      return false;
    }

    m_events->addEvent(Event(type, getEventTarget(), HotKeyInfo::alloc(id)));

    return true;
  }

  // decode event type
  bool down = (eventKind == kCGEventKeyDown);
  bool up = (eventKind == kCGEventKeyUp);
  bool isRepeat = (CGEventGetIntegerValueField(event, kCGKeyboardEventAutorepeat) == 1);

  // map event to keys
  KeyModifierMask mask;
  OSXKeyState::KeyIDs keys;
  KeyButton button = m_keyState->mapKeyFromEvent(keys, &mask, event);
  if (button == 0) {
    return false;
  }

  // check for AltGr in mask.  if set we send neither the AltGr nor
  // the super modifiers to clients then remove AltGr before passing
  // the modifiers to onKey.
  KeyModifierMask sendMask = (mask & ~KeyModifierAltGr);
  if ((mask & KeyModifierAltGr) != 0) {
    sendMask &= ~KeyModifierSuper;
  }
  mask &= ~KeyModifierAltGr;

  // update button state
  if (down) {
    m_keyState->onKey(button, true, mask);
  } else if (up) {
    if (!m_keyState->isKeyDown(button)) {
      // up event for a dead key.  throw it away.
      return false;
    }
    m_keyState->onKey(button, false, mask);
  }

  // send key events
  for (OSXKeyState::KeyIDs::const_iterator i = keys.begin(); i != keys.end(); ++i) {
    m_keyState->sendKeyEvent(getEventTarget(), down, isRepeat, *i, sendMask, 1, button);
  }

  return true;
}

void OSXScreen::onMediaKey(CGEventRef event)
{
  KeyID keyID;
  bool down;
  bool isRepeat;

  if (!getMediaKeyEventInfo(event, &keyID, &down, &isRepeat)) {
    LOG_ERR("Failed to decode media key event");
    return;
  }

  LOG_DEBUG2("Media key event: keyID=0x%02x, %s, repeat=%s", keyID, (down ? "down" : "up"), (isRepeat ? "yes" : "no"));

  KeyButton button = 0;
  KeyModifierMask mask = m_keyState->getActiveModifiers();
  m_keyState->sendKeyEvent(getEventTarget(), down, isRepeat, keyID, mask, 1, button);
}

bool OSXScreen::onHotKey(EventRef event) const
{
  // get the hotkey id
  EventHotKeyID hkid;
  GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID, nullptr, sizeof(EventHotKeyID), nullptr, &hkid);
  uint32_t id = hkid.id;

  // determine event type
  EventTypes type;
  uint32_t eventKind = GetEventKind(event);
  if (eventKind == kEventHotKeyPressed) {
    type = EventTypes::PrimaryScreenHotkeyDown;
  } else if (eventKind == kEventHotKeyReleased) {
    type = EventTypes::PrimaryScreenHotkeyUp;
  } else {
    return false;
  }

  m_events->addEvent(Event(type, getEventTarget(), HotKeyInfo::alloc(id)));

  return true;
}

ButtonID OSXScreen::mapDeskflowButtonToMac(uint16_t button) const
{
  switch (button) {
  case 1:
    return kButtonLeft;
  case 2:
    return kMacButtonMiddle;
  case 3:
    return kMacButtonRight;
  }

  return static_cast<ButtonID>(button);
}

ButtonID OSXScreen::mapMacButtonToDeskflow(uint16_t macButton) const
{
  switch (macButton) {
  case 1:
    return kButtonLeft;

  case 2:
    return kButtonRight;

  case 3:
    return kButtonMiddle;
  }

  return static_cast<ButtonID>(macButton);
}

int32_t OSXScreen::mapScrollWheelToDeskflow(int32_t x) const
{
  // return accelerated scrolling
  double d = (1.0 + getScrollSpeed()) * x;
  return static_cast<int32_t>(120.0 * d);
}

int32_t OSXScreen::mapScrollWheelFromDeskflow(int32_t x) const
{
  // use server's acceleration with a little boost since other platforms
  // take one wheel step as a larger step than the mac does.
  auto result = static_cast<int32_t>(3.0 * x / 120.0);
  return mapClientScrollDirection(result);
}

double OSXScreen::getScrollSpeed() const
{
  double scaling = 0.0;

  CFPropertyListRef pref = ::CFPreferencesCopyValue(
      CFSTR("com.apple.scrollwheel.scaling"), kCFPreferencesAnyApplication, kCFPreferencesCurrentUser,
      kCFPreferencesAnyHost
  );
  if (pref != nullptr) {
    CFTypeID id = CFGetTypeID(pref);
    if (id == CFNumberGetTypeID()) {
      CFNumberRef value = static_cast<CFNumberRef>(pref);
      if (CFNumberGetValue(value, kCFNumberDoubleType, &scaling)) {
        if (scaling < 0.0) {
          scaling = 0.0;
        }
      }
    }
    CFRelease(pref);
  }

  return scaling;
}

void OSXScreen::updateButtons()
{
  uint32_t buttons = GetCurrentButtonState();

  m_buttonState.overwrite(buttons);
}

IKeyState *OSXScreen::getKeyState() const
{
  return m_keyState;
}

bool OSXScreen::updateScreenShape(const CGDirectDisplayID, const CGDisplayChangeSummaryFlags flags)
{
  return updateScreenShape();
}

bool OSXScreen::updateScreenShape()
{
  // get info for each display
  CGDisplayCount displayCount = 0;

  if (CGGetActiveDisplayList(0, nullptr, &displayCount) != CGDisplayNoErr) {
    return false;
  }

  if (displayCount == 0) {
    return false;
  }

  CGDirectDisplayID *displays = new CGDirectDisplayID[displayCount];
  if (displays == nullptr) {
    return false;
  }

  if (CGGetActiveDisplayList(displayCount, displays, &displayCount) != CGDisplayNoErr) {
    delete[] displays;
    return false;
  }

  // get smallest rect enclosing all display rects
  CGRect totalBounds = CGRectZero;
  for (CGDisplayCount i = 0; i < displayCount; ++i) {
    CGRect bounds = CGDisplayBounds(displays[i]);
    totalBounds = CGRectUnion(totalBounds, bounds);
  }

  // get shape of default screen
  m_x = (int32_t)totalBounds.origin.x;
  m_y = (int32_t)totalBounds.origin.y;
  m_w = (int32_t)totalBounds.size.width;
  m_h = (int32_t)totalBounds.size.height;

  // get center of default screen
  CGDirectDisplayID main = CGMainDisplayID();
  const CGRect rect = CGDisplayBounds(main);
  m_xCenter = (rect.origin.x + rect.size.width) / 2;
  m_yCenter = (rect.origin.y + rect.size.height) / 2;

  delete[] displays;
  // We want to notify the peer screen whether we are primary screen or not
  sendEvent(EventTypes::ScreenShapeChanged);

  LOG_DEBUG(
      "screen shape: center=%d,%d size=%dx%d on %u %s", m_x, m_y, m_w, m_h, displayCount,
      (displayCount == 1) ? "display" : "displays"
  );

  return true;
}

#pragma mark -

//
// FAST USER SWITCH NOTIFICATION SUPPORT
//
// OSXScreen::userSwitchCallback(void*)
//
// gets called if a fast user switch occurs
//

pascal OSStatus OSXScreen::userSwitchCallback(EventHandlerCallRef nextHandler, EventRef theEvent, void *inUserData)
{
  OSXScreen *screen = (OSXScreen *)inUserData;
  uint32_t kind = GetEventKind(theEvent);
  IEventQueue *events = screen->getEvents();

  if (kind == kEventSystemUserSessionDeactivated) {
    LOG_DEBUG("user session deactivated");
    events->addEvent(Event(EventTypes::ScreenSuspend, screen->getEventTarget()));
  } else if (kind == kEventSystemUserSessionActivated) {
    LOG_DEBUG("user session activated");
    events->addEvent(Event(EventTypes::ScreenResume, screen->getEventTarget()));
  }
  return (CallNextEventHandler(nextHandler, theEvent));
}

#pragma mark -

//
// SLEEP/WAKEUP NOTIFICATION SUPPORT
//
// OSXScreen::watchSystemPowerThread(void*)
//
// main of thread monitoring system power (sleep/wakup) using a CFRunLoop
//

void OSXScreen::watchSystemPowerThread(const void *)
{
  io_object_t notifier;
  IONotificationPortRef notificationPortRef;
  CFRunLoopSourceRef runloopSourceRef = 0;

  m_pmRunloop = CFRunLoopGetCurrent();
  // install system power change callback
  m_pmRootPort = IORegisterForSystemPower(this, &notificationPortRef, powerChangeCallback, &notifier);
  if (m_pmRootPort == 0) {
    LOG_WARN("IORegisterForSystemPower failed");
  } else {
    runloopSourceRef = IONotificationPortGetRunLoopSource(notificationPortRef);
    CFRunLoopAddSource(m_pmRunloop, runloopSourceRef, kCFRunLoopCommonModes);
  }

  // thread is ready
  {
    Lock lock(m_pmMutex);
    *m_pmThreadReady = true;
    m_pmThreadReady->signal();
  }

  // if we were unable to initialize then exit.  we must do this after
  // setting m_pmThreadReady to true otherwise the parent thread will
  // block waiting for it.
  if (m_pmRootPort == 0) {
    LOG_WARN("failed to init watchSystemPowerThread");
    return;
  }

  LOG_DEBUG("started watchSystemPowerThread");

  LOG_DEBUG("waiting for event loop");
  m_events->waitForReady();

  {
    Lock lockCarbon(m_carbonLoopMutex);
    if (*m_carbonLoopReady == false) {

      // we signalling carbon loop ready before starting
      // unless we know how to do it within the loop
      LOG_DEBUG("signalling carbon loop ready");

      *m_carbonLoopReady = true;
      m_carbonLoopReady->signal();
    }
  }

  // start the run loop
  LOG_DEBUG("starting carbon loop");
  CFRunLoopRun();
  LOG_DEBUG("carbon loop has stopped");

  // cleanup
  if (notificationPortRef) {
    CFRunLoopRemoveSource(m_pmRunloop, runloopSourceRef, kCFRunLoopDefaultMode);
    CFRunLoopSourceInvalidate(runloopSourceRef);
    CFRelease(runloopSourceRef);
  }

  Lock lock(m_pmMutex);
  IODeregisterForSystemPower(&notifier);
  m_pmRootPort = 0;
  LOG_DEBUG("stopped watchSystemPowerThread");
}

void OSXScreen::powerChangeCallback(void *refcon, io_service_t service, natural_t messageType, void *messageArg)
{
  ((OSXScreen *)refcon)->handlePowerChangeRequest(messageType, messageArg);
}

void OSXScreen::handlePowerChangeRequest(natural_t messageType, void *messageArg)
{
  // we've received a power change notification
  switch (messageType) {
  case kIOMessageSystemWillSleep:
    // OSXScreen has to handle this in the main thread so we have to
    // queue a confirm sleep event here.  we actually don't allow the
    // system to sleep until the event is handled.
    m_events->addEvent(
        Event(EventTypes::OsxScreenConfirmSleep, getEventTarget(), messageArg, Event::EventFlags::DontFreeData)
    );
    return;

  case kIOMessageSystemHasPoweredOn:
    LOG_DEBUG("system wakeup");
    m_events->addEvent(Event(EventTypes::ScreenResume, getEventTarget()));
    break;

  default:
    break;
  }

  Lock lock(m_pmMutex);
  if (m_pmRootPort != 0) {
    IOAllowPowerChange(m_pmRootPort, (long)messageArg);
  }
}

void OSXScreen::handleConfirmSleep(const Event &event)
{
  long messageArg = (long)event.getData();
  if (messageArg != 0) {
    Lock lock(m_pmMutex);
    if (m_pmRootPort != 0) {
      // deliver suspend event immediately.
      m_events->addEvent(
          Event(EventTypes::ScreenSuspend, getEventTarget(), nullptr, Event::EventFlags::DeliverImmediately)
      );

      LOG_DEBUG("system will sleep");
      IOAllowPowerChange(m_pmRootPort, messageArg);
    }
  }
}

#pragma mark -

//
// GLOBAL HOTKEY OPERATING MODE SUPPORT (10.3)
//
// CoreGraphics private API (OSX 10.3)
// Source: http://ichiro.nnip.org/osx/Cocoa/GlobalHotkey.html
//
// We load the functions dynamically because they're not available in
// older SDKs.  We don't use weak linking because we want users of
// older SDKs to build an app that works on newer systems and older
// SDKs will not provide the symbols.
//
// FIXME: This is hosed as of OS 10.5; patches to repair this are
// a good thing.
//
#if 0

#ifdef __cplusplus
extern "C" {
#endif

typedef int CGSConnection;
typedef enum {
	CGSGlobalHotKeyEnable = 0,
	CGSGlobalHotKeyDisable = 1,
} CGSGlobalHotKeyOperatingMode;

extern CGSConnection _CGSDefaultConnection(void) WEAK_IMPORT_ATTRIBUTE;
extern CGError CGSGetGlobalHotKeyOperatingMode(CGSConnection connection, CGSGlobalHotKeyOperatingMode *mode) WEAK_IMPORT_ATTRIBUTE;
extern CGError CGSSetGlobalHotKeyOperatingMode(CGSConnection connection, CGSGlobalHotKeyOperatingMode mode) WEAK_IMPORT_ATTRIBUTE;

typedef CGSConnection (*_CGSDefaultConnection_t)(void);
typedef CGError (*CGSGetGlobalHotKeyOperatingMode_t)(CGSConnection connection, CGSGlobalHotKeyOperatingMode *mode);
typedef CGError (*CGSSetGlobalHotKeyOperatingMode_t)(CGSConnection connection, CGSGlobalHotKeyOperatingMode mode);

static _CGSDefaultConnection_t				s__CGSDefaultConnection;
static CGSGetGlobalHotKeyOperatingMode_t	s_CGSGetGlobalHotKeyOperatingMode;
static CGSSetGlobalHotKeyOperatingMode_t	s_CGSSetGlobalHotKeyOperatingMode;

#ifdef __cplusplus
}
#endif

#define LOOKUP(name_)                                                                                                  \
  s_##name_ = nullptr;                                                                                                 \
  if (NSIsSymbolNameDefinedWithHint("_" #name_, "CoreGraphics")) {                                                     \
    s_##name_ = (name_##_t)NSAddressOfSymbol(NSLookupAndBindSymbolWithHint("_" #name_, "CoreGraphics"));               \
  }

bool
OSXScreen::isGlobalHotKeyOperatingModeAvailable()
{
	if (!s_testedForGHOM) {
		s_testedForGHOM = true;
		LOOKUP(_CGSDefaultConnection);
		LOOKUP(CGSGetGlobalHotKeyOperatingMode);
		LOOKUP(CGSSetGlobalHotKeyOperatingMode);
		s_hasGHOM = (s__CGSDefaultConnection != nullptr &&
					s_CGSGetGlobalHotKeyOperatingMode != nullptr &&
					s_CGSSetGlobalHotKeyOperatingMode != nullptr);
	}
	return s_hasGHOM;
}

void
OSXScreen::setGlobalHotKeysEnabled(bool enabled)
{
	if (isGlobalHotKeyOperatingModeAvailable()) {
		CGSConnection conn = s__CGSDefaultConnection();

		CGSGlobalHotKeyOperatingMode mode;
		s_CGSGetGlobalHotKeyOperatingMode(conn, &mode);

		if (enabled && mode == CGSGlobalHotKeyDisable) {
			s_CGSSetGlobalHotKeyOperatingMode(conn, CGSGlobalHotKeyEnable);
		}
		else if (!enabled && mode == CGSGlobalHotKeyEnable) {
			s_CGSSetGlobalHotKeyOperatingMode(conn, CGSGlobalHotKeyDisable);
		}
	}
}

bool
OSXScreen::getGlobalHotKeysEnabled()
{
	CGSGlobalHotKeyOperatingMode mode;
	if (isGlobalHotKeyOperatingModeAvailable()) {
		CGSConnection conn = s__CGSDefaultConnection();
		s_CGSGetGlobalHotKeyOperatingMode(conn, &mode);
	}
	else {
		mode = CGSGlobalHotKeyEnable;
	}
	return (mode == CGSGlobalHotKeyEnable);
}

#endif

//
// OSXScreen::HotKeyItem
//

OSXScreen::HotKeyItem::HotKeyItem(uint32_t keycode, uint32_t mask) : m_ref(nullptr), m_keycode(keycode), m_mask(mask)
{
  // do nothing
}

OSXScreen::HotKeyItem::HotKeyItem(EventHotKeyRef ref, uint32_t keycode, uint32_t mask)
    : m_ref(ref),
      m_keycode(keycode),
      m_mask(mask)
{
  // do nothing
}

EventHotKeyRef OSXScreen::HotKeyItem::getRef() const
{
  return m_ref;
}

bool OSXScreen::HotKeyItem::operator<(const HotKeyItem &x) const
{
  return (m_keycode < x.m_keycode || (m_keycode == x.m_keycode && m_mask < x.m_mask));
}

// Quartz event tap support for the secondary display. This makes sure that we
// will show the cursor if a local event comes in while deskflow has the cursor
// off the screen.
CGEventRef
OSXScreen::handleCGInputEventSecondary(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
  // this fix is really screwing with the correct show/hide behavior. it
  // should be tested better before reintroducing.
  return event;

  OSXScreen *screen = (OSXScreen *)refcon;
  if (screen->m_cursorHidden && type == kCGEventMouseMoved) {

    CGPoint pos = CGEventGetLocation(event);
    if (pos.x != screen->m_xCenter || pos.y != screen->m_yCenter) {

      LOG_DEBUG("show cursor on secondary, type=%d pos=%d,%d", type, pos.x, pos.y);
      screen->showCursor();
    }
  }
  return event;
}

// Quartz event tap support
CGEventRef OSXScreen::handleCGInputEvent(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
  OSXScreen *screen = (OSXScreen *)refcon;

  switch (type) {
  case kCGEventLeftMouseDown:
  case kCGEventRightMouseDown:
  case kCGEventOtherMouseDown:
    screen->onMouseButton(true, CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber) + 1);
    break;
  case kCGEventLeftMouseUp:
  case kCGEventRightMouseUp:
  case kCGEventOtherMouseUp:
    screen->onMouseButton(false, CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber) + 1);
    break;
  case kCGEventLeftMouseDragged:
  case kCGEventRightMouseDragged:
  case kCGEventOtherMouseDragged:
  case kCGEventMouseMoved:
    // we intentionally ignore the position in the event here as the events are
    // queued and will no longer be accurate when we process them.
    screen->onMouseMove();

    // The system ignores our cursor-centering calls if
    // we don't return the event. This should be harmless,
    // but might register as slight movement to other apps
    // on the system. It hasn't been a problem before, though.
    return event;
    break;
  case kCGEventScrollWheel:
    screen->onMouseWheel(
        screen->mapScrollWheelToDeskflow(CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis2)),
        screen->mapScrollWheelToDeskflow(CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1))
    );
    break;
  case kCGEventKeyDown:
  case kCGEventKeyUp:
  case kCGEventFlagsChanged:
    screen->onKey(event);
    break;
  case kCGEventTapDisabledByTimeout:
    // Re-enable our event-tap
    CGEventTapEnable(screen->m_eventTapPort, true);
    LOG_INFO("quartz event tap was disabled by timeout, re-enabling");
    break;
  case kCGEventTapDisabledByUserInput:
    LOG_ERR("quartz event tap was disabled by user input");
    break;
  case NX_NULLEVENT:
    break;
  default:
    if (type == NX_SYSDEFINED) {
      if (isMediaKeyEvent(event)) {
        LOG_DEBUG2("detected media key event");
        screen->onMediaKey(event);
      } else {
        LOG_DEBUG2("ignoring unknown system defined event");
        return event;
      }
      break;
    }

    LOG_DEBUG2("unknown quartz event type: 0x%02x", type);
  }

  if (screen->m_isOnScreen) {
    return event;
  } else {
    return nullptr;
  }
}

void OSXScreen::MouseButtonState::set(uint32_t button, EMouseButtonState state)
{
  bool newState = (state == kMouseButtonDown);
  m_buttons.set(button, newState);
}

bool OSXScreen::MouseButtonState::any()
{
  return m_buttons.any();
}

void OSXScreen::MouseButtonState::reset()
{
  m_buttons.reset();
}

void OSXScreen::MouseButtonState::overwrite(uint32_t buttons)
{
  m_buttons = std::bitset<NumButtonIDs>(buttons);
}

bool OSXScreen::MouseButtonState::test(uint32_t button) const
{
  return m_buttons.test(button);
}

int8_t OSXScreen::MouseButtonState::getFirstButtonDown() const
{
  if (m_buttons.any()) {
    for (unsigned short button = 0; button < m_buttons.size(); button++) {
      if (m_buttons.test(button)) {
        return button;
      }
    }
  }
  return -1;
}

char *OSXScreen::CFStringRefToUTF8String(CFStringRef aString)
{
  if (aString == nullptr) {
    return nullptr;
  }

  CFIndex length = CFStringGetLength(aString);
  CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8);
  char *buffer = (char *)malloc(maxSize);

  if (!CFStringGetCString(aString, buffer, maxSize, kCFStringEncodingUTF8)) {
    free(buffer);
    buffer = nullptr;
  }

  return buffer;
}

void OSXScreen::waitForCarbonLoop() const
{
  if (*m_carbonLoopReady) {
    LOG_DEBUG("carbon loop already ready");
    return;
  }

  Lock lock(m_carbonLoopMutex);

  LOG_DEBUG("waiting for carbon loop");

  double timeout = Arch::time() + kCarbonLoopWaitTimeout;
  while (!m_carbonLoopReady->wait()) {
    if (Arch::time() > timeout) {
      LOG_DEBUG("carbon loop not ready, waiting again");
      timeout = Arch::time() + kCarbonLoopWaitTimeout;
    }
  }

  LOG_DEBUG("carbon loop ready");
}

std::string OSXScreen::getSecureInputApp() const
{
  if (IsSecureEventInputEnabled()) {
    int secureInputProcessPID = getSecureInputEventPID();
    if (secureInputProcessPID == 0)
      return "unknown";
    return getProcessName(secureInputProcessPID);
  }
  return "";
}

int getSecureInputEventPID()
{
  io_service_t service = MACH_PORT_NULL, service_root = MACH_PORT_NULL;
  mach_port_t masterPort;

  kern_return_t kr = IOMasterPort(MACH_PORT_NULL, &masterPort);
  if (kr != KERN_SUCCESS)
    return 0;

  // IO registry refuses to tap into the root level directly
  // as a workaround access the parent of the top user level
  service = IORegistryEntryFromPath(masterPort, kIOServicePlane ":/");
  IORegistryEntryGetParentEntry(service, kIOServicePlane, &service_root);

  std::unique_ptr<std::remove_pointer<CFTypeRef>::type, decltype(&CFRelease)> consoleUsers(
      IORegistryEntrySearchCFProperty(
          service_root, kIOServicePlane, CFSTR("IOConsoleUsers"), nullptr,
          kIORegistryIterateParents | kIORegistryIterateRecursively
      ),
      CFRelease
  );
  if (!consoleUsers)
    return 0;

  CFTypeID type = CFGetTypeID(consoleUsers.get());
  if (type != CFArrayGetTypeID())
    return 0;

  CFTypeRef dict = CFArrayGetValueAtIndex((CFArrayRef)consoleUsers.get(), 0);
  if (!dict)
    return 0;

  type = CFGetTypeID(dict);
  if (type != CFDictionaryGetTypeID())
    return 0;

  CFTypeRef secureInputPID = nullptr;
  CFDictionaryGetValueIfPresent((CFDictionaryRef)dict, CFSTR("kCGSSessionSecureInputPID"), &secureInputPID);

  if (secureInputPID == nullptr)
    return 0;

  type = CFGetTypeID(secureInputPID);
  if (type != CFNumberGetTypeID())
    return 0;

  auto pidRef = (CFNumberRef)secureInputPID;
  CFNumberType numberType = CFNumberGetType(pidRef);
  if (numberType != kCFNumberSInt32Type)
    return 0;

  int pid;
  CFNumberGetValue(pidRef, kCFNumberSInt32Type, &pid);
  return pid;
}

std::string getProcessName(int pid)
{
  if (!pid)
    return "";
  char buf[128];
  proc_name(pid, buf, sizeof(buf));
  return buf;
}

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

void setZeroSuppressionInterval()
{
  CGSetLocalEventsSuppressionInterval(0.0);
}

void avoidSupression()
{
  // avoid suppression of local hardware events
  // stkamp@users.sourceforge.net
  CGSetLocalEventsFilterDuringSupressionState(
      kCGEventFilterMaskPermitAllEvents, kCGEventSupressionStateSupressionInterval
  );
  CGSetLocalEventsFilterDuringSupressionState(
      (kCGEventFilterMaskPermitLocalKeyboardEvents | kCGEventFilterMaskPermitSystemDefinedEvents),
      kCGEventSupressionStateRemoteMouseDrag
  );
}

void logCursorVisibility()
{
  // CGCursorIsVisible is probably deprecated because its unreliable.
  if (!CGCursorIsVisible()) {
    LOG_WARN("cursor may not be visible");
  }
}

void avoidHesitatingCursor()
{
  // This used to be necessary to get smooth mouse motion on other screens,
  // but now is just to avoid a hesitating cursor when transitioning to
  // the primary (this) screen.
  CGSetLocalEventsSuppressionInterval(0.0001);
}

#pragma GCC diagnostic error "-Wdeprecated-declarations"
