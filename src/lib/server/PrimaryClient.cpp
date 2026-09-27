/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/PrimaryClient.h"

#include "base/Log.h"
#include "deskflow/Computer.h"
//
// PrimaryClient
//

PrimaryClient::PrimaryClient(const std::string &name, deskflow::Computer *computer)
    : BaseClientProxy(name),
      m_computer(computer)
{
  // do nothing
}

void PrimaryClient::reconfigure(uint32_t activeSides)
{
  m_computer->reconfigure(activeSides);
}

uint32_t PrimaryClient::registerHotKey(KeyID key, KeyModifierMask mask)
{
  return m_computer->registerHotKey(key, mask);
}

void PrimaryClient::unregisterHotKey(uint32_t id)
{
  m_computer->unregisterHotKey(id);
}

void PrimaryClient::fakeInputBegin()
{
  if (++m_fakeInputCount == 1) {
    m_computer->fakeInputBegin();
  }
}

void PrimaryClient::fakeInputEnd()
{
  if (--m_fakeInputCount == 0) {
    m_computer->fakeInputEnd();
  }
}

int32_t PrimaryClient::getJumpZoneSize() const
{
  return m_computer->getJumpZoneSize();
}

void PrimaryClient::getCursorCenter(int32_t &x, int32_t &y) const
{
  m_computer->getCursorCenter(x, y);
}

KeyModifierMask PrimaryClient::getToggleMask() const
{
  return m_computer->pollActiveModifiers();
}

bool PrimaryClient::isLockedToComputer() const
{
  return m_computer->isLockedToComputer();
}

void *PrimaryClient::getEventTarget() const
{
  return m_computer->getEventTarget();
}

bool PrimaryClient::getClipboard(ClipboardID id, IClipboard *clipboard) const
{
  return m_computer->getClipboard(id, clipboard);
}

void PrimaryClient::getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const
{
  m_computer->getShape(x, y, width, height);
}

void PrimaryClient::getCursorPos(int32_t &x, int32_t &y) const
{
  m_computer->getCursorPos(x, y);
}

void PrimaryClient::enable()
{
  m_computer->enable();
}

void PrimaryClient::disable()
{
  m_computer->disable();
}

void PrimaryClient::enter(int32_t xAbs, int32_t yAbs, uint32_t seqNum, KeyModifierMask mask, bool screensaver)
{
  m_computer->setSequenceNumber(seqNum);
  if (!screensaver) {
    m_computer->warpCursor(xAbs, yAbs);
  }
  m_computer->enter(mask);
}

bool PrimaryClient::leave()
{
  return m_computer->leave();
}

void PrimaryClient::setClipboard(ClipboardID id, const IClipboard *clipboard)
{
  // ignore if this clipboard is already clean
  if (m_clipboardDirty[id]) {
    // this clipboard is now clean
    m_clipboardDirty[id] = false;

    // set clipboard
    m_computer->setClipboard(id, clipboard);
  }
}

void PrimaryClient::grabClipboard(ClipboardID id)
{
  // grab clipboard
  m_computer->grabClipboard(id);

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
    // XXX -- don't forward keystrokes to primary computer for now
    (void)key;
    (void)mask;
    (void)button;
    //        m_computer->keyDown(key, mask, button);
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
    //        m_computer->keyUp(key, mask, button);
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
  m_computer->warpCursor(x, y);
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
  return m_computer->getSecureInputApp();
}

void PrimaryClient::secureInputNotification(const std::string &app) const
{
  LOG_INFO("application \"%s\" is blocking the keyboard", app.c_str());
}

void PrimaryClient::resetOptions()
{
  m_computer->resetOptions();
}

void PrimaryClient::setOptions(const OptionsList &options)
{
  m_computer->setOptions(options);
}
