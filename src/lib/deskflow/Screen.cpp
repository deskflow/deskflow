/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2003 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/Screen.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "base/TMethodEventJob.h"
#include "deskflow/IPlatformScreen.h"
#include "deskflow/protocol_types.h"
#include "server/ClientProxy.h"

namespace deskflow {

//
// Screen
//

Screen::Screen(IPlatformScreen *platformScreen, IEventQueue *events)
    : m_screen(platformScreen),
      m_isPrimary(platformScreen->isPrimary()),
      m_enabled(false),
      m_entered(m_isPrimary),
      m_fakeInput(false),
      m_events(events),
      m_mock(false),
      m_enableDragDrop(false)
{
  assert(m_screen != NULL);

  // reset options
  resetOptions();

  LOG((CLOG_DEBUG "opened display"));
}

Screen::~Screen()
{
  if (m_mock) {
    return;
  }

  if (m_enabled) {
    disable();
  }
  assert(!m_enabled);

  // Originally there was an assert here added before 2009 (history not in
  // tact). This condition seems to occur on a Windows client when the process
  // is shut down to make way for a new elevated process (e.g. at login screen).
  // The reason why this assert was originally added is unclear, and was causing
  // pain when using debug builds; you lose control of the client when it's at
  // the login screen. Therefore it has been converted to a warning so that we
  // can still see when it happens but it won't cause the process to pause. This
  // also gives us the added benefit of seeing when it happens in production.
  // Perhaps it indicates that the cursor is still being controlled on the
  // client while it's shutting down? i.e. the screen is entered and is not the
  // server, or the screen is not entered and is the server.
  if (m_entered == m_isPrimary) {
    LOG(
        (CLOG_DEBUG "current screen: entered=%s, primary=%s", //
         m_entered ? "yes" : "no", m_isPrimary ? "yes" : "no")
    );
    if (m_isPrimary) {
      LOG((CLOG_WARN "current primary screen is not entered on shutdown"));
    } else {
      LOG((CLOG_WARN "current secondary screen is entered on shutdown"));
    }
  }

  delete m_screen;
  LOG((CLOG_DEBUG "closed display"));
}

void Screen::enable()
{
  assert(!m_enabled);

  m_screen->updateKeyMap();
  m_screen->updateKeyState();
  m_screen->enable();
  if (m_isPrimary) {
    enablePrimary();
  } else {
    enableSecondary();
  }

  // note activation
  m_enabled = true;
}

void Screen::disable()
{
  assert(m_enabled);

  if (!m_isPrimary && m_entered) {
    leave();
  } else if (m_isPrimary && !m_entered) {
    enter(0);
  }
  m_screen->disable();
  if (m_isPrimary) {
    disablePrimary();
  } else {
    disableSecondary();
  }

  // note deactivation
  m_enabled = false;
}

void Screen::enter(KeyModifierMask toggleMask)
{
  LOG((CLOG_INFO "entering screen"));

  if (m_entered) {
    LOG_WARN("screen already entered");
  }

  // now on screen
  m_entered = true;

  m_screen->enter();
  if (m_isPrimary) {
    enterPrimary();
  } else {
    enterSecondary(toggleMask);
  }
}

bool Screen::leave()
{
  LOG((CLOG_INFO "leaving screen"));

  if (!m_entered) {
    LOG_WARN("screen already left");
  }

  if (!m_screen->canLeave()) {
    return false;
  }

  if (m_isPrimary) {
    leavePrimary();
  } else {
    leaveSecondary();
  }

  m_screen->leave();

  // make sure our idea of clipboard ownership is correct
  m_screen->checkClipboards();

  // now not on screen
  m_entered = false;

  return true;
}

void Screen::reconfigure(uint32_t activeSides)
{
  assert(m_isPrimary);
  m_screen->reconfigure(activeSides);
}

void Screen::warpCursor(int32_t x, int32_t y)
{
  assert(m_isPrimary);
  m_screen->warpCursor(x, y);
}

void Screen::setClipboard(ClipboardID id, const IClipboard *clipboard)
{
  m_screen->setClipboard(id, clipboard);
}

void Screen::grabClipboard(ClipboardID id)
{
  m_screen->setClipboard(id, NULL);
}

void Screen::screensaver(bool) const
{
  // do nothing
}

void Screen::keyDown(KeyID id, KeyModifierMask mask, KeyButton button, const std::string &lang)
{
  // check for ctrl+alt+del emulation
  if (id == kKeyDelete && (mask & (KeyModifierControl | KeyModifierAlt)) == (KeyModifierControl | KeyModifierAlt)) {
    LOG((CLOG_DEBUG "emulating ctrl+alt+del press"));
    if (m_screen->fakeCtrlAltDel()) {
      return;
    }
  }
  m_screen->fakeKeyDown(id, mask, button, lang);
}

void Screen::keyRepeat(KeyID id, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &lang)
{
  assert(!m_isPrimary);
  m_screen->fakeKeyRepeat(id, mask, count, button, lang);
}

void Screen::keyUp(KeyID, KeyModifierMask, KeyButton button)
{
  m_screen->fakeKeyUp(button);
}

void Screen::mouseDown(ButtonID button)
{
  m_screen->fakeMouseButton(button, true);
}

void Screen::mouseUp(ButtonID button)
{
  m_screen->fakeMouseButton(button, false);
}

void Screen::mouseMove(int32_t x, int32_t y)
{
  assert(!m_isPrimary);
  m_screen->fakeMouseMove(x, y);
}

void Screen::mouseRelativeMove(int32_t dx, int32_t dy)
{
  assert(!m_isPrimary);
  m_screen->fakeMouseRelativeMove(dx, dy);
}

void Screen::mouseWheel(int32_t xDelta, int32_t yDelta) const
{
  assert(!m_isPrimary);
  m_screen->fakeMouseWheel(xDelta, yDelta);
}

void Screen::resetOptions()
{
  // reset options
  m_halfDuplex = 0;

  // let screen handle its own options
  m_screen->resetOptions();
}

void Screen::setOptions(const OptionsList &options)
{
  // update options
  for (uint32_t i = 0, n = (uint32_t)options.size(); i < n; i += 2) {
    if (options[i] == kOptionHalfDuplexCapsLock) {
      if (options[i + 1] != 0) {
        m_halfDuplex |= KeyModifierCapsLock;
      } else {
        m_halfDuplex &= ~KeyModifierCapsLock;
      }
      LOG((CLOG_DEBUG1 "half-duplex caps-lock %s", ((m_halfDuplex & KeyModifierCapsLock) != 0) ? "on" : "off"));
    } else if (options[i] == kOptionHalfDuplexNumLock) {
      if (options[i + 1] != 0) {
        m_halfDuplex |= KeyModifierNumLock;
      } else {
        m_halfDuplex &= ~KeyModifierNumLock;
      }
      LOG((CLOG_DEBUG1 "half-duplex num-lock %s", ((m_halfDuplex & KeyModifierNumLock) != 0) ? "on" : "off"));
    } else if (options[i] == kOptionHalfDuplexScrollLock) {
      if (options[i + 1] != 0) {
        m_halfDuplex |= KeyModifierScrollLock;
      } else {
        m_halfDuplex &= ~KeyModifierScrollLock;
      }
      LOG((CLOG_DEBUG1 "half-duplex scroll-lock %s", ((m_halfDuplex & KeyModifierScrollLock) != 0) ? "on" : "off"));
    }
  }

  // update half-duplex options
  m_screen->setHalfDuplexMask(m_halfDuplex);

  // let screen handle its own options
  m_screen->setOptions(options);
}

void Screen::setSequenceNumber(uint32_t seqNum)
{
  m_screen->setSequenceNumber(seqNum);
}

uint32_t Screen::registerHotKey(KeyID key, KeyModifierMask mask)
{
  return m_screen->registerHotKey(key, mask);
}

void Screen::unregisterHotKey(uint32_t id)
{
  m_screen->unregisterHotKey(id);
}

void Screen::fakeInputBegin()
{
  assert(!m_fakeInput);

  m_fakeInput = true;
  m_screen->fakeInputBegin();
}

void Screen::fakeInputEnd()
{
  assert(m_fakeInput);

  m_fakeInput = false;
  m_screen->fakeInputEnd();
}

bool Screen::isOnScreen() const
{
  return m_entered;
}

bool Screen::isLockedToScreen() const
{
  // check for pressed mouse buttons
  // HACK: commented out as it breaks new drag drop feature
  uint32_t buttonID = 0;

  if (m_screen->isAnyMouseButtonDown(buttonID)) {
    if (buttonID != kButtonLeft) {
      LOG((CLOG_DEBUG "locked by mouse buttonID: %d", buttonID));
    }

    if (m_enableDragDrop) {
      return (buttonID == kButtonLeft) ? false : true;
    } else {
      return true;
    }
  }

  // not locked
  return false;
}

int32_t Screen::getJumpZoneSize() const
{
  if (!m_isPrimary) {
    return 0;
  } else {
    return m_screen->getJumpZoneSize();
  }
}

void Screen::getCursorCenter(int32_t &x, int32_t &y) const
{
  m_screen->getCursorCenter(x, y);
}

KeyModifierMask Screen::getActiveModifiers() const
{
  return m_screen->getActiveModifiers();
}

KeyModifierMask Screen::pollActiveModifiers() const
{
  return m_screen->pollActiveModifiers();
}

bool Screen::isDraggingStarted() const
{
  return m_screen->isDraggingStarted();
}

bool Screen::isFakeDraggingStarted() const
{
  return m_screen->isFakeDraggingStarted();
}

void Screen::setDraggingStarted(bool started)
{
  m_screen->setDraggingStarted(started);
}

void Screen::startDraggingFiles(DragFileList &fileList)
{
  m_screen->fakeDraggingFiles(fileList);
}

void Screen::setEnableDragDrop(bool enabled)
{
  m_enableDragDrop = enabled;
}

std::string &Screen::getDraggingFilename() const
{
  return m_screen->getDraggingFilename();
}

void Screen::clearDraggingFilename()
{
  m_screen->clearDraggingFilename();
}

const std::string &Screen::getDropTarget() const
{
  return m_screen->getDropTarget();
}

void *Screen::getEventTarget() const
{
  return m_screen;
}

bool Screen::getClipboard(ClipboardID id, IClipboard *clipboard) const
{
  return m_screen->getClipboard(id, clipboard);
}

void Screen::getShape(int32_t &x, int32_t &y, int32_t &w, int32_t &h) const
{
  m_screen->getShape(x, y, w, h);
}

void Screen::getCursorPos(int32_t &x, int32_t &y) const
{
  m_screen->getCursorPos(x, y);
}

void Screen::enablePrimary()
{
  // get notified of screen saver activation/deactivation
  m_screen->openScreensaver(true);

  // claim screen changed size
  m_events->addEvent(Event(m_events->forIScreen().shapeChanged(), getEventTarget()));
}

void Screen::enableSecondary()
{
  // assume primary has all clipboards
  for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
    grabClipboard(id);
  }
}

void Screen::disablePrimary()
{
  // done with screen saver
  m_screen->closeScreensaver();
}

void Screen::disableSecondary()
{
  // done with screen saver
  m_screen->closeScreensaver();
}

void Screen::enterPrimary()
{
  // do nothing
}

void Screen::enterSecondary(KeyModifierMask)
{
  // do nothing
}

void Screen::leavePrimary()
{
  // we don't track keys while on the primary screen so update our
  // idea of them now.  this is particularly to update the state of
  // the toggle modifiers.
  m_screen->updateKeyState();
}

void Screen::leaveSecondary()
{
  // release any keys we think are still down
  m_screen->fakeAllKeysUp();
}

std::string Screen::getSecureInputApp() const
{
  return m_screen->getSecureInputApp();
}

} // namespace deskflow
