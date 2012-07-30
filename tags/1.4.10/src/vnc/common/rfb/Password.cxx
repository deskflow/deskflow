/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

//
// XXX not thread-safe, because d3des isn't - do we need to worry about this?
//

#include <string.h>
extern "C" {
#include <rfb/d3des.h>
}
#include <rdr/types.h>
#include <rdr/Exception.h>
#include <rfb/Password.h>

using namespace rfb;

static unsigned char d3desObfuscationKey[] = {23,82,107,6,35,78,88,7};


PlainPasswd::PlainPasswd() {}

PlainPasswd::PlainPasswd(char* pwd) : CharArray(pwd) {
}

PlainPasswd::PlainPasswd(const ObfuscatedPasswd& obfPwd) : CharArray(9) {
  if (obfPwd.length < 8)
    throw rdr::Exception("bad obfuscated password length");
  deskey(d3desObfuscationKey, DE1);
  des((rdr::U8*)obfPwd.buf, (rdr::U8*)buf);
  buf[8] = 0;
}

PlainPasswd::~PlainPasswd() {
  replaceBuf(0);
}

void PlainPasswd::replaceBuf(char* b) {
  if (buf)
    memset(buf, 0, strlen(buf));
  CharArray::replaceBuf(b);
}


ObfuscatedPasswd::ObfuscatedPasswd() : length(0) {
}

ObfuscatedPasswd::ObfuscatedPasswd(int len) : CharArray(len), length(len) {
}

ObfuscatedPasswd::ObfuscatedPasswd(const PlainPasswd& plainPwd) : CharArray(8), length(8) {
  int l = strlen(plainPwd.buf), i;
  for (i=0; i<8; i++)
    buf[i] = i<l ? plainPwd.buf[i] : 0;
  deskey(d3desObfuscationKey, EN0);
  des((rdr::U8*)buf, (rdr::U8*)buf);
}

ObfuscatedPasswd::~ObfuscatedPasswd() {
  if (buf)
    memset(buf, 0, length);
}
