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

#include <cryptopp562/gcm.h>
#include <cryptopp562/modes.h>
#include <cryptopp562/aes.h>
#include "ECryptoMode.h"
#include "CString.h"

//! Encapsulation of modes
/*!
Polymorphism is tricky in Crypto++, so we encapsulate all crypto modes
and switch based on an enum for ctor, dtor and all functions.
*/
class CCryptoMode {
public:
	CCryptoMode(ECryptoMode mode, bool encryption = true);
	~CCryptoMode();

	//! Encrypt or decrypt data
	void				processData(byte* out, const byte* in, size_t length);

	//! Variable length key and initialization vector
	void				setKeyWithIv(const byte* key, size_t length, const byte* iv);
	
private:
	typedef	CryptoPP::OFB_Mode<CryptoPP::AES>::Encryption COfbModeEnc;
	typedef	CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption CCfbModeEnc;
	typedef CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption CCtrModeEnc;
	typedef CryptoPP::GCM<CryptoPP::AES>::Encryption CGcmModeEnc;
		
	typedef CryptoPP::OFB_Mode<CryptoPP::AES>::Decryption COfbModeDec;
	typedef CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption CCfbModeDec;
	typedef CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption CCtrModeDec;
	typedef CryptoPP::GCM<CryptoPP::AES>::Decryption CGcmModeDec;
	
	static ECryptoMode		parseMode(CString& mode);

	ECryptoMode			m_mode;
	void*				m_crypto;
	bool				m_encryption;
};
