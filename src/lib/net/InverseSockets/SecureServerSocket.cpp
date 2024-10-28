/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2022 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "SecureServerSocket.h"

#include <arch/XArch.h>
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
    certificateFilename = deskflow::string::sprintf("%s/tls/deskflow.pem", ARCH->getProfileDirectory().c_str());
  }

  return certificateFilename;
}
