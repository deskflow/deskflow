/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "CPlatformScreen.h"

CPlatformScreen::CPlatformScreen()
{
	// do nothing
}

CPlatformScreen::~CPlatformScreen()
{
	// do nothing
}

void
CPlatformScreen::updateKeyMap(UInt8 id)
{
	getKeyState(id)->updateKeyMap(id);
}

void
CPlatformScreen::updateKeyState(UInt8 id)
{
	getKeyState(id)->updateKeyState(id);
	updateButtons();
}

void
CPlatformScreen::setHalfDuplexMask(KeyModifierMask mask, UInt8 id)
{
	getKeyState(id)->setHalfDuplexMask(mask, id);
}

void
CPlatformScreen::fakeKeyDown(KeyID kId, KeyModifierMask mask,
				KeyButton button, UInt8 id)
{
	getKeyState(id)->fakeKeyDown(kId, mask, button, id);
}

void
CPlatformScreen::fakeKeyRepeat(KeyID kId, KeyModifierMask mask,
				SInt32 count, KeyButton button, UInt8 id)
{
	getKeyState(id)->fakeKeyRepeat(kId, mask, count, button, id);
}

void
CPlatformScreen::fakeKeyUp(KeyButton button, UInt8 id)
{
	getKeyState(id)->fakeKeyUp(button, id);
}

void
CPlatformScreen::fakeAllKeysUp(UInt8 id)
{  
	getKeyState(id)->fakeAllKeysUp(id);
}

bool
CPlatformScreen::fakeCtrlAltDel(UInt8 id)
{
	return getKeyState(id)->fakeCtrlAltDel(id);
}

bool
CPlatformScreen::isKeyDown(KeyButton button, UInt8 id) const
{
	return getKeyState(id)->isKeyDown(button, id);
}

KeyModifierMask
CPlatformScreen::getActiveModifiers(UInt8 id) const
{
	return getKeyState(id)->getActiveModifiers(id);
}

KeyModifierMask
CPlatformScreen::pollActiveModifiers(UInt8 id) const
{
	return getKeyState(id)->pollActiveModifiers(id);
}

SInt32
CPlatformScreen::pollActiveGroup(UInt8 id) const
{
	return getKeyState(id)->pollActiveGroup(id);
}

void
CPlatformScreen::pollPressedKeys(KeyButtonSet& pressedKeys, UInt8 id) const
{
	getKeyState(id)->pollPressedKeys(pressedKeys, id);
}
