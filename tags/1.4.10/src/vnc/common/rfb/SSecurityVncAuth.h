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
// SSecurityVncAuth - legacy VNC authentication protocol.
// The getPasswd call can be overridden if you wish to store
// the VncAuth password in an implementation-specific place.
// Otherwise, the password is read from a BinaryParameter
// called Password.

#ifndef __RFB_SSECURITYVNCAUTH_H__
#define __RFB_SSECURITYVNCAUTH_H__

#include <rfb/SSecurity.h>
#include <rfb/secTypes.h>
#include <rdr/types.h>

namespace rfb {

  class VncAuthPasswdGetter {
  public:
    // getPasswd() returns a string or null if unsuccessful.  The
    // SSecurityVncAuth object delete[]s the string when done.
    virtual char* getVncAuthPasswd()=0;
  };

  class SSecurityVncAuth : public SSecurity {
  public:
    SSecurityVncAuth(VncAuthPasswdGetter* pg);
    virtual bool processMsg(SConnection* sc);
    virtual int getType() const {return secTypeVncAuth;}
    virtual const char* getUserName() const {return 0;}
  private:
    enum {vncAuthChallengeSize = 16};
    rdr::U8 challenge[vncAuthChallengeSize];
    rdr::U8 response[vncAuthChallengeSize];
    bool sentChallenge;
    int responsePos;
    VncAuthPasswdGetter* pg;
  };
}
#endif
