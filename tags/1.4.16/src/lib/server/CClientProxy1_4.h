/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2011 Chris Schoeneman
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

class CServer;

//! Proxy for client implementing protocol version 1.4
class CClientProxy1_4 : public CClientProxy1_3 {
public:
	CClientProxy1_4(const CString& name, synergy::IStream* adoptedStream, CServer* server, IEventQueue* events);
	~CClientProxy1_4();

	//! @name accessors
	//@{

	//! get server pointer
	CServer*			getServer() { return m_server; }

	//@}

	// IClient overrides
	virtual void		keyDown(KeyID key, KeyModifierMask mask, KeyButton button);
	virtual void		keyRepeat(KeyID key, KeyModifierMask mask, SInt32 count, KeyButton button);
	virtual void		keyUp(KeyID key, KeyModifierMask mask, KeyButton button);
	virtual void		keepAlive();

	//! Send IV to make 
	void				cryptoIv();

	CServer*			m_server;
};
