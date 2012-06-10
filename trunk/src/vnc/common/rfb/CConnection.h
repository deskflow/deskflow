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
// CConnection - class on the client side representing a connection to a
// server.  A derived class should override methods appropriately.
//

#ifndef __RFB_CCONNECTION_H__
#define __RFB_CCONNECTION_H__

#include <rdr/InStream.h>
#include <rdr/OutStream.h>
#include <rfb/CMsgHandler.h>
#include <rfb/util.h>

namespace rfb {

  class CMsgReader;
  class CMsgWriter;
  class CSecurity;
  class IdentityVerifier;

  class CConnection : public CMsgHandler {
  public:

    CConnection();
    virtual ~CConnection();

    // Methods to initialise the connection

    // setServerName() is used to provide a unique(ish) name for the server to
    // which we are connected.  This might be the result of getPeerEndpoint on
    // a TcpSocket, for example, or a host specified by DNS name & port.
    // The serverName is used when verifying the Identity of a host (see RA2).
    void setServerName(const char* name_) { serverName.replaceBuf(strDup(name_)); }

    // setStreams() sets the streams to be used for the connection.  These must
    // be set before initialiseProtocol() and processMsg() are called.  The
    // CSecurity object may call setStreams() again to provide alternative
    // streams over which the RFB protocol is sent (i.e. encrypting/decrypting
    // streams).  Ownership of the streams remains with the caller
    // (i.e. SConnection will not delete them).
    void setStreams(rdr::InStream* is, rdr::OutStream* os);

    // addSecType() should be called once for each security type which the
    // client supports.  The order in which they're added is such that the
    // first one is most preferred.
    void addSecType(rdr::U8 secType);

    // setClientSecTypeOrder() determines whether the client should obey
    // the server's security type preference, by picking the first server security
    // type that the client supports, or whether it should pick the first type
    // that the server supports, from the client-supported list of types.
    void setClientSecTypeOrder(bool clientOrder);

    // setShared sets the value of the shared flag which will be sent to the
    // server upon initialisation.
    void setShared(bool s) { shared = s; }

    // setProtocol3_3 configures whether or not the CConnection should
    // only ever support protocol version 3.3
    void setProtocol3_3(bool s) {useProtocol3_3 = s;}

    // initialiseProtocol() should be called once the streams and security
    // types are set.  Subsequently, processMsg() should be called whenever
    // there is data to read on the InStream.
    void initialiseProtocol();

    // processMsg() should be called whenever there is either:
    // - data available on the underlying network stream
    //   In this case, processMsg may return without processing an RFB message,
    //   if the available data does not result in an RFB message being ready
    //   to handle. e.g. if data is encrypted.
    // NB: This makes it safe to call processMsg() in response to select()
    // - data available on the CConnection's current InStream
    //   In this case, processMsg should always process the available RFB
    //   message before returning.
    // NB: In either case, you must have called initialiseProtocol() first.
    void processMsg();


    // Methods to be overridden in a derived class

    // getCSecurity() gets the CSecurity object for the given type.  The type
    // is guaranteed to be one of the secTypes passed in to addSecType().  The
    // CSecurity object's destroy() method will be called by the CConnection
    // from its destructor.
    virtual CSecurity* getCSecurity(int secType)=0;

    // getCurrentCSecurity() gets the CSecurity instance used for this connection.
    const CSecurity* getCurrentCSecurity() const {return security;} 

    // getIdVerifier() returns the identity verifier associated with the connection.
    // Ownership of the IdentityVerifier is retained by the CConnection instance.
    virtual IdentityVerifier* getIdentityVerifier() {return 0;}

    // authSuccess() is called when authentication has succeeded.
    virtual void authSuccess();

    // serverInit() is called when the ServerInit message is received.  The
    // derived class must call on to CConnection::serverInit().
    virtual void serverInit();


    // Other methods

    // deleteReaderAndWriter() deletes the reader and writer associated with
    // this connection.  This may be useful if you want to delete the streams
    // before deleting the SConnection to make sure that no attempt by the
    // SConnection is made to read or write.
    // XXX Do we really need this at all???
    void deleteReaderAndWriter();

    CMsgReader* reader() { return reader_; }
    CMsgWriter* writer() { return writer_; }

    rdr::InStream* getInStream() { return is; }
    rdr::OutStream* getOutStream() { return os; }

    // Access method used by SSecurity implementations that can verify servers'
    // Identities, to determine the unique(ish) name of the server.
    const char* getServerName() const { return serverName.buf; }

    enum stateEnum {
      RFBSTATE_UNINITIALISED,
      RFBSTATE_PROTOCOL_VERSION,
      RFBSTATE_SECURITY_TYPES,
      RFBSTATE_SECURITY,
      RFBSTATE_SECURITY_RESULT,
      RFBSTATE_INITIALISATION,
      RFBSTATE_NORMAL,
      RFBSTATE_INVALID
    };

    stateEnum state() { return state_; }

  protected:
    void setState(stateEnum s) { state_ = s; }

  private:
    void processVersionMsg();
    void processSecurityTypesMsg();
    void processSecurityMsg();
    void processSecurityResultMsg();
    void processInitMsg();
    void throwAuthFailureException();
    void throwConnFailedException();
    void securityCompleted();

    rdr::InStream* is;
    rdr::OutStream* os;
    CMsgReader* reader_;
    CMsgWriter* writer_;
    bool deleteStreamsWhenDone;
    bool shared;
    CSecurity* security;
    enum { maxSecTypes = 8 };
    int nSecTypes;
    rdr::U8 secTypes[maxSecTypes];
    bool clientSecTypeOrder;
    stateEnum state_;

    CharArray serverName;

    bool useProtocol3_3;
  };
}
#endif
