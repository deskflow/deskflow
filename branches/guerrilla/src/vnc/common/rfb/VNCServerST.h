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

// -=- VNCServerST.h

// Single-threaded VNCServer implementation

#ifndef __RFB_VNCSERVERST_H__
#define __RFB_VNCSERVERST_H__

#include <list>

#include <rfb/SDesktop.h>
#include <rfb/VNCServer.h>
#include <rfb/Configuration.h>
#include <rfb/LogWriter.h>
#include <rfb/Blacklist.h>
#include <rfb/Cursor.h>
#include <network/Socket.h>

namespace rfb {

  class VNCSConnectionST;
  class ComparingUpdateTracker;
  class PixelBuffer;
  class KeyRemapper;

  class VNCServerST : public VNCServer, public network::SocketServer {
  public:
    // -=- Constructors

    //   Create a server exporting the supplied desktop.
    VNCServerST(const char* name_, SDesktop* desktop_,
                SSecurityFactory* securityFactory_=0);
    virtual ~VNCServerST();


    // Methods overridden from SocketServer

    // addSocket
    //   Causes the server to allocate an RFB-protocol management
    //   structure for the socket & initialise it.
    virtual void addSocket(network::Socket* sock, bool outgoing=false);

    // removeSocket
    //   Clean up any resources associated with the Socket
    virtual void removeSocket(network::Socket* sock);

    // processSocketEvent
    //   Read more RFB data from the Socket.  If an error occurs during
    //   processing then shutdown() is called on the Socket, causing
    //   removeSocket() to be called by the caller at a later time.
    virtual void processSocketEvent(network::Socket* sock);

    // checkTimeouts
    //   Returns the number of milliseconds left until the next idle timeout
    //   expires.  If any have already expired, the corresponding connections
    //   are closed.  Zero is returned if there is no idle timeout.
    virtual int checkTimeouts();


    // Methods overridden from VNCServer

    virtual void setPixelBuffer(PixelBuffer* pb);
    virtual void setColourMapEntries(int firstColour=0, int nColours=0);
    virtual void serverCutText(const char* str, int len);
    virtual void add_changed(const Region &region);
    virtual void add_copied(const Region &dest, const Point &delta);
    virtual bool clientsReadyForUpdate();
    virtual void tryUpdate();
    virtual void setCursor(int width, int height, const Point& hotspot,
                           void* cursorData, void* mask);
    virtual void setCursorPos(const Point& p);
    virtual void setSSecurityFactory(SSecurityFactory* f) {securityFactory=f;}

    virtual void bell();

    // - Close all currently-connected clients, by calling
    //   their close() method with the supplied reason.
    virtual void closeClients(const char* reason) {closeClients(reason, 0);}

    // VNCServerST-only methods

    // closeClients() closes all RFB sessions, except the specified one (if
    // any), and logs the specified reason for closure.
    void closeClients(const char* reason, network::Socket* sock);

    // getSockets() gets a list of sockets.  This can be used to generate an
    // fd_set for calling select().

    void getSockets(std::list<network::Socket*>* sockets);

    // getSConnection() gets the SConnection for a particular Socket.  If
    // the Socket is not recognised then null is returned.

    SConnection* getSConnection(network::Socket* sock);

    // getDesktopSize() returns the size of the SDesktop exported by this
    // server.
    Point getDesktopSize() const {return desktop->getFbSize();}

    // getName() returns the name of this VNC Server.  NB: The value returned
    // is the server's internal buffer which may change after any other methods
    // are called - take a copy if necessary.
    const char* getName() const {return name.buf;}

    // setName() specifies the desktop name that the server should provide to
    // clients
    void setName(const char* name_) {name.replaceBuf(strDup(name_));}

    // A QueryConnectionHandler, if supplied, is passed details of incoming
    // connections to approve, reject, or query the user about.
    //
    // queryConnection() is called when a connection has been
    // successfully authenticated.  The sock and userName arguments identify
    // the socket and the name of the authenticated user, if any.  It should
    // return ACCEPT if the connection should be accepted, REJECT if it should
    // be rejected, or PENDING if a decision cannot yet be reached.  If REJECT
    // is returned, *reason can be set to a string describing the reason - this
    // will be delete[]ed when it is finished with.  If PENDING is returned,
    // approveConnection() must be called some time later to accept or reject
    // the connection.
    enum queryResult { ACCEPT, REJECT, PENDING };
    struct QueryConnectionHandler {
      virtual ~QueryConnectionHandler() {}
      virtual queryResult queryConnection(network::Socket* sock,
                                          const char* userName,
                                          char** reason) = 0;
    };
    void setQueryConnectionHandler(QueryConnectionHandler* qch) {
      queryConnectionHandler = qch;
    }

    // queryConnection is called as described above, and either passes the
    // request on to the registered handler, or accepts the connection if
    // no handler has been specified.
    virtual queryResult queryConnection(network::Socket* sock,
                                        const char* userName,
                                        char** reason) {
      return queryConnectionHandler
        ? queryConnectionHandler->queryConnection(sock, userName, reason)
        : ACCEPT;
    }

    // approveConnection() is called by the active QueryConnectionHandler,
    // some time after queryConnection() has returned with PENDING, to accept
    // or reject the connection.  The accept argument should be true for
    // acceptance, or false for rejection, in which case a string reason may
    // also be given.
    void approveConnection(network::Socket* sock, bool accept,
                           const char* reason);

    // setBlacklist() is called to replace the VNCServerST's internal
    // Blacklist instance with another instance.  This allows a single
    // Blacklist to be shared by multiple VNCServerST instances.
    void setBlacklist(Blacklist* bl) {blHosts = bl ? bl : &blacklist;}

    // setEconomicTranslate() determines (for new connections) whether pixels
    // should be translated for <=16bpp clients using a large lookup table
    // (fast) or separate, smaller R, G and B tables (slower).  If set to true,
    // small tables are used, to save memory.
    void setEconomicTranslate(bool et) { useEconomicTranslate = et; }

    // setKeyRemapper() replaces the VNCServerST's default key remapper.
    // NB: A null pointer is valid here.
    void setKeyRemapper(KeyRemapper* kr) { keyRemapper = kr; }

  protected:

    friend class VNCSConnectionST;

    void startDesktop();

    static LogWriter connectionsLog;
    Blacklist blacklist;
    Blacklist* blHosts;

    SDesktop* desktop;
    bool desktopStarted;
    PixelBuffer* pb;

    CharArray name;

    std::list<VNCSConnectionST*> clients;
    VNCSConnectionST* pointerClient;
    std::list<network::Socket*> closingSockets;

    ComparingUpdateTracker* comparer;

    Point cursorPos;
    Cursor cursor;
    Point cursorTL() { return cursorPos.subtract(cursor.hotspot); }
    Point renderedCursorTL;
    ManagedPixelBuffer renderedCursor;
    bool renderedCursorInvalid;

    // - Check how many of the clients are authenticated.
    int authClientCount();

    bool needRenderedCursor();
    void checkUpdate();

    SSecurityFactory* securityFactory;
    QueryConnectionHandler* queryConnectionHandler;
    KeyRemapper* keyRemapper;
    bool useEconomicTranslate;
  };

};

#endif

