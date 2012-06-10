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
#include <stdio.h>
#include <string.h>
#include <rfb/Exception.h>
#include <rfb/CMsgReaderV3.h>
#include <rfb/CMsgWriterV3.h>
#include <rfb/CSecurity.h>
#include <rfb/secTypes.h>
#include <rfb/CConnection.h>
#include <rfb/util.h>

#include <rfb/LogWriter.h>

using namespace rfb;

static LogWriter vlog("CConnection");

CConnection::CConnection()
  : is(0), os(0), reader_(0), writer_(0),
    shared(false), security(0), nSecTypes(0), clientSecTypeOrder(false),
    state_(RFBSTATE_UNINITIALISED), useProtocol3_3(false)
{
}

CConnection::~CConnection()
{
  if (security) security->destroy();
  deleteReaderAndWriter();
}

void CConnection::deleteReaderAndWriter()
{
  delete reader_;
  reader_ = 0;
  delete writer_;
  writer_ = 0;
}

void CConnection::setStreams(rdr::InStream* is_, rdr::OutStream* os_)
{
  is = is_;
  os = os_;
}

void CConnection::addSecType(rdr::U8 secType)
{
  if (nSecTypes == maxSecTypes)
    throw Exception("too many security types");
  secTypes[nSecTypes++] = secType;
}

void CConnection::setClientSecTypeOrder(bool clientOrder) {
  clientSecTypeOrder = clientOrder;
}

void CConnection::initialiseProtocol()
{
  state_ = RFBSTATE_PROTOCOL_VERSION;
}

void CConnection::processMsg()
{
  switch (state_) {

  case RFBSTATE_PROTOCOL_VERSION: processVersionMsg();       break;
  case RFBSTATE_SECURITY_TYPES:   processSecurityTypesMsg(); break;
  case RFBSTATE_SECURITY:         processSecurityMsg();      break;
  case RFBSTATE_SECURITY_RESULT:  processSecurityResultMsg(); break;
  case RFBSTATE_INITIALISATION:   processInitMsg();          break;
  case RFBSTATE_NORMAL:           reader_->readMsg();        break;
  case RFBSTATE_UNINITIALISED:
    throw Exception("CConnection::processMsg: not initialised yet?");
  default:
    throw Exception("CConnection::processMsg: invalid state");
  }
}

void CConnection::processVersionMsg()
{
  vlog.debug("reading protocol version");
  bool done;
  if (!cp.readVersion(is, &done)) {
    state_ = RFBSTATE_INVALID;
    throw Exception("reading version failed: not an RFB server?");
  }
  if (!done) return;

  vlog.info("Server supports RFB protocol version %d.%d",
            cp.majorVersion, cp.minorVersion);

  // The only official RFB protocol versions are currently 3.3, 3.7 and 3.8
  if (cp.beforeVersion(3,3)) {
    char msg[256];
    sprintf(msg,"Server gave unsupported RFB protocol version %d.%d",
            cp.majorVersion, cp.minorVersion);
    vlog.error(msg);
    state_ = RFBSTATE_INVALID;
    throw Exception(msg);
  } else if (useProtocol3_3 || cp.beforeVersion(3,7)) {
    cp.setVersion(3,3);
  } else if (cp.afterVersion(3,8)) {
    cp.setVersion(3,8);
  }

  cp.writeVersion(os);
  state_ = RFBSTATE_SECURITY_TYPES;

  vlog.info("Using RFB protocol version %d.%d",
            cp.majorVersion, cp.minorVersion);
}


void CConnection::processSecurityTypesMsg()
{
  vlog.debug("processing security types message");

  int secType = secTypeInvalid;

  if (cp.isVersion(3,3)) {

    // legacy 3.3 server may only offer "vnc authentication" or "none"

    secType = is->readU32();
    if (secType == secTypeInvalid) {
      throwConnFailedException();

    } else if (secType == secTypeNone || secType == secTypeVncAuth) {
      int j;
      for (j = 0; j < nSecTypes; j++)
        if (secTypes[j] == secType) break;
      if (j == nSecTypes)
        secType = secTypeInvalid;
    } else {
      vlog.error("Unknown 3.3 security type %d", secType);
      throw Exception("Unknown 3.3 security type");
    }

  } else {

    // >=3.7 server will offer us a list

    int nServerSecTypes = is->readU8();
    if (nServerSecTypes == 0)
      throwConnFailedException();

    int secTypePos = nSecTypes;
    for (int i = 0; i < nServerSecTypes; i++) {
      rdr::U8 serverSecType = is->readU8();
      vlog.debug("Server offers security type %s(%d)",
                 secTypeName(serverSecType),serverSecType);

      // If we haven't already chosen a secType, try this one
      // If we are using the client's preference for types,
      // we keep trying types, to find the one that matches and
      // which appears first in the client's list of supported types.
      if (secType == secTypeInvalid || clientSecTypeOrder) {
        for (int j = 0; j < nSecTypes; j++) {
          if (secTypes[j] == serverSecType && j < secTypePos) {
            secType = secTypes[j];
            secTypePos = j;
            break;
          }
        }
        // NB: Continue reading the remaining server secTypes, but ignore them
      }
    }

    // Inform the server of our decision
    if (secType != secTypeInvalid) {
      os->writeU8(secType);
      os->flush();
      vlog.debug("Choosing security type %s(%d)",secTypeName(secType),secType);
    }
  }

  if (secType == secTypeInvalid) {
    state_ = RFBSTATE_INVALID;
    vlog.error("No matching security types");
    throw Exception("No matching security types");
  }

  state_ = RFBSTATE_SECURITY;
  security = getCSecurity(secType);
  processSecurityMsg();
}

void CConnection::processSecurityMsg()
{
  vlog.debug("processing security message");
  if (security->processMsg(this)) {
    state_ = RFBSTATE_SECURITY_RESULT;
    processSecurityResultMsg();
  }
}

void CConnection::processSecurityResultMsg()
{
  vlog.debug("processing security result message");
  int result;
  if (cp.beforeVersion(3,8) && security->getType() == secTypeNone) {
    result = secResultOK;
  } else {
    if (!is->checkNoWait(1)) return;
    result = is->readU32();
  }
  switch (result) {
  case secResultOK:
    securityCompleted();
    return;
  case secResultFailed:
    vlog.debug("auth failed");
    break;
  case secResultTooMany:
    vlog.debug("auth failed - too many tries");
    break;
  default:
    throw Exception("Unknown security result from server");
  }
  CharArray reason;
  if (cp.beforeVersion(3,8))
    reason.buf = strDup("Authentication failure");
  else
    reason.buf = is->readString();
  state_ = RFBSTATE_INVALID;
  throw AuthFailureException(reason.buf);
}

void CConnection::processInitMsg()
{
  vlog.debug("reading server initialisation");
  reader_->readServerInit();
}

void CConnection::throwConnFailedException()
{
  state_ = RFBSTATE_INVALID;
  CharArray reason;
  reason.buf = is->readString();
  throw ConnFailedException(reason.buf);
}

void CConnection::securityCompleted()
{
  state_ = RFBSTATE_INITIALISATION;
  reader_ = new CMsgReaderV3(this, is);
  writer_ = new CMsgWriterV3(&cp, os);
  vlog.debug("Authentication success!");
  authSuccess();
  writer_->writeClientInit(shared);
}

void CConnection::authSuccess()
{
}

void CConnection::serverInit()
{
  state_ = RFBSTATE_NORMAL;
  vlog.debug("initialisation done");
}
