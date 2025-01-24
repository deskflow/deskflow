/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/PlatformScreen.h"
#include "deskflow/App.h"
#include "deskflow/ArgsBase.h"

PlatformScreen::PlatformScreen(IEventQueue *events, deskflow::ClientScrollDirection scrollDirection)
    : IPlatformScreen(events),
      m_draggingStarted(false),
      m_fakeDraggingStarted(false),
      m_clientScrollDirection(scrollDirection)
{
}

PlatformScreen::~PlatformScreen()
{
  // do nothing
}

void PlatformScreen::updateKeyMap()
{
  getKeyState()->updateKeyMap();
}

void PlatformScreen::updateKeyState()
{
  getKeyState()->updateKeyState();
  updateButtons();
}

void PlatformScreen::setHalfDuplexMask(KeyModifierMask mask)
{
  getKeyState()->setHalfDuplexMask(mask);
}

void PlatformScreen::fakeKeyDown(KeyID id, KeyModifierMask mask, KeyButton button, const std::string &lang)
{
  getKeyState()->fakeKeyDown(id, mask, button, lang);
}

bool PlatformScreen::fakeKeyRepeat(
    KeyID id, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &lang
)
{
  return getKeyState()->fakeKeyRepeat(id, mask, count, button, lang);
}

bool PlatformScreen::fakeKeyUp(KeyButton button)
{
  return getKeyState()->fakeKeyUp(button);
}

void PlatformScreen::fakeAllKeysUp()
{
  getKeyState()->fakeAllKeysUp();
}

bool PlatformScreen::fakeCtrlAltDel()
{
  return getKeyState()->fakeCtrlAltDel();
}

bool PlatformScreen::isKeyDown(KeyButton button) const
{
  return getKeyState()->isKeyDown(button);
}

KeyModifierMask PlatformScreen::getActiveModifiers() const
{
  return getKeyState()->getActiveModifiers();
}

KeyModifierMask PlatformScreen::pollActiveModifiers() const
{
  return getKeyState()->pollActiveModifiers();
}

int32_t PlatformScreen::pollActiveGroup() const
{
  return getKeyState()->pollActiveGroup();
}

void PlatformScreen::pollPressedKeys(KeyButtonSet &pressedKeys) const
{
  getKeyState()->pollPressedKeys(pressedKeys);
}

bool PlatformScreen::isDraggingStarted()
{
  if (App::instance().argsBase().m_enableDragDrop) {
    return m_draggingStarted;
  }
  return false;
}

int32_t PlatformScreen::mapClientScrollDirection(int32_t x) const
{
  return (x * m_clientScrollDirection);
}
