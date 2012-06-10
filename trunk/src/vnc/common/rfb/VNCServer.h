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
// VNCServer - abstract interface implemented by the RFB library.  The back-end
// code calls the relevant methods as appropriate.

#ifndef __RFB_VNCSERVER_H__
#define __RFB_VNCSERVER_H__

#include <rfb/UpdateTracker.h>
#include <rfb/SSecurity.h>

namespace rfb {

  class VNCServer : public UpdateTracker {
  public:

    // setPixelBuffer() tells the server to use the given pixel buffer.  If
    // this differs in size from the previous pixel buffer, this may result in
    // protocol messages being sent, or clients being disconnected.
    virtual void setPixelBuffer(PixelBuffer* pb) = 0;

    // setColourMapEntries() tells the server that some entries in the colour
    // map have changed.  The server will retrieve them via the PixelBuffer's
    // ColourMap object.  This may result in protocol messages being sent.
    // If nColours is 0, this means the rest of the colour map.
    virtual void setColourMapEntries(int firstColour=0, int nColours=0) = 0;

    // serverCutText() tells the server that the cut text has changed.  This
    // will normally be sent to all clients.
    virtual void serverCutText(const char* str, int len) = 0;

    // bell() tells the server that it should make all clients make a bell sound.
    virtual void bell() = 0;

    // clientsReadyForUpdate() returns true if there is at least one client
    // waiting for an update, false if no clients are ready.
    virtual bool clientsReadyForUpdate() = 0;

    // - Close all currently-connected clients, by calling
    //   their close() method with the supplied reason.
    virtual void closeClients(const char* reason) = 0;

    // tryUpdate() causes the server to attempt to send updates to any waiting
    // clients.
    virtual void tryUpdate() = 0;

    // setCursor() tells the server that the cursor has changed.  The
    // cursorData argument contains width*height pixel values in the pixel
    // buffer's format.  The mask argument is a bitmask with a 1-bit meaning
    // the corresponding pixel in cursorData is valid.  The mask consists of
    // left-to-right, top-to-bottom scanlines, where each scanline is padded to
    // a whole number of bytes [(width+7)/8].  Within each byte the most
    // significant bit represents the leftmost pixel, and the bytes are simply
    // in left-to-right order.  The server takes its own copy of the data in
    // cursorData and mask.
    virtual void setCursor(int width, int height, const Point& hotspot,
                           void* cursorData, void* mask) = 0;

    // setCursorPos() tells the server the current position of the cursor.
    virtual void setCursorPos(const Point& p) = 0;

    // setSSecurityFactory() tells the server which factory to use when
    // attempting to authenticate connections.
    virtual void setSSecurityFactory(SSecurityFactory* f) = 0;

    // setName() tells the server what desktop title to supply to clients
    virtual void setName(const char* name) = 0;
  };
}
#endif
