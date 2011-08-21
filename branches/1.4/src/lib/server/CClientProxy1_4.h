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
#include "GameDeviceTypes.h"

class CServer;

//! Proxy for client implementing protocol version 1.4
class CClientProxy1_4 : public CClientProxy1_3 {
public:
	CClientProxy1_4(const CString& name, IStream* adoptedStream, CServer* server);
	~CClientProxy1_4();

	// IClient overrides
	virtual void		gameDeviceButtons(GameDeviceID id, GameDeviceButton buttons);
	virtual void		gameDeviceSticks(GameDeviceID id, SInt16 x1, SInt16 y1, SInt16 x2, SInt16 y2);
	virtual void		gameDeviceTriggers(GameDeviceID id, UInt8 t1, UInt8 t2);
	virtual void		gameDeviceTimingReq();

protected:
	// CClientProxy overrides
	virtual bool		parseMessage(const UInt8* code);

private:
	// message handlers
	void				gameDeviceTimingResp();
	void				gameDeviceFeedback();

	CServer*			m_server;
};
