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
// VNCSConnectionST is our derived class of SConnection for VNCServerST - there
// is one for each connected client.  We think of VNCSConnectionST as part of
// the VNCServerST implementation, so its methods are allowed full access to
// members of VNCServerST.
//

#ifndef __RFB_VNCSCONNECTIONST_H__
#define __RFB_VNCSCONNECTIONST_H__

#include <set>
#include <rfb/SConnection.h>
#include <rfb/SMsgWriter.h>
#include <rfb/TransImageGetter.h>
#include <rfb/VNCServerST.h>

namespace rfb {
  class VNCSConnectionST : public SConnection,
                           public WriteSetCursorCallback {
  public:
    VNCSConnectionST(VNCServerST* server_, network::Socket* s, bool reverse);
    virtual ~VNCSConnectionST();

    // Methods called from VNCServerST.  None of these methods ever knowingly
    // throw an exception.

    // Unless otherwise stated, the SConnectionST may not be valid after any of
    // these methods are called, since they catch exceptions and may have
    // called close() which deletes the object.

    // init() must be called to initialise the protocol.  If it fails it
    // returns false, and close() will have been called.
    bool init();

    // close() shuts down the socket to the client and deletes the
    // SConnectionST object.
    void close(const char* reason);

    // processMessages() processes incoming messages from the client, invoking
    // various callbacks as a result.  It continues to process messages until
    // reading might block.  shutdown() will be called on the connection's
    // Socket if an error occurs, via the close() call.
    void processMessages();

    void writeFramebufferUpdateOrClose();
    void pixelBufferChange();
    void setColourMapEntriesOrClose(int firstColour, int nColours);
    void bell();
    void serverCutText(const char *str, int len);
    void setCursorOrClose();

    // checkIdleTimeout() returns the number of milliseconds left until the
    // idle timeout expires.  If it has expired, the connection is closed and
    // zero is returned.  Zero is also returned if there is no idle timeout.
    int checkIdleTimeout();

    // The following methods never throw exceptions nor do they ever delete the
    // SConnectionST object.

    // renderedCursorChange() is called whenever the server-side rendered
    // cursor changes shape or position.  It ensures that the next update will
    // clean up the old rendered cursor and if necessary draw the new rendered
    // cursor.
    void renderedCursorChange();

    // needRenderedCursor() returns true if this client needs the server-side
    // rendered cursor.  This may be because it does not support local cursor
    // or because the current cursor position has not been set by this client.
    bool needRenderedCursor();

    network::Socket* getSock() { return sock; }
    bool readyForUpdate() { return !requested.is_empty(); }
    void add_changed(const Region& region) { updates.add_changed(region); }
    void add_copied(const Region& dest, const Point& delta) {
      updates.add_copied(dest, delta);
    }

    const char* getPeerEndpoint() const {return peerEndpoint.buf;}

    // approveConnectionOrClose() is called some time after
    // VNCServerST::queryConnection() has returned with PENDING to accept or
    // reject the connection.  The accept argument should be true for
    // acceptance, or false for rejection, in which case a string reason may
    // also be given.

    void approveConnectionOrClose(bool accept, const char* reason);

  private:
    // SConnection callbacks

    // These methods are invoked as callbacks from processMsg().  Note that
    // none of these methods should call any of the above methods which may
    // delete the SConnectionST object.

    virtual void authSuccess();
    virtual void queryConnection(const char* userName);
    virtual void clientInit(bool shared);
    virtual void setPixelFormat(const PixelFormat& pf);
    virtual void pointerEvent(const Point& pos, int buttonMask);
    virtual void keyEvent(rdr::U32 key, bool down);
    virtual void clientCutText(const char* str, int len);
    virtual void framebufferUpdateRequest(const Rect& r, bool incremental);
    virtual void setInitialColourMap();
    virtual void supportsLocalCursor();

    // setAccessRights() allows a security package to limit the access rights
    // of a VNCSConnectioST to the server.  These access rights are applied
    // such that the actual rights granted are the minimum of the server's
    // default access settings and the connection's access settings.
    virtual void setAccessRights(AccessRights ar) {accessRights=ar;}

    // WriteSetCursorCallback
    virtual void writeSetCursorCallback();

    // Internal methods

    // writeFramebufferUpdate() attempts to write a framebuffer update to the
    // client.

    void writeFramebufferUpdate();

    void writeRenderedCursorRect();
    void setColourMapEntries(int firstColour, int nColours);
    void setCursor();
    void setSocketTimeouts();

    network::Socket* sock;
    CharArray peerEndpoint;
    VNCServerST* server;
    SimpleUpdateTracker updates;
    TransImageGetter image_getter;
    Region requested;
    bool drawRenderedCursor, removeRenderedCursor;
    Rect renderedCursorRect;

    std::set<rdr::U32> pressedKeys;

    time_t lastEventTime;
    time_t pointerEventTime;
    Point pointerEventPos;

    AccessRights accessRights;

    CharArray closeReason;
  };
}
#endif
