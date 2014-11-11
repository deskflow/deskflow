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

#include "io/CryptoMode.h"

#include "io/XIO.h"

using namespace CryptoPP;

CryptoMode::CryptoMode(ECryptoMode mode, bool encryption) :
	m_mode(mode),
	m_crypto(NULL),
	m_encryption(encryption)
{
	if (m_encryption) {
		switch (m_mode) {
		case kCfb:
			m_crypto = new CfbModeEnc;
			break;

		case kDisabled:
			break;

		default:
			throw XIOBadCryptoMode();
		}
	}
	else {
		switch (m_mode) {
		case kCfb:
			m_crypto = new CfbModeDec;
			break;

		case kDisabled:
			break;

		default:
			throw XIOBadCryptoMode();
		}
	}
}

CryptoMode::~CryptoMode()
{
	if (m_crypto == NULL) {
		return;
	}

	if (m_encryption) {
		delete reinterpret_cast<CfbModeEnc*>(m_crypto);
	}
	else {
		delete reinterpret_cast<CfbModeDec*>(m_crypto);
	}
}

void
CryptoMode::processData(byte* out, const byte* in, size_t length)
{
	if (m_crypto == NULL) {
		return;
	}

	if (m_encryption) {
		reinterpret_cast<CfbModeEnc*>(m_crypto)->ProcessData(out, in, length);
	}
	else {
		reinterpret_cast<CfbModeDec*>(m_crypto)->ProcessData(out, in, length);
	}
}


void
CryptoMode::setKeyWithIv(const byte* key, size_t length, const byte* iv)
{
	if (m_crypto == NULL) {
		return;
	}

	if (m_encryption) {
		reinterpret_cast<CfbModeEnc*>(m_crypto)->SetKeyWithIV(key, length, iv);
	}
	else {
		reinterpret_cast<CfbModeDec*>(m_crypto)->SetKeyWithIV(key, length, iv);
	}
}
