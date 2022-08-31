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

#pragma once

#include <thread>
#include <QObject>
#include <QThread>
#include <QEventLoop>
#include <QCoreApplication>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <httplib.h>

// #include "server/Config.h"

class Server;

class ServerAPIListener : public QObject {
Q_OBJECT
public:
  // The factories are adopted.
  ServerAPIListener();

  ~ServerAPIListener() override;

  // ServerAPIListener& operator=(ServerAPIListener const &) =delete;
  // ServerAPIListener& operator=(ServerAPIListener &&) =delete;

  //! @name manipulators
  //@{

  void setServer(Server *server);

  //@}

  //! @name accessors
  //@{

  //! Get next connected client
  /*!
  Returns the next connected client and removes it from the internal
  list.  The client is responsible for deleting the returned client.
  Returns NULL if no clients are available.
  */

  //! Get server which owns this listener
  Server *getServer();

  //@}
  // client connection event handlers

  void dispose();

private slots:

  void onRequest();


private:
  void workerRunnable();

  QTcpServer *m_tcpServer{nullptr};
  Server *m_server{nullptr};
  QEventLoop * m_eventLoop{nullptr};
  QThread * m_worker{nullptr};
  // private:
  //     // client connection event handlers
  //
  //     // void                dispose();
  //     QTcpServer*            m_tcpServer{nullptr};
  //     Server*                m_server{nullptr};
  // QHostAddress m_serverAddress{QHostAddress::LocalHost};
  // quint16 m_serverPort{24900};
};


