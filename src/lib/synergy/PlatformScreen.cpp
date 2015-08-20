/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2004 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library.
 * You must obey the GNU General Public License in all respects for all of
 * the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete
 * this exception statement from your version. If you delete this exception
 * statement from all source files in the program, then also delete it here.
 */

#include "synergy/PlatformScreen.h"
#include "synergy/App.h"
#include "synergy/ArgsBase.h"

PlatformScreen::PlatformScreen(IEventQueue* events) :
	IPlatformScreen(events),
	m_draggingStarted(false),
	m_fakeDraggingStarted(false)
{
}

PlatformScreen::~PlatformScreen()
{
	// do nothing
}

void
PlatformScreen::updateKeyMap()
{
	getKeyState()->updateKeyMap();
}

void
PlatformScreen::updateKeyState()
{
	getKeyState()->updateKeyState();
	updateButtons();
}

void
PlatformScreen::setHalfDuplexMask(KeyModifierMask mask)
{
	getKeyState()->setHalfDuplexMask(mask);
}

void
PlatformScreen::fakeKeyDown(KeyID id, KeyModifierMask mask,
				KeyButton button)
{
	getKeyState()->fakeKeyDown(id, mask, button);
}

bool
PlatformScreen::fakeKeyRepeat(KeyID id, KeyModifierMask mask,
				SInt32 count, KeyButton button)
{
	return getKeyState()->fakeKeyRepeat(id, mask, count, button);
}

bool
PlatformScreen::fakeKeyUp(KeyButton button)
{
	return getKeyState()->fakeKeyUp(button);
}

void
PlatformScreen::fakeAllKeysUp()
{
	getKeyState()->fakeAllKeysUp();
}

bool
PlatformScreen::fakeCtrlAltDel()
{
	return getKeyState()->fakeCtrlAltDel();
}

bool
PlatformScreen::isKeyDown(KeyButton button) const
{
	return getKeyState()->isKeyDown(button);
}

KeyModifierMask
PlatformScreen::getActiveModifiers() const
{
	return getKeyState()->getActiveModifiers();
}

KeyModifierMask
PlatformScreen::pollActiveModifiers() const
{
	return getKeyState()->pollActiveModifiers();
}

SInt32
PlatformScreen::pollActiveGroup() const
{
	return getKeyState()->pollActiveGroup();
}

void
PlatformScreen::pollPressedKeys(KeyButtonSet& pressedKeys) const
{
	getKeyState()->pollPressedKeys(pressedKeys);
}

bool
PlatformScreen::isDraggingStarted()
{
	if (App::instance().argsBase().m_enableDragDrop) {
		return m_draggingStarted;
	}
	return false;
}
