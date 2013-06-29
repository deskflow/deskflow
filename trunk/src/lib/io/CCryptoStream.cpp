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

#include "CCryptoStream.h"
#include "CLog.h"
#include "CCryptoOptions.h"
#include <sstream>
#include <string>
#include <stdio.h>

using namespace CryptoPP;
using namespace synergy::crypto;

CCryptoStream::CCryptoStream(
		IEventQueue* events,
		synergy::IStream* stream,
		const CCryptoOptions& options,
		bool adoptStream) :
	CStreamFilter(events, stream, adoptStream),
	m_key(NULL),
	m_encryption(options.m_mode, true),
	m_decryption(options.m_mode, false)
{
	LOG((CLOG_INFO "crypto mode: %s", options.m_modeString.c_str()));

	m_key = new byte[kKeyLength];
	if (!options.m_pass.empty()) {
		createKey(m_key, options.m_pass, kKeyLength, static_cast<UInt8>(options.m_pass.length()));

		byte iv[CRYPTO_IV_SIZE];
		createKey(iv, options.m_pass, CRYPTO_IV_SIZE, static_cast<UInt8>(options.m_pass.length()) * 2);
		setEncryptIv(iv);
		setDecryptIv(iv);
	}
}

CCryptoStream::~CCryptoStream()
{
	delete[] m_key;
}

UInt32
CCryptoStream::read(void* out, UInt32 n)
{
	assert(m_key != NULL);
	LOG((CLOG_DEBUG4 "crypto: read %i (decrypt)", n));

	byte* cypher = new byte[n];
	int result = getStream()->read(cypher, n);
	if (result == 0) {
		// nothing to read.
		return 0;
	}

	if (result != n) {
		LOG((CLOG_ERR "crypto: decrypt failed, only %i of %i bytes", result, n));
		return 0;
	}

	logBuffer("cypher", cypher, n);
	m_decryption.processData(static_cast<byte*>(out), cypher, n);
	logBuffer("plaintext", static_cast<byte*>(out), n);
	delete[] cypher;
	return result;
}

void
CCryptoStream::write(const void* in, UInt32 n)
{
	assert(m_key != NULL);
	LOG((CLOG_DEBUG4 "crypto: write %i (encrypt)", n));

	logBuffer("plaintext", static_cast<byte*>(const_cast<void*>(in)), n);
	byte* cypher = new byte[n];
	m_encryption.processData(cypher, static_cast<const byte*>(in), n);
	logBuffer("cypher", cypher, n);
	getStream()->write(cypher, n);
	delete[] cypher;
}

void
CCryptoStream::createKey(byte* out, const CString& password, UInt8 keyLength, UInt8 hashCount)
{
	assert(keyLength <= SHA256::DIGESTSIZE);

	byte temp[SHA256::DIGESTSIZE];
	byte* in = reinterpret_cast<byte*>(const_cast<char*>(password.c_str()));
	SHA256().CalculateDigest(temp, in, password.length());

	byte* tempKey = new byte[SHA256::DIGESTSIZE];
	for (int i = 0; i < hashCount; ++i) {
		memcpy(tempKey, temp, SHA256::DIGESTSIZE);
		SHA256().CalculateDigest(temp, tempKey, SHA256::DIGESTSIZE);
	}
	delete[] tempKey;

	memcpy(out, temp, keyLength);
}

void
CCryptoStream::setEncryptIv(const byte* iv)
{
	assert(m_key != NULL);
	logBuffer("encrypt iv", iv, CRYPTO_IV_SIZE);
	m_encryption.setKeyWithIv(m_key, kKeyLength, iv);
}

void
CCryptoStream::setDecryptIv(const byte* iv)
{
	assert(m_key != NULL);
	logBuffer("decrypt iv", iv, CRYPTO_IV_SIZE);
	m_decryption.setKeyWithIv(m_key, kKeyLength, iv);
}

void
CCryptoStream::newIv(byte* out)
{
	m_autoSeedRandomPool.GenerateBlock(out, CRYPTO_IV_SIZE);
}

void
CCryptoStream::logBuffer(const char* name, const byte* buf, int length)
{
	if (CLOG->getFilter() < kDEBUG4) {
		return;
	}

	std::stringstream ss;
	ss << "crypto: " << name << ":";

	char buffer[4];
	for (int i = 0; i < length; i++) {
		sprintf(buffer, " %02X", buf[i]);
		ss << buffer;
	}

	LOG((CLOG_DEBUG4 "%s", ss.str().c_str()));
}
