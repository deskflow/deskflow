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
#include "XSynergy.h"

using namespace CryptoPP;

CCryptoMode::CCryptoMode(ECryptoMode mode, bool encryption) :
	m_mode(mode),
	m_crypto(NULL),
	m_encryption(encryption)
{
	if (m_encryption) {
		switch (m_mode) {
		case kOfb:
			m_crypto = new COfbModeEnc;
			break;

		case kCfb:
			m_crypto = new CCfbModeEnc;
			break;

		case kCtr:
			m_crypto = new CCtrModeEnc;
			break;

		case kGcm:
			m_crypto = new CGcmModeEnc;
			break;

		case kDisabled:
			break;

		default:
			throw XBadCryptoMode();
		}
	}
	else {
		switch (m_mode) {
		case kOfb:
			m_crypto = new COfbModeDec;
			break;

		case kCfb:
			m_crypto = new CCfbModeDec;
			break;

		case kCtr:
			m_crypto = new CCtrModeDec;
			break;

		case kGcm:
			m_crypto = new CGcmModeDec;
			break;

		case kDisabled:
			break;

		default:
			throw XBadCryptoMode();
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
		case kOfb:
			delete reinterpret_cast<COfbModeEnc*>(m_crypto);
			break;

		case kCfb:
			delete reinterpret_cast<CCfbModeEnc*>(m_crypto);
			break;

		case kCtr:
			delete reinterpret_cast<CCtrModeEnc*>(m_crypto);
			break;

		case kGcm:
			delete reinterpret_cast<CGcmModeEnc*>(m_crypto);
			break;
		}
	}
	else {
		switch (m_mode) {
		case kOfb:
			delete reinterpret_cast<COfbModeDec*>(m_crypto);
			break;

		case kCfb:
			delete reinterpret_cast<CCfbModeDec*>(m_crypto);
			break;

		case kCtr:
			delete reinterpret_cast<CCtrModeDec*>(m_crypto);
			break;

		case kGcm:
			delete reinterpret_cast<CGcmModeDec*>(m_crypto);
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
		case kOfb:
			reinterpret_cast<COfbModeEnc*>(m_crypto)->ProcessData(out, in, length);
			break;

		case kCfb:
			reinterpret_cast<CCfbModeEnc*>(m_crypto)->ProcessData(out, in, length);
			break;

		case kCtr:
			reinterpret_cast<CCtrModeEnc*>(m_crypto)->ProcessData(out, in, length);
			break;

		case kGcm:
			reinterpret_cast<CGcmModeEnc*>(m_crypto)->ProcessData(out, in, length);
			break;
		}
	}
	else {
		switch (m_mode) {
		case kOfb:
			reinterpret_cast<COfbModeDec*>(m_crypto)->ProcessData(out, in, length);
			break;

		case kCfb:
			reinterpret_cast<CCfbModeDec*>(m_crypto)->ProcessData(out, in, length);
			break;

		case kCtr:
			reinterpret_cast<CCtrModeDec*>(m_crypto)->ProcessData(out, in, length);
			break;

		case kGcm:
			reinterpret_cast<CGcmModeDec*>(m_crypto)->ProcessData(out, in, length);
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
		case kOfb:
			reinterpret_cast<COfbModeEnc*>(m_crypto)->SetKeyWithIV(key, length, iv);
			break;

		case kCfb:
			reinterpret_cast<CCfbModeEnc*>(m_crypto)->SetKeyWithIV(key, length, iv);
			break;

		case kCtr:
			reinterpret_cast<CCtrModeEnc*>(m_crypto)->SetKeyWithIV(key, length, iv);
			break;

		case kGcm:
			reinterpret_cast<CGcmModeEnc*>(m_crypto)->SetKeyWithIV(key, length, iv);
			break;
		}
	}
	else {
		switch (m_mode) {
		case kOfb:
			reinterpret_cast<COfbModeDec*>(m_crypto)->SetKeyWithIV(key, length, iv);
			break;

		case kCfb:
			reinterpret_cast<CCfbModeDec*>(m_crypto)->SetKeyWithIV(key, length, iv);
			break;

		case kCtr:
			reinterpret_cast<CCtrModeDec*>(m_crypto)->SetKeyWithIV(key, length, iv);
			break;

		case kGcm:
			reinterpret_cast<CGcmModeDec*>(m_crypto)->SetKeyWithIV(key, length, iv);
			break;
		}
	}
}
