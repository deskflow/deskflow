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
// SMsgReader - class for reading RFB messages on the server side
// (i.e. messages from client to server).
//

#ifndef __RFB_SMSGREADER_H__
#define __RFB_SMSGREADER_H__

namespace rdr { class InStream; }

namespace rfb {
  class SMsgHandler;

  class SMsgReader {
  public:
    virtual ~SMsgReader();

    virtual void readClientInit()=0;

    // readMsg() reads a message, calling the handler as appropriate.
    virtual void readMsg()=0;

    rdr::InStream* getInStream() { return is; }

  protected:
    virtual void readSetPixelFormat();
    virtual void readSetEncodings();
    virtual void readFramebufferUpdateRequest();
    virtual void readKeyEvent();
    virtual void readPointerEvent();
    virtual void readClientCutText();

    SMsgReader(SMsgHandler* handler, rdr::InStream* is);

    SMsgHandler* handler;
    rdr::InStream* is;
  };
}
#endif
