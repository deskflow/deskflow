/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include <QDataStream>
#include <QCoreApplication>

#include "server/ServerAPIListener.h"
#include "server/Server.h"
#include "base/Log.h"
#include "shared/QtHelpers.h"
#include <QTimer>

//
// ServerAPIListener
//

ServerAPIListener::ServerAPIListener() {

  m_worker =
      QThread::create(std::bind(&ServerAPIListener::workerRunnable, this));
  moveToThread(m_worker);
  m_worker->start();

  LOG((CLOG_INFO "API Created"));
}

void ServerAPIListener::workerRunnable() {
  LOG((CLOG_INFO "API Starting"));

  QEventLoop loop;
  m_tcpServer = new QTcpServer(this);

  QHostAddress m_serverAddress{QHostAddress::LocalHost};
  quint16 m_serverPort{24900};
  if (!m_tcpServer->listen(m_serverAddress, m_serverPort)) {
    throw std::runtime_error("Failed to start listening");
  }
  connect(
      m_tcpServer,
      &QTcpServer::newConnection,
      this,
      &ServerAPIListener::onRequest
  );

  LOG((CLOG_INFO "API Listening"));

  loop.exec();
  // QThread::currentThread()->eventDispatcher()
  // QThread::exec();
}

ServerAPIListener::~ServerAPIListener() {
  LOG((CLOG_INFO "API Stopped"));
  dispose();

  qQuitPointer(m_eventLoop);
  qExitPointer(m_worker);

  m_worker->wait(1000);
  m_eventLoop->deleteLater();
  m_worker->deleteLater();
}

void ServerAPIListener::setServer(Server *server) {
  assert(server != nullptr);
  m_server = server;

}

void ServerAPIListener::dispose() {
  m_tcpServer->close();
  delete m_tcpServer;
}

void ServerAPIListener::onRequest() {
  auto clientConnection = m_tcpServer->nextPendingConnection();
  connect(
      clientConnection,
      &QAbstractSocket::disconnected,
      clientConnection,
      &QObject::deleteLater
  );

  auto activeClient = m_server->getActiveClient();
  LOG((CLOG_DEBUG1 "active client: %s", activeClient.c_str()));

  clientConnection->write(activeClient.c_str(), qint64(activeClient.length()));
  clientConnection->flush();
  clientConnection->disconnectFromHost();
}

Server *ServerAPIListener::getServer() {
  return m_server;
}


