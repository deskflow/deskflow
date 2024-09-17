/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "arch/Arch.h"
#include "arch/IArchMultithread.h"
#include "base/EventTypes.h"
#include "base/String.h"
#include "net/NetworkAddress.h"
#include "server/Config.h"
#include "deskflow/App.h"

enum EServerState {
  kUninitialized,
  kInitializing,
  kInitializingToStart,
  kInitialized,
  kStarting,
  kStarted
};

class Server;
namespace deskflow {
class Screen;
}
class ClientListener;
class EventQueueTimer;
class ILogOutputter;
class IEventQueue;
class ISocketFactory;

namespace deskflow {
class ServerArgs;
}

class ServerApp : public App {
  using ServerConfig = deskflow::server::Config;

public:
  ServerApp(
      IEventQueue *events, CreateTaskBarReceiverFunc createTaskBarReceiver);
  virtual ~ServerApp();

  //
  // IApp overrides
  //

  void parseArgs(int argc, const char *const *argv) override;
  void help() override;
  const char *daemonName() const override;
  const char *daemonInfo() const override;
  void loadConfig() override;
  bool loadConfig(const String &pathname) override;
  deskflow::Screen *createScreen() override;
  int mainLoop() override;
  int runInner(
      int argc, char **argv, ILogOutputter *outputter,
      StartupFunc startup) override;
  int standardStartup(int argc, char **argv) override;
  int foregroundStartup(int argc, char **argv) override;
  void startNode() override;

  //
  // App overrides
  //

  std::string configSection() const override { return "server"; }

  //
  // Regular functions
  //

  void reloadConfig(const Event &, void *);
  void forceReconnect(const Event &, void *);
  void resetServer(const Event &, void *);
  void handleClientConnected(const Event &, void *vlistener);
  void handleClientsDisconnected(const Event &, void *);
  void closeServer(Server *server);
  void stopRetryTimer();
  void updateStatus();
  void updateStatus(const String &msg);
  void closeClientListener(ClientListener *listen);
  void stopServer();
  void closePrimaryClient(PrimaryClient *primaryClient);
  void closeServerScreen(deskflow::Screen *screen);
  void cleanupServer();
  bool initServer();
  void retryHandler(const Event &, void *);
  deskflow::Screen *openServerScreen();
  PrimaryClient *openPrimaryClient(const String &name, deskflow::Screen *screen);
  void handleScreenError(const Event &, void *);
  void handleSuspend(const Event &, void *);
  void handleResume(const Event &, void *);
  ClientListener *openClientListener(const NetworkAddress &address);
  Server *openServer(ServerConfig &config, PrimaryClient *primaryClient);
  void handleNoClients(const Event &, void *);
  bool startServer();
  Server *getServerPtr() { return m_server; }

  deskflow::ServerArgs &args() const {
    return (deskflow::ServerArgs &)argsBase();
  }

  //
  // Static functions
  //

  static void reloadSignalHandler(Arch::ESignal, void *);
  static ServerApp &instance() { return (ServerApp &)App::instance(); }

private:
  void handleScreenSwitched(const Event &, void *data);
  ISocketFactory *getSocketFactory() const;
  NetworkAddress getAddress(const NetworkAddress &address) const;

  Server *m_server;
  EServerState m_serverState;
  deskflow::Screen *m_serverScreen;
  PrimaryClient *m_primaryClient;
  ClientListener *m_listener;
  EventQueueTimer *m_timer;
  NetworkAddress *m_deskflowAddress;
};

// configuration file name
#if SYSAPI_WIN32
#define USER_CONFIG_NAME "deskflow.sgc"
#define SYSTEM_CONFIG_NAME "deskflow.sgc"
#elif SYSAPI_UNIX
#define USER_CONFIG_NAME ".deskflow.conf"
#define SYSTEM_CONFIG_NAME "deskflow.conf"
#endif