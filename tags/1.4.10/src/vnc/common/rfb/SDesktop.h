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

/////////////////////////////////////////////////////////////////////////////

// SDesktop is an interface implemented by back-ends, on which callbacks are
// made by the VNCServer as appropriate for pointer and keyboard events, etc.
// SDesktop objects are always created before the VNCServer - the SDesktop
// will be passed a pointer to the VNCServer in the start() call.  If a more
// implementation-specific pointer to the VNCServer is required then this
// can be provided to the SDesktop via an implementation-specific method.
//
// An SDesktop usually has an associated PixelBuffer which it tells the
// VNCServer via the VNCServer's setPixelBuffer() method.  It can do this at
// any time, but the PixelBuffer MUST be valid by the time the call to start()
// returns.  The PixelBuffer may be set to null again if desired when stop() is
// called.  Note that start() and stop() are guaranteed to be called
// alternately; there should never be two calls to start() without an
// intervening stop() and vice-versa.
//

#ifndef __RFB_SDESKTOP_H__
#define __RFB_SDESKTOP_H__

#include <rfb/PixelBuffer.h>
#include <rfb/VNCServer.h>
#include <rfb/InputHandler.h>
#include <rfb/Exception.h>

namespace rfb {

  class VNCServer;

  class SDesktop : public InputHandler {
  public:
    // start() is called by the server when the first client authenticates
    // successfully, and can be used to begin any expensive tasks which are not
    // needed when there are no clients.  A valid PixelBuffer must have been
    // set via the VNCServer's setPixelBuffer() method by the time this call
    // returns.

    virtual void start(VNCServer* vs) {}

    // stop() is called by the server when there are no longer any
    // authenticated clients, and therefore the desktop can cease any
    // expensive tasks.  No further calls to the VNCServer passed to start()
    // can be made once stop has returned.

    virtual void stop() {}

    // framebufferUpdateRequest() is called to let the desktop know that at
    // least one client has become ready for an update.  Desktops can check
    // whether there are clients ready at any time by calling the VNCServer's
    // clientsReadyForUpdate() method.

    virtual void framebufferUpdateRequest() {}

    // getFbSize() returns the current dimensions of the framebuffer.
    // This can be called even while the SDesktop is not start()ed.

    virtual Point getFbSize() = 0;

    // InputHandler interface
    // pointerEvent(), keyEvent() and clientCutText() are called in response to
    // the relevant RFB protocol messages from clients.
    // See InputHandler for method signatures.
  protected:
    virtual ~SDesktop() {}
  };

  // -=- SStaticDesktop
  //     Trivial implementation of the SDesktop interface, which provides
  //     dummy input handlers and event processing routine, and exports
  //     a plain black desktop of the specified format.
  class SStaticDesktop : public SDesktop {
  public:
    SStaticDesktop(const Point& size) : server(0), buffer(0) {
      PixelFormat pf;
      buffer = new ManagedPixelBuffer(pf, size.x, size.y);
      if (buffer) memset(buffer->data, 0, (pf.bpp/8) * (size.x*size.y));
    }
    SStaticDesktop(const Point& size, const PixelFormat& pf) : buffer(0) {
      buffer = new ManagedPixelBuffer(pf, size.x, size.y);
      if (buffer) memset(buffer->data, 0, (pf.bpp/8) * (size.x*size.y));
    }
    virtual ~SStaticDesktop() {
      if (buffer) delete buffer;
    }

    virtual void start(VNCServer* vs) {
      server = vs;
      server->setPixelBuffer(buffer);
    }
    virtual void stop() {
      server->setPixelBuffer(0);
      server = 0;
    }

  protected:
    VNCServer* server;
    ManagedPixelBuffer* buffer;
  };

};

#endif // __RFB_SDESKTOP_H__
