/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SecureListenSocket.h"

#include "SecureSocket.h"
#include "arch/XArch.h"
#include "base/String.h"
#include "common/constants.h"
#include "deskflow/ArgParser.h"
#include "deskflow/ArgsBase.h"
#include "net/NetworkAddress.h"
#include "net/SocketMultiplexer.h"
#include "net/TSocketMultiplexerMethodJob.h"

// TODO: Reduce duplication of these strings between here and SecureSocket.cpp
static const char s_certificateDir[] = {"tls"};
static const char s_certificateFileExt[] = {"pem"};

//
// SecureListenSocket
//

SecureListenSocket::SecureListenSocket(
    IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::EAddressFamily family,
    SecurityLevel securityLevel
)
    : TCPListenSocket(events, socketMultiplexer, family),
      m_securityLevel{securityLevel}

{
}

IDataSocket *SecureListenSocket::accept()
{
  SecureSocket *socket = NULL;
  try {
    socket = new SecureSocket(m_events, m_socketMultiplexer, ARCH->acceptSocket(m_socket, NULL), m_securityLevel);
    socket->initSsl(true);

    if (socket != NULL) {
      setListeningJob();
    }

    // default location of the TLS cert file in users dir
    std::string certificateFilename = deskflow::string::sprintf(
        "%s/%s/%s.%s", ARCH->getProfileDirectory().c_str(), s_certificateDir, kAppId, s_certificateFileExt
    );

    // if the tls cert option is set use that for the certificate file
    if (!ArgParser::argsBase().m_tlsCertFile.empty()) {
      certificateFilename = ArgParser::argsBase().m_tlsCertFile;
    }

    bool loaded = socket->loadCertificates(certificateFilename);
    if (!loaded) {
      delete socket;
      return NULL;
    }

    socket->secureAccept();

    return dynamic_cast<IDataSocket *>(socket);
  } catch (XArchNetwork &) {
    if (socket != NULL) {
      delete socket;
      setListeningJob();
    }
    return NULL;
  } catch (std::exception &ex) {
    if (socket != NULL) {
      delete socket;
      setListeningJob();
    }
    throw ex;
  }
}
