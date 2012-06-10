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
// SSecurity - class on the server side for handling security handshaking.  A
// derived class for a particular security type overrides the processMsg()
// method.

// processMsg() is called first when the security type has been decided on, and
// will keep being called whenever there is data to read from the client.  It
// should return false when it needs more data, or true when the connection has
// been successfully authenticated.  In the event of authentication failure an
// AuthFailureException should be thrown - this will result in a "failed"
// security result being sent to the client with the str() from the exception
// being sent as the reason.  Any other type of failure should be indicated by
// some other kind of exception which will cause the connection to be aborted.
//
// processMsg() must never block (or at least must never block until the client
// has been authenticated) - this is to prevent denial of service attacks.
// Note that the first time processMsg() is called, there is no guarantee that
// there is any data to read from the SConnection's InStream, but subsequent
// calls guarantee there is at least one byte which can be read without
// blocking.
//
// getType() should return the secType value corresponding to the SSecurity
// implementation.
//

#ifndef __RFB_SSECURITY_H__
#define __RFB_SSECURITY_H__

#include <rdr/types.h>
#include <rfb/util.h>
#include <list>

namespace rfb {

  class SConnection;

  class SSecurity {
  public:
    virtual ~SSecurity() {}
    virtual bool processMsg(SConnection* sc)=0;
    virtual void destroy() { delete this; }
    virtual int getType() const = 0;

    // getUserName() gets the name of the user attempting authentication.  The
    // storage is owned by the SSecurity object, so a copy must be taken if
    // necessary.  Null may be returned to indicate that there is no user name
    // for this security type.
    virtual const char* getUserName() const = 0;
  };

  // SSecurityFactory creates new SSecurity instances for
  // particular security types.
  // The instances must be destroyed by calling destroy()
  // on them when done.
  // getSecTypes returns a list of the security types that are both configured
  // and actually supported.  Which configuration is considered depends on the
  // reverseConnection parameter.
  class SSecurityFactory {
  public:
    virtual ~SSecurityFactory() {}
    virtual SSecurity* getSSecurity(rdr::U8 secType, bool noAuth=false)=0;
    virtual void getSecTypes(std::list<rdr::U8>* secTypes,
                             bool reverseConnection) = 0;
  };

}
#endif
