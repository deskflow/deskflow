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

// TODO: these are just for testing -- make sure they're gone by release!
const byte g_key1[] = "aaaaaaaaaaaaaaa";
const byte g_key2[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
const byte g_iv1[] = "aaaaaaaaaaaaaaa";
const byte g_iv2[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

using namespace CryptoPP;

CCryptoStream::CCryptoStream(IEventQueue& eventQueue, synergy::IStream* stream) :
	CStreamFilter(eventQueue, stream, false)
{
	m_encryption.SetKeyWithIV(g_key1, sizeof(g_key1), g_iv1);
	m_decryption.SetKeyWithIV(g_key1, sizeof(g_key1), g_iv1);
}

CCryptoStream::~CCryptoStream()
{
}

UInt32
CCryptoStream::read(void* out, UInt32 n)
{
	byte* in = new byte[n];
	int result = getStream()->read(in, n);
	m_decryption.ProcessData(static_cast<byte*>(out), in, n);
	delete[] in;
	return result;
}

void
CCryptoStream::write(const void* in, UInt32 n)
{
	byte* out = new byte[n];
	m_encryption.ProcessData(out, static_cast<const byte*>(in), n);
	getStream()->write(out, n);
	delete[] out;
}
