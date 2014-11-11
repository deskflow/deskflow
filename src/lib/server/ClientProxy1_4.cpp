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

#include "server/ClientProxy1_4.h"

#include "server/Server.h"
#include "synergy/ProtocolUtil.h"
#include "io/CryptoStream.h"
#include "base/Log.h"
#include "base/IEventQueue.h"
#include "base/TMethodEventJob.h"

#include <cstring>
#include <memory>

//
// ClientProxy1_4
//

ClientProxy1_4::ClientProxy1_4(const String& name, synergy::IStream* stream, Server* server, IEventQueue* events) :
	ClientProxy1_3(name, stream, events), m_server(server)
{
	assert(m_server != NULL);
}

ClientProxy1_4::~ClientProxy1_4()
{
}

void
ClientProxy1_4::keyDown(KeyID key, KeyModifierMask mask, KeyButton button)
{
	cryptoIv();
	ClientProxy1_3::keyDown(key, mask, button);
}

void
ClientProxy1_4::keyRepeat(KeyID key, KeyModifierMask mask, SInt32 count, KeyButton button)
{
	cryptoIv();
	ClientProxy1_3::keyRepeat(key, mask, count, button);
}

void
ClientProxy1_4::keyUp(KeyID key, KeyModifierMask mask, KeyButton button)
{
	cryptoIv();
	ClientProxy1_3::keyUp(key, mask, button);
}

void
ClientProxy1_4::keepAlive()
{
	cryptoIv();
	ClientProxy1_3::keepAlive();
}

void
ClientProxy1_4::cryptoIv()
{
	CryptoStream* cryptoStream = dynamic_cast<CryptoStream*>(getStream());
	if (cryptoStream == NULL) {
		return;
	}

	byte iv[CRYPTO_IV_SIZE];
	cryptoStream->newIv(iv);
	String data(reinterpret_cast<const char*>(iv), CRYPTO_IV_SIZE);

	LOG((CLOG_DEBUG2 "send crypto iv change to \"%s\"", getName().c_str()));
	ProtocolUtil::writef(getStream(), kMsgDCryptoIv, &data);
	
	// change IV only after we've sent the current IV, otherwise
	// the client won't be able to decrypt the new IV.
	cryptoStream->setEncryptIv(iv);
}
