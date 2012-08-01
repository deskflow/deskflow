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
// SMsgHandler - class to handle incoming messages on the server side.
//

#ifndef __RFB_SMSGHANDLER_H__
#define __RFB_SMSGHANDLER_H__

#include <rdr/types.h>
#include <rfb/PixelFormat.h>
#include <rfb/ConnParams.h>
#include <rfb/InputHandler.h>

namespace rdr { class InStream; }

namespace rfb {

  class SMsgHandler : public InputHandler {
  public:
    SMsgHandler();
    virtual ~SMsgHandler();

    // The following methods are called as corresponding messages are read.  A
    // derived class should override these methods as desired.  Note that for
    // the setPixelFormat() and setEncodings() methods, a derived class must
    // call on to SMsgHandler's methods.

    virtual void clientInit(bool shared);

    virtual void setPixelFormat(const PixelFormat& pf);
    virtual void setEncodings(int nEncodings, rdr::U32* encodings);
    virtual void framebufferUpdateRequest(const Rect& r, bool incremental);

    // InputHandler interface
    // The InputHandler methods will be called for the corresponding messages.

    // supportsLocalCursor() is called whenever the status of
    // cp.supportsLocalCursor has changed.  At the moment this happens on a
    // setEncodings message, but in the future this may be due to a message
    // specially for this purpose.
    virtual void supportsLocalCursor();

    ConnParams cp;
  };
}
#endif
