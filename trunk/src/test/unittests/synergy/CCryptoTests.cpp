/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2011 Nick Bolton
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

#include <gtest/gtest.h>
#include "CClipboard.h"
#include <string>
#include "gcm.h"
#include "aes.h"
#include "filters.h"

using namespace std;
using namespace CryptoPP;

TEST(CCryptoTests, encrypt)
{
	string plaintext = "hello", ciphertext;
	const byte key[] = "123456781234567";
	const byte iv[] = "123456781234567";

	GCM<AES>::Encryption enc;
	enc.SetKeyWithIV(key, sizeof(key), iv, sizeof(iv));

	AuthenticatedEncryptionFilter aef(enc, new StringSink(ciphertext));

	aef.Put((const byte*)plaintext.data(), plaintext.size());
	aef.MessageEnd();

	EXPECT_EQ("Vh\x86r\xF4\xD0\xD7\xE0\x95\xDE\xCB\xB7\xFA@\v\xFE\xEE\\\xF8\xE8V", ciphertext);
}

TEST(CCryptoTests, decrypt)
{
	string ciphertext = "Vh\x86r\xF4\xD0\xD7\xE0\x95\xDE\xCB\xB7\xFA@\v\xFE\xEE\\\xF8\xE8V", plaintext;
	const byte key[] = "123456781234567";
	const byte iv[] = "123456781234567";

	GCM<AES>::Decryption dec;
	dec.SetKeyWithIV(key, sizeof(key), iv, sizeof(iv));

	AuthenticatedDecryptionFilter adf(dec, new StringSink(plaintext));

	adf.Put((const byte*)ciphertext.data(), ciphertext.size());
	adf.MessageEnd();
	
	EXPECT_EQ("hello", plaintext);
}
