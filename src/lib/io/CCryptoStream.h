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
#include "CCryptoMode.h"
#include <cryptopp562/osrng.h>
#include <cryptopp562/sha.h>

class CCryptoOptions;

#define CRYPTO_IV_SIZE CryptoPP::AES::BLOCKSIZE

//! Bidirectional encrypted stream
/*!
Encrypts (on write) and decrypts (on read) to and from an underlying stream.
*/
class CCryptoStream : public CStreamFilter {
public:
	CCryptoStream(IEventQueue* eventQueue, synergy::IStream* stream, const CCryptoOptions& options, bool adoptStream = true);
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

	//! Set the IV for encryption
	void				setEncryptIv(const byte* iv);
	
	//! Set the IV for decryption
	void				setDecryptIv(const byte* iv);

	//! Get a new IV
	/*!
	Writes a new IV to the \c out buffer, and also uses the IV for further
	crypto.
	*/
	void				newIv(byte* out);

	//! Creates a key from a password
	static void			createKey(byte* out, const CString& password, UInt8 keyLength, UInt8 hashCount);

private:
	void				logBuffer(const char* name, const byte* buf, int length);
	
	byte*				m_key;
	CCryptoMode			m_encryption;
	CCryptoMode			m_decryption;
	CryptoPP::AutoSeededRandomPool m_autoSeedRandomPool;
};

namespace synergy {
namespace crypto {

const UInt32 kKeyLength = 32;

}
}
