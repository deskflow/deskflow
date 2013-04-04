/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Bolton Software Ltd.
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

#include "BasicTypes.h"
#include "CStreamFilter.h"
#include "cryptopp562/gcm.h"
#include "cryptopp562/aes.h"

//! Bidirectional encrypted stream
/*!
Encrypts (on write) and decrypts (on read) to and from an underlying stream.
*/
class CCryptoStream : public CStreamFilter {
public:
	CCryptoStream(IEventQueue& eventQueue, synergy::IStream* stream);
	virtual ~CCryptoStream();

	//! @name manipulators
	//@{

	//! Read from stream
	/*!
	Read up to \p n bytes into \p buffer to the stream using encryption.
	Returns the number of bytes read by the underlying stream.
	*/
	virtual UInt32		read(void* out, UInt32 n);

	//! Write to stream
	/*!
	Write \c n bytes from \c buffer to the stream using encryption.
	*/
	virtual void		write(const void* in, UInt32 n);

private:
	// TODO: allow user to change between GCM/CTR/CFB
	CryptoPP::GCM<CryptoPP::AES>::Encryption		m_encryption;
	CryptoPP::GCM<CryptoPP::AES>::Decryption		m_decryption;
};
