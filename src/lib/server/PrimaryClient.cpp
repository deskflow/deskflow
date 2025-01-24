/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/PrimaryClient.h"

#include "base/Log.h"
#include "deskflow/AppUtil.h"
#include "deskflow/Clipboard.h"
#include "deskflow/Screen.h"
//
// PrimaryClient
//

PrimaryClient::PrimaryClient(const std::string &name, deskflow::Screen *screen)
    : BaseClientProxy(name),
      m_screen(screen),
      m_fakeInputCount(0)
{
  // all clipboards are clean
  for (uint32_t i = 0; i < kClipboardEnd; ++i) {
    m_clipboardDirty[i] = false;
  }
}

PrimaryClient::~PrimaryClient()
{
  // do nothing
}

void PrimaryClient::reconfigure(uint32_t activeSides)
{
  m_screen->reconfigure(activeSides);
}

uint32_t PrimaryClient::registerHotKey(KeyID key, KeyModifierMask mask)
{
  return m_screen->registerHotKey(key, mask);
}

void PrimaryClient::unregisterHotKey(uint32_t id)
{
  m_screen->unregisterHotKey(id);
}

void PrimaryClient::fakeInputBegin()
{
  if (++m_fakeInputCount == 1) {
    m_screen->fakeInputBegin();
  }
}

void PrimaryClient::fakeInputEnd()
{
  if (--m_fakeInputCount == 0) {
    m_screen->fakeInputEnd();
  }
}

int32_t PrimaryClient::getJumpZoneSize() const
{
  return m_screen->getJumpZoneSize();
}

void PrimaryClient::getCursorCenter(int32_t &x, int32_t &y) const
{
  m_screen->getCursorCenter(x, y);
}

KeyModifierMask PrimaryClient::getToggleMask() const
{
  return m_screen->pollActiveModifiers();
}

bool PrimaryClient::isLockedToScreen() const
{
  return m_screen->isLockedToScreen();
}

void *PrimaryClient::getEventTarget() const
{
  return m_screen->getEventTarget();
}

bool PrimaryClient::getClipboard(ClipboardID id, IClipboard *clipboard) const
{
  return m_screen->getClipboard(id, clipboard);
}

void PrimaryClient::getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const
{
  m_screen->getShape(x, y, width, height);
}

void PrimaryClient::getCursorPos(int32_t &x, int32_t &y) const
{
  m_screen->getCursorPos(x, y);
}

void PrimaryClient::enable()
{
  m_screen->enable();
}

void PrimaryClient::disable()
{
  m_screen->disable();
}

void PrimaryClient::enter(int32_t xAbs, int32_t yAbs, uint32_t seqNum, KeyModifierMask mask, bool screensaver)
{
  m_screen->setSequenceNumber(seqNum);
  if (!screensaver) {
    m_screen->warpCursor(xAbs, yAbs);
  }
  m_screen->enter(mask);
}

bool PrimaryClient::leave()
{
  return m_screen->leave();
}

void PrimaryClient::setClipboard(ClipboardID id, const IClipboard *clipboard)
{
  // ignore if this clipboard is already clean
  if (m_clipboardDirty[id]) {
    // this clipboard is now clean
    m_clipboardDirty[id] = false;

    // set clipboard
    m_screen->setClipboard(id, clipboard);
  }
}

void PrimaryClient::grabClipboard(ClipboardID id)
{
  // grab clipboard
  m_screen->grabClipboard(id);

  // clipboard is dirty (because someone else owns it now)
  m_clipboardDirty[id] = true;
}

void PrimaryClient::setClipboardDirty(ClipboardID id, bool dirty)
{
  m_clipboardDirty[id] = dirty;
}

void PrimaryClient::keyDown(KeyID key, KeyModifierMask mask, KeyButton button, const std::string &)
{
  if (m_fakeInputCount > 0) {
    // XXX -- don't forward keystrokes to primary screen for now
    (void)key;
    (void)mask;
    (void)button;
    //        m_screen->keyDown(key, mask, button);
  }
}

void PrimaryClient::keyRepeat(KeyID, KeyModifierMask, int32_t, KeyButton, const std::string &)
{
  // ignore
}

void PrimaryClient::keyUp(KeyID key, KeyModifierMask mask, KeyButton button)
{
  if (m_fakeInputCount > 0) {
    // XXX -- don't forward keystrokes to primary screen for now
    (void)key;
    (void)mask;
    (void)button;
    //        m_screen->keyUp(key, mask, button);
  }
}

void PrimaryClient::mouseDown(ButtonID)
{
  // ignore
}

void PrimaryClient::mouseUp(ButtonID)
{
  // ignore
}

void PrimaryClient::mouseMove(int32_t x, int32_t y)
{
  m_screen->warpCursor(x, y);
}

void PrimaryClient::mouseRelativeMove(int32_t, int32_t)
{
  // ignore
}

void PrimaryClient::mouseWheel(int32_t, int32_t)
{
  // ignore
}

void PrimaryClient::screensaver(bool)
{
  // ignore
}

void PrimaryClient::sendDragInfo(uint32_t fileCount, const char *info, size_t size)
{
  // ignore
}

void PrimaryClient::fileChunkSending(uint8_t mark, char *data, size_t dataSize)
{
  // ignore
}

std::string PrimaryClient::getSecureInputApp() const
{
  return m_screen->getSecureInputApp();
}

void PrimaryClient::secureInputNotification(const std::string &app) const
{
  if (app != "unknown") {
    AppUtil::instance().showNotification(
        "The client keyboards may stop working.", "'Secure input' enabled by " + app +
                                                      ". "
                                                      "Close " +
                                                      app + " to continue using keyboards on the clients."
    );
  } else {
    AppUtil::instance().showNotification(
        "The client keyboards may stop working.", "'Secure input' enabled by an application. "
                                                  "Close the application to continue using keyboards on the clients."
    );
  }
}

void PrimaryClient::resetOptions()
{
  m_screen->resetOptions();
}

void PrimaryClient::setOptions(const OptionsList &options)
{
  m_screen->setOptions(options);
}
