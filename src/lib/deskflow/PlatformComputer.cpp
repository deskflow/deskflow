/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/PlatformComputer.h"
#include "base/DirectionTypes.h"
#include "deskflow/App.h"

PlatformComputer::PlatformComputer(IEventQueue *events) : IPlatformComputer(events)
{
  // do nothing
}

void PlatformComputer::updateKeyMap()
{
  getKeyState()->updateKeyMap();
}

void PlatformComputer::updateKeyState()
{
  getKeyState()->updateKeyState();
  updateButtons();
}

void PlatformComputer::setHalfDuplexMask(KeyModifierMask mask)
{
  getKeyState()->setHalfDuplexMask(mask);
}

void PlatformComputer::fakeKeyDown(KeyID id, KeyModifierMask mask, KeyButton button, const std::string &lang)
{
  getKeyState()->fakeKeyDown(id, mask, button, lang);
}

bool PlatformComputer::fakeKeyRepeat(
    KeyID id, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &lang
)
{
  return getKeyState()->fakeKeyRepeat(id, mask, count, button, lang);
}

bool PlatformComputer::fakeKeyUp(KeyButton button)
{
  return getKeyState()->fakeKeyUp(button);
}

void PlatformComputer::fakeAllKeysUp()
{
  getKeyState()->fakeAllKeysUp();
}

bool PlatformComputer::fakeCtrlAltDel()
{
  return getKeyState()->fakeCtrlAltDel();
}

bool PlatformComputer::isKeyDown(KeyButton button) const
{
  return getKeyState()->isKeyDown(button);
}

KeyModifierMask PlatformComputer::getActiveModifiers() const
{
  return getKeyState()->getActiveModifiers();
}

KeyModifierMask PlatformComputer::pollActiveModifiers() const
{
  return getKeyState()->pollActiveModifiers();
}

int32_t PlatformComputer::pollActiveGroup() const
{
  return getKeyState()->pollActiveGroup();
}

void PlatformComputer::pollPressedKeys(KeyButtonSet &pressedKeys) const
{
  getKeyState()->pollPressedKeys(pressedKeys);
}

void PlatformComputer::clearStaleModifiers()
{
  getKeyState()->clearStaleModifiers();
}

std::string PlatformComputer::sidesMaskToString(uint32_t sides)
{
  using enum DirectionMask;
  std::string sidesText;
  if ((sides & static_cast<int>(LeftMask)) != 0) {
    sidesText += "L";
  }
  if ((sides & static_cast<int>(RightMask)) != 0) {
    sidesText += "R";
  }
  if ((sides & static_cast<int>(TopMask)) != 0) {
    sidesText += "T";
  }
  if ((sides & static_cast<int>(BottomMask)) != 0) {
    sidesText += "B";
  }
  return sidesText;
}
