/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2011 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "CClientProxy1_3.h"
#include "GamepadTypes.h"

//! Proxy for client implementing protocol version 1.4
class CClientProxy1_4 : public CClientProxy1_3 {
public:
	CClientProxy1_4(const CString& name, IStream* adoptedStream);
	~CClientProxy1_4();

	// IClient overrides
	virtual void		gamepadButtonDown(GamepadButtonID id);
	virtual void		gamepadButtonUp(GamepadButtonID id);
	virtual void		gamepadAnalog(GamepadAnalogID id, SInt16 x, SInt16 y);
};
