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
// CSecurity - class on the client side for handling security handshaking.  A
// derived class for a particular security type overrides the processMsg()
// method.

// processMsg() is called first when the security type has been decided on, and
// will keep being called whenever there is data to read from the server.  It
// should return false when it needs more data, or true when the security
// handshaking is over and we are now waiting for the SecurityResult message
// from the server.  In the event of failure a suitable exception should be
// thrown.
//
// Note that the first time processMsg() is called, there is no guarantee that
// there is any data to read from the CConnection's InStream, but subsequent
// calls guarantee there is at least one byte which can be read without
// blocking.
//
// description is a string describing the level of encryption applied to the
// session, or null if no encryption will be used.

#ifndef __RFB_CSECURITY_H__
#define __RFB_CSECURITY_H__

namespace rfb {
  class CConnection;
  class CSecurity {
  public:
    virtual ~CSecurity() {}
    virtual bool processMsg(CConnection* cc)=0;
    virtual void destroy() { delete this; }
    virtual int getType() const = 0;
    virtual const char* description() const = 0;
  };
}
#endif
