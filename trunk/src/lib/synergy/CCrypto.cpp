/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "CCrypto.h"

#include "cryptlib.h"
#include "default.h"
#include "modes.h"
#include "aes.h"
#include "randpool.h"
#include "files.h"
#include "hex.h"
#include "rsa.h"
#include <time.h>

using namespace CryptoPP;

static OFB_Mode<AES>::Encryption s_globalRNG;

void CCrypto::test()
{
    std::string seed = IntToString(time(NULL));
    seed.resize(16);
    s_globalRNG.SetKeyWithIV((byte *)seed.data(), 16, (byte *)seed.data());
}
