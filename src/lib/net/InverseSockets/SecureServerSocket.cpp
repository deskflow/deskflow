/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2022 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SecureServerSocket.h"

#include <arch/XArch.h>
#include <base/String.h>
#include <common/constants.h>
#include <deskflow/ArgParser.h>
#include <deskflow/ArgsBase.h>
#include <net/SecureSocket.h>
#include <net/TSocketMultiplexerMethodJob.h>

//
// SecureServerSocket
//
SecureServerSocket::SecureServerSocket(
    IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::EAddressFamily family
)
    : InverseServerSocket(events, socketMultiplexer, family)
{
}

IDataSocket *SecureServerSocket::accept()
{
  SecureSocket *socket = nullptr;

  try {
    socket = new SecureSocket(m_events, m_socketMultiplexer, m_socket.getRawSocket());
    socket->initSsl(true);
    setListeningJob();

    auto certificateFilename = getCertificateFileName();
    if (socket->loadCertificates(certificateFilename)) {
      socket->secureAccept();
    } else {
      delete socket;
      socket = nullptr;
    }
  } catch (const XArchNetwork &) {
    if (socket) {
      delete socket;
      socket = nullptr;
      setListeningJob();
    }
  } catch (const std::exception &) {
    if (socket) {
      delete socket;
      setListeningJob();
    }
    throw;
  }

  return dynamic_cast<IDataSocket *>(socket);
}

std::string SecureServerSocket::getCertificateFileName() const
{
  // if the tls cert option is set use that for the certificate file
  auto certificateFilename = ArgParser::argsBase().m_tlsCertFile;

  if (certificateFilename.empty()) {
    // TODO: Reduce duplication of these strings between here and
    // SecureSocket.cpp
    certificateFilename = deskflow::string::sprintf("%s/tls/%s.pem", ARCH->getProfileDirectory().c_str(), kAppId);
  }

  return certificateFilename;
}
