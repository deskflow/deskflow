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
// CSecurityVncAuth
//
// XXX not thread-safe, because d3des isn't - do we need to worry about this?
//

#include <string.h>
#include <stdio.h>
#include <rfb/CConnection.h>
#include <rfb/UserPasswdGetter.h>
#include <rfb/Password.h>
#include <rfb/CSecurityVncAuth.h>
#include <rfb/util.h>
extern "C" {
#include <rfb/d3des.h>
}


using namespace rfb;

static const int vncAuthChallengeSize = 16;


CSecurityVncAuth::CSecurityVncAuth(UserPasswdGetter* upg_)
  : upg(upg_)
{
}

CSecurityVncAuth::~CSecurityVncAuth()
{
}

bool CSecurityVncAuth::processMsg(CConnection* cc)
{
  rdr::InStream* is = cc->getInStream();
  rdr::OutStream* os = cc->getOutStream();

  // Read the challenge & obtain the user's password
  rdr::U8 challenge[vncAuthChallengeSize];
  is->readBytes(challenge, vncAuthChallengeSize);
  PlainPasswd passwd;
  upg->getUserPasswd(0, &passwd.buf);

  // Calculate the correct response
  rdr::U8 key[8];
  int pwdLen = strlen(passwd.buf);
  for (int i=0; i<8; i++)
    key[i] = i<pwdLen ? passwd.buf[i] : 0;
  deskey(key, EN0);
  for (int j = 0; j < vncAuthChallengeSize; j += 8)
    des(challenge+j, challenge+j);

  // Return the response to the server
  os->writeBytes(challenge, vncAuthChallengeSize);
  os->flush();
  return true;
}
