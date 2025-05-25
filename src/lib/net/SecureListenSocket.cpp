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
#include "common/Settings.h"
#include "deskflow/ArgParser.h"
#include "deskflow/ArgsBase.h"
#include "net/NetworkAddress.h"
#include "net/SocketMultiplexer.h"
#include "net/TSocketMultiplexerMethodJob.h"

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
  // do nothing
}

std::unique_ptr<IDataSocket> SecureListenSocket::accept()
{
  std::unique_ptr<SecureSocket> socket;
  try {
    socket = std::make_unique<SecureSocket>(
        m_events, m_socketMultiplexer, ARCH->acceptSocket(m_socket, nullptr), m_securityLevel
    );
    socket->initSsl(true);

    setListeningJob();

    // default location of the TLS cert file in users dir
    std::string certificateFilename = Settings::value(Settings::Security::Certificate).toString().toStdString();

    // if the tls cert option is set use that for the certificate file
    if (!ArgParser::argsBase().m_tlsCertFile.empty()) {
      certificateFilename = ArgParser::argsBase().m_tlsCertFile;
    }

    if (!socket->loadCertificates(certificateFilename)) {
      return nullptr;
    }

    socket->secureAccept();

    return socket;
  } catch (XArchNetwork &) {
    if (socket) {
      setListeningJob();
    }
    return nullptr;
  } catch (std::exception &ex) {
    if (socket) {
      setListeningJob();
    }
    throw ex;
  }
}
