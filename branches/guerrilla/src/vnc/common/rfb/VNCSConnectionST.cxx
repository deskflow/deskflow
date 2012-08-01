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

#include <rfb/VNCSConnectionST.h>
#include <rfb/LogWriter.h>
#include <rfb/secTypes.h>
#include <rfb/ServerCore.h>
#include <rfb/ComparingUpdateTracker.h>
#include <rfb/KeyRemapper.h>
#define XK_MISCELLANY
#define XK_XKB_KEYS
#include <rfb/keysymdef.h>

#pragma warning(disable: 4244)

using namespace rfb;

static LogWriter vlog("VNCSConnST");

VNCSConnectionST::VNCSConnectionST(VNCServerST* server_, network::Socket *s,
                                   bool reverse)
  : SConnection(server_->securityFactory, reverse), sock(s), server(server_),
    updates(false), image_getter(server->useEconomicTranslate),
    drawRenderedCursor(false), removeRenderedCursor(false),
    pointerEventTime(0), accessRights(AccessDefault)
{
  setStreams(&sock->inStream(), &sock->outStream());
  peerEndpoint.buf = sock->getPeerEndpoint();
  VNCServerST::connectionsLog.write(1,"accepted: %s", peerEndpoint.buf);

  // Configure the socket
  setSocketTimeouts();
  lastEventTime = time(0);

  // Add this client to the VNCServerST
  server->clients.push_front(this);
}


VNCSConnectionST::~VNCSConnectionST()
{
  // If we reach here then VNCServerST is deleting us!
  VNCServerST::connectionsLog.write(1,"closed: %s (%s)",
                                    peerEndpoint.buf,
                                    (closeReason.buf) ? closeReason.buf : "");

  // Release any keys the client still had pressed
  std::set<rdr::U32>::iterator i;
  for (i=pressedKeys.begin(); i!=pressedKeys.end(); i++)
    server->desktop->keyEvent(*i, false);
  if (server->pointerClient == this)
    server->pointerClient = 0;

  // Remove this client from the server
  server->clients.remove(this);
}


// Methods called from VNCServerST

bool VNCSConnectionST::init()
{
  try {
    initialiseProtocol();
  } catch (rdr::Exception& e) {
    close(e.str());
    return false;
  }
  return true;
}

void VNCSConnectionST::close(const char* reason)
{
  // Log the reason for the close
  if (!closeReason.buf)
    closeReason.buf = strDup(reason);
  else
    vlog.debug("second close: %s (%s)", peerEndpoint.buf, reason);

  // Just shutdown the socket and mark our state as closing.  Eventually the
  // calling code will call VNCServerST's removeSocket() method causing us to
  // be deleted.
  sock->shutdown();
  setState(RFBSTATE_CLOSING);
}


void VNCSConnectionST::processMessages()
{
  if (state() == RFBSTATE_CLOSING) return;
  try {
    // - Now set appropriate socket timeouts and process data
    setSocketTimeouts();
    bool clientsReadyBefore = server->clientsReadyForUpdate();

    while (getInStream()->checkNoWait(1)) {
      processMsg();
    }

    if (!clientsReadyBefore && !requested.is_empty())
      server->desktop->framebufferUpdateRequest();
  } catch (rdr::EndOfStream&) {
    close("Clean disconnection");
  } catch (rdr::Exception &e) {
    close(e.str());
  }
}

void VNCSConnectionST::writeFramebufferUpdateOrClose()
{
  try {
    writeFramebufferUpdate();
  } catch(rdr::Exception &e) {
    close(e.str());
  }
}

void VNCSConnectionST::pixelBufferChange()
{
  try {
    if (!authenticated()) return;
    if (cp.width && cp.height && (server->pb->width() != cp.width ||
                                  server->pb->height() != cp.height))
    {
      // We need to clip the next update to the new size, but also add any
      // extra bits if it's bigger.  If we wanted to do this exactly, something
      // like the code below would do it, but at the moment we just update the
      // entire new size.  However, we do need to clip the renderedCursorRect
      // because that might be added to updates in writeFramebufferUpdate().

      //updates.intersect(server->pb->getRect());
      //
      //if (server->pb->width() > cp.width)
      //  updates.add_changed(Rect(cp.width, 0, server->pb->width(),
      //                           server->pb->height()));
      //if (server->pb->height() > cp.height)
      //  updates.add_changed(Rect(0, cp.height, cp.width,
      //                           server->pb->height()));

      renderedCursorRect = renderedCursorRect.intersect(server->pb->getRect());

      cp.width = server->pb->width();
      cp.height = server->pb->height();
      if (state() == RFBSTATE_NORMAL) {
        if (!writer()->writeSetDesktopSize()) {
          close("Client does not support desktop resize");
          return;
        }
      }
    }
    // Just update the whole screen at the moment because we're too lazy to
    // work out what's actually changed.
    updates.clear();
    updates.add_changed(server->pb->getRect());
    vlog.debug("pixel buffer changed - re-initialising image getter");
    image_getter.init(server->pb, cp.pf(), writer());
    if (writer()->needFakeUpdate())
      writeFramebufferUpdate();
  } catch(rdr::Exception &e) {
    close(e.str());
  }
}

void VNCSConnectionST::setColourMapEntriesOrClose(int firstColour,int nColours)
{
  try {
    setColourMapEntries(firstColour, nColours);
  } catch(rdr::Exception& e) {
    close(e.str());
  }
}

void VNCSConnectionST::bell()
{
  try {
    if (state() == RFBSTATE_NORMAL) writer()->writeBell();
  } catch(rdr::Exception& e) {
    close(e.str());
  }
}

void VNCSConnectionST::serverCutText(const char *str, int len)
{
  try {
    if (!(accessRights & AccessCutText)) return;
    if (!rfb::Server::sendCutText) return;
    if (state() == RFBSTATE_NORMAL)
      writer()->writeServerCutText(str, len);
  } catch(rdr::Exception& e) {
    close(e.str());
  }
}

void VNCSConnectionST::setCursorOrClose()
{
  try {
    setCursor();
  } catch(rdr::Exception& e) {
    close(e.str());
  }
}


int VNCSConnectionST::checkIdleTimeout()
{
  int idleTimeout = rfb::Server::idleTimeout;
  if (idleTimeout == 0) return 0;
  if (state() != RFBSTATE_NORMAL && idleTimeout < 15)
    idleTimeout = 15; // minimum of 15 seconds while authenticating
  time_t now = time(0);
  if (now < lastEventTime) {
    // Someone must have set the time backwards.  Set lastEventTime so that the
    // idleTimeout will count from now.
    vlog.info("Time has gone backwards - resetting idle timeout");
    lastEventTime = now;
  }
  int timeLeft = lastEventTime + idleTimeout - now;
  if (timeLeft < -60) {
    // Our callback is over a minute late - someone must have set the time
    // forwards.  Set lastEventTime so that the idleTimeout will count from
    // now.
    vlog.info("Time has gone forwards - resetting idle timeout");
    lastEventTime = now;
    return secsToMillis(idleTimeout);
  }
  if (timeLeft <= 0) {
    close("Idle timeout");
    return 0;
  }
  return secsToMillis(timeLeft);
}

// renderedCursorChange() is called whenever the server-side rendered cursor
// changes shape or position.  It ensures that the next update will clean up
// the old rendered cursor and if necessary draw the new rendered cursor.

void VNCSConnectionST::renderedCursorChange()
{
  if (state() != RFBSTATE_NORMAL) return;
  removeRenderedCursor = true;
  if (needRenderedCursor())
    drawRenderedCursor = true;
}

// needRenderedCursor() returns true if this client needs the server-side
// rendered cursor.  This may be because it does not support local cursor or
// because the current cursor position has not been set by this client.
// Unfortunately we can't know for sure when the current cursor position has
// been set by this client.  We guess that this is the case when the current
// cursor position is the same as the last pointer event from this client, or
// if it is a very short time since this client's last pointer event (up to a
// second).  [ Ideally we should do finer-grained timing here and make the time
// configurable, but I don't think it's that important. ]

bool VNCSConnectionST::needRenderedCursor()
{
  return (state() == RFBSTATE_NORMAL
          && (!cp.supportsLocalCursor
              || (!server->cursorPos.equals(pointerEventPos) &&
                  (time(0) - pointerEventTime) > 0)));
}


void VNCSConnectionST::approveConnectionOrClose(bool accept,
                                                const char* reason)
{
  try {
    approveConnection(accept, reason);
  } catch (rdr::Exception& e) {
    close(e.str());
  }
}



// -=- Callbacks from SConnection

void VNCSConnectionST::authSuccess()
{
  lastEventTime = time(0);

  server->startDesktop();

  // - Set the connection parameters appropriately
  cp.width = server->pb->width();
  cp.height = server->pb->height();
  cp.setName(server->getName());
  
  // - Set the default pixel format
  cp.setPF(server->pb->getPF());
  char buffer[256];
  cp.pf().print(buffer, 256);
  vlog.info("Server default pixel format %s", buffer);
  image_getter.init(server->pb, cp.pf(), 0);

  // - Mark the entire display as "dirty"
  updates.add_changed(server->pb->getRect());
}

void VNCSConnectionST::queryConnection(const char* userName)
{
  // - Authentication succeeded - clear from blacklist
  CharArray name; name.buf = sock->getPeerAddress();
  server->blHosts->clearBlackmark(name.buf);

  // - Special case to provide a more useful error message
  if (rfb::Server::neverShared && !rfb::Server::disconnectClients &&
    server->authClientCount() > 0) {
    approveConnection(false, "The server is already in use");
    return;
  }

  // - Does the client have the right to bypass the query?
  if (reverseConnection ||
      !(rfb::Server::queryConnect || sock->requiresQuery()) ||
      (accessRights & AccessNoQuery))
  {
    approveConnection(true);
    return;
  }

  // - Get the server to display an Accept/Reject dialog, if required
  //   If a dialog is displayed, the result will be PENDING, and the
  //   server will call approveConnection at a later time
  CharArray reason;
  VNCServerST::queryResult qr = server->queryConnection(sock, userName,
                                                        &reason.buf);
  if (qr == VNCServerST::PENDING)
    return;

  // - If server returns ACCEPT/REJECT then pass result to SConnection
  approveConnection(qr == VNCServerST::ACCEPT, reason.buf);
}

void VNCSConnectionST::clientInit(bool shared)
{
  lastEventTime = time(0);
  if (rfb::Server::alwaysShared || reverseConnection) shared = true;
  if (rfb::Server::neverShared) shared = false;
  if (!shared) {
    if (rfb::Server::disconnectClients) {
      // - Close all the other connected clients
      vlog.debug("non-shared connection - closing clients");
      server->closeClients("Non-shared connection requested", getSock());
    } else {
      // - Refuse this connection if there are existing clients, in addition to
      // this one
      if (server->authClientCount() > 1) {
        close("Server is already in use");
        return;
      }
    }
  }
  SConnection::clientInit(shared);
}

void VNCSConnectionST::setPixelFormat(const PixelFormat& pf)
{
  SConnection::setPixelFormat(pf);
  char buffer[256];
  pf.print(buffer, 256);
  vlog.info("Client pixel format %s", buffer);
  image_getter.init(server->pb, pf, writer());
  setCursor();
}

void VNCSConnectionST::pointerEvent(const Point& pos, int buttonMask)
{
  pointerEventTime = lastEventTime = time(0);
  if (!(accessRights & AccessPtrEvents)) return;
  if (!rfb::Server::acceptPointerEvents) return;
  if (!server->pointerClient || server->pointerClient == this) {
    pointerEventPos = pos;
    if (buttonMask)
      server->pointerClient = this;
    else
      server->pointerClient = 0;
    server->desktop->pointerEvent(pointerEventPos, buttonMask);
  }
}


class VNCSConnectionSTShiftPresser {
public:
  VNCSConnectionSTShiftPresser(SDesktop* desktop_)
    : desktop(desktop_), pressed(false) {}
  ~VNCSConnectionSTShiftPresser() {
    if (pressed) { desktop->keyEvent(XK_Shift_L, false); }
  }
  void press() {
    desktop->keyEvent(XK_Shift_L, true);
    pressed = true;
  }
  SDesktop* desktop;
  bool pressed;
};

// keyEvent() - record in the pressedKeys which keys were pressed.  Allow
// multiple down events (for autorepeat), but only allow a single up event.
void VNCSConnectionST::keyEvent(rdr::U32 key, bool down) {
  lastEventTime = time(0);
  if (!(accessRights & AccessKeyEvents)) return;
  if (!rfb::Server::acceptKeyEvents) return;

  // Remap the key if required
  if (server->keyRemapper)
    key = server->keyRemapper->remapKey(key);

  // Turn ISO_Left_Tab into shifted Tab.
  VNCSConnectionSTShiftPresser shiftPresser(server->desktop);
  if (key == XK_ISO_Left_Tab) {
    if (pressedKeys.find(XK_Shift_L) == pressedKeys.end() &&
        pressedKeys.find(XK_Shift_R) == pressedKeys.end())
      shiftPresser.press();
    key = XK_Tab;
  }

  if (down) {
    pressedKeys.insert(key);
  } else {
    if (!pressedKeys.erase(key)) return;
  }
  server->desktop->keyEvent(key, down);
}

void VNCSConnectionST::clientCutText(const char* str, int len)
{
  if (!(accessRights & AccessCutText)) return;
  if (!rfb::Server::acceptCutText) return;
  server->desktop->clientCutText(str, len);
}

void VNCSConnectionST::framebufferUpdateRequest(const Rect& r,bool incremental)
{
  if (!(accessRights & AccessView)) return;

  SConnection::framebufferUpdateRequest(r, incremental);

  Region reqRgn(r);
  requested.assign_union(reqRgn);

  if (!incremental) {
    // Non-incremental update - treat as if area requested has changed
    updates.add_changed(reqRgn);
    server->comparer->add_changed(reqRgn);
  }

  writeFramebufferUpdate();
}

void VNCSConnectionST::setInitialColourMap()
{
  setColourMapEntries(0, 0);
}

// supportsLocalCursor() is called whenever the status of
// cp.supportsLocalCursor has changed.  If the client does now support local
// cursor, we make sure that the old server-side rendered cursor is cleaned up
// and the cursor is sent to the client.

void VNCSConnectionST::supportsLocalCursor()
{
  if (cp.supportsLocalCursor) {
    removeRenderedCursor = true;
    drawRenderedCursor = false;
    setCursor();
  }
}

void VNCSConnectionST::writeSetCursorCallback()
{
  rdr::U8* transData = writer()->getImageBuf(server->cursor.area());
  image_getter.translatePixels(server->cursor.data, transData,
                               server->cursor.area());

  writer()->writeSetCursor(server->cursor.width(),
                           server->cursor.height(),
                           server->cursor.hotspot,
                           transData, server->cursor.mask.buf);
}


void VNCSConnectionST::writeFramebufferUpdate()
{
  if (state() != RFBSTATE_NORMAL || requested.is_empty()) return;

  server->checkUpdate();

  // If the previous position of the rendered cursor overlaps the source of the
  // copy, then when the copy happens the corresponding rectangle in the
  // destination will be wrong, so add it to the changed region.

  if (!updates.get_copied().is_empty() && !renderedCursorRect.is_empty()) {
    Rect bogusCopiedCursor = (renderedCursorRect.translate(updates.get_delta())
                              .intersect(server->pb->getRect()));
    if (!updates.get_copied().intersect(bogusCopiedCursor).is_empty()) {
      updates.add_changed(bogusCopiedCursor);
    }
  }

  // If we need to remove the old rendered cursor, just add the rectangle to
  // the changed region.

  if (removeRenderedCursor) {
    updates.add_changed(renderedCursorRect);
    renderedCursorRect.clear();
    removeRenderedCursor = false;
  }

  // Return if there is nothing to send the client.

  if (updates.is_empty() && !writer()->needFakeUpdate() && !drawRenderedCursor)
    return;

  // If the client needs a server-side rendered cursor, work out the cursor
  // rectangle.  If it's empty then don't bother drawing it, but if it overlaps
  // with the update region, we need to draw the rendered cursor regardless of
  // whether it has changed.

  if (needRenderedCursor()) {
    renderedCursorRect
      = (server->renderedCursor.getRect(server->renderedCursorTL)
         .intersect(requested.get_bounding_rect()));

    if (renderedCursorRect.is_empty()) {
      drawRenderedCursor = false;
    } else if (!updates.get_changed().union_(updates.get_copied())
               .intersect(renderedCursorRect).is_empty()) {
      drawRenderedCursor = true;
    }

    // We could remove the new cursor rect from updates here.  It's not clear
    // whether this is worth it.  If we do remove it, then we won't draw over
    // the same bit of screen twice, but we have the overhead of a more complex
    // region.

    //if (drawRenderedCursor)
    //  updates.subtract(renderedCursorRect);
  }

  UpdateInfo update;
  updates.enable_copyrect(cp.useCopyRect);
  updates.getUpdateInfo(&update, requested);
  if (!update.is_empty() || writer()->needFakeUpdate() || drawRenderedCursor) {
    int nRects = update.numRects() + (drawRenderedCursor ? 1 : 0);
    writer()->writeFramebufferUpdateStart(nRects);
    Region updatedRegion;
    writer()->writeRects(update, &image_getter, &updatedRegion);
    updates.subtract(updatedRegion);
    if (drawRenderedCursor)
      writeRenderedCursorRect();
    writer()->writeFramebufferUpdateEnd();
    requested.clear();
  }
}


// writeRenderedCursorRect() writes a single rectangle drawing the rendered
// cursor on the client.

void VNCSConnectionST::writeRenderedCursorRect()
{
  image_getter.setPixelBuffer(&server->renderedCursor);
  image_getter.setOffset(server->renderedCursorTL);

  Rect actual;
  writer()->writeRect(renderedCursorRect, &image_getter, &actual);

  image_getter.setPixelBuffer(server->pb);
  image_getter.setOffset(Point(0,0));

  drawRenderedCursor = false;
}

void VNCSConnectionST::setColourMapEntries(int firstColour, int nColours)
{
  if (!readyForSetColourMapEntries) return;
  if (server->pb->getPF().trueColour) return;

  image_getter.setColourMapEntries(firstColour, nColours, writer());

  if (cp.pf().trueColour) {
    updates.add_changed(server->pb->getRect());
  }
}


// setCursor() is called whenever the cursor has changed shape or pixel format.
// If the client supports local cursor then it will arrange for the cursor to
// be sent to the client.

void VNCSConnectionST::setCursor()
{
  if (state() != RFBSTATE_NORMAL || !cp.supportsLocalCursor) return;
  writer()->cursorChange(this);
  if (writer()->needFakeUpdate())
    writeFramebufferUpdate();
}

void VNCSConnectionST::setSocketTimeouts()
{
  int timeoutms = rfb::Server::clientWaitTimeMillis;
  soonestTimeout(&timeoutms, secsToMillis(rfb::Server::idleTimeout));
  if (timeoutms == 0)
    timeoutms = -1;
  sock->inStream().setTimeout(timeoutms);
  sock->outStream().setTimeout(timeoutms);
}
