/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SecureListenSocket.h"

#include "SecureSocket.h"
#include "arch/ArchException.h"
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
    IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::AddressFamily family,
    SecurityLevel securityLevel
)
    : TCPListenSocket(events, socketMultiplexer, family),
      m_securityLevel{securityLevel}

{
  // do nothing
}

std::unique_ptr<IDataSocket> SecureListenSocket::accept()
{
  std::unique_ptr<SecureSocket> secureSocket;
  try {
    secureSocket = std::make_unique<SecureSocket>(
        events(), socketMultiplexer(), ARCH->acceptSocket(socket(), nullptr), m_securityLevel
    );
    secureSocket->initSsl(true);

    setListeningJob();

    // default location of the TLS cert file in users dir
    if (const auto certificateFilename = Settings::value(Settings::Security::Certificate).toString().toStdString();
        !secureSocket->loadCertificates(certificateFilename)) {
      return nullptr;
    }

    secureSocket->secureAccept();

    return secureSocket;
  } catch (ArchNetworkException &) {
    if (secureSocket) {
      setListeningJob();
    }
    return nullptr;
  } catch (std::exception &ex) {
    if (secureSocket) {
      setListeningJob();
    }
    throw ex;
  }
}
