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

#include "CCryptoMode.h"
#include "XIO.h"

using namespace CryptoPP;

CCryptoMode::CCryptoMode(ECryptoMode mode, bool encryption) :
	m_mode(mode),
	m_crypto(NULL),
	m_encryption(encryption)
{
	if (m_encryption) {
		switch (m_mode) {
		case kCfb:
			m_crypto = new CCfbModeEnc;
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
			m_crypto = new CCfbModeDec;
			break;

		case kDisabled:
			break;

		default:
			throw XIOBadCryptoMode();
		}
	}
}

CCryptoMode::~CCryptoMode()
{
	if (m_crypto == NULL) {
		return;
	}

	if (m_encryption) {
		switch (m_mode) {
		case kCfb:
			delete reinterpret_cast<CCfbModeEnc*>(m_crypto);
			break;
		}
	}
	else {
		switch (m_mode) {
		case kCfb:
			delete reinterpret_cast<CCfbModeDec*>(m_crypto);
			break;
		}
	}
}

void
CCryptoMode::processData(byte* out, const byte* in, size_t length)
{
	if (m_crypto == NULL) {
		return;
	}

	if (m_encryption) {
		switch (m_mode) {
		case kCfb:
			reinterpret_cast<CCfbModeEnc*>(m_crypto)->ProcessData(out, in, length);
			break;
		}
	}
	else {
		switch (m_mode) {
		case kCfb:
			reinterpret_cast<CCfbModeDec*>(m_crypto)->ProcessData(out, in, length);
			break;
		}
	}
}


void
CCryptoMode::setKeyWithIv(const byte* key, size_t length, const byte* iv)
{
	if (m_crypto == NULL) {
		return;
	}

	if (m_encryption) {
		switch (m_mode) {
		case kCfb:
			reinterpret_cast<CCfbModeEnc*>(m_crypto)->SetKeyWithIV(key, length, iv);
			break;
		}
	}
	else {
		switch (m_mode) {
		case kCfb:
			reinterpret_cast<CCfbModeDec*>(m_crypto)->SetKeyWithIV(key, length, iv);
			break;
		}
	}
}
