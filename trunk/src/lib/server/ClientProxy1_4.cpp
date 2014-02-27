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

#include "CClientProxy1_4.h"
#include "CProtocolUtil.h"
#include "CLog.h"
#include "IEventQueue.h"
#include "TMethodEventJob.h"
#include <cstring>
#include <memory>
#include "CServer.h"
#include "CCryptoStream.h"

//
// CClientProxy1_4
//

CClientProxy1_4::CClientProxy1_4(const CString& name, synergy::IStream* stream, CServer* server, IEventQueue* events) :
	CClientProxy1_3(name, stream, events), m_server(server)
{
	assert(m_server != NULL);
}

CClientProxy1_4::~CClientProxy1_4()
{
}

void
CClientProxy1_4::keyDown(KeyID key, KeyModifierMask mask, KeyButton button)
{
	cryptoIv();
	CClientProxy1_3::keyDown(key, mask, button);
}

void
CClientProxy1_4::keyRepeat(KeyID key, KeyModifierMask mask, SInt32 count, KeyButton button)
{
	cryptoIv();
	CClientProxy1_3::keyRepeat(key, mask, count, button);
}

void
CClientProxy1_4::keyUp(KeyID key, KeyModifierMask mask, KeyButton button)
{
	cryptoIv();
	CClientProxy1_3::keyUp(key, mask, button);
}

void
CClientProxy1_4::keepAlive()
{
	cryptoIv();
	CClientProxy1_3::keepAlive();
}

void
CClientProxy1_4::cryptoIv()
{
	CCryptoStream* cryptoStream = dynamic_cast<CCryptoStream*>(getStream());
	if (cryptoStream == NULL) {
		return;
	}

	byte iv[CRYPTO_IV_SIZE];
	cryptoStream->newIv(iv);
	CString data(reinterpret_cast<const char*>(iv), CRYPTO_IV_SIZE);

	LOG((CLOG_DEBUG2 "send crypto iv change to \"%s\"", getName().c_str()));
	CProtocolUtil::writef(getStream(), kMsgDCryptoIv, &data);
	
	// change IV only after we've sent the current IV, otherwise
	// the client won't be able to decrypt the new IV.
	cryptoStream->setEncryptIv(iv);
}
