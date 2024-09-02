/*
 * synergy -- mouse and keyboard sharing utility
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
#include "synergy/App.h"

enum EServerState {
  kUninitialized,
  kInitializing,
  kInitializingToStart,
  kInitialized,
  kStarting,
  kStarted
};

class Server;
namespace synergy {
class Screen;
}
class ClientListener;
class EventQueueTimer;
class ILogOutputter;
class IEventQueue;
class ISocketFactory;

namespace synergy {
class ServerArgs;
}

class ServerApp : public App {
  using ServerConfig = synergy::server::Config;

public:
  ServerApp(
      IEventQueue *events, CreateTaskBarReceiverFunc createTaskBarReceiver);
  virtual ~ServerApp();

  void parseArgs(int argc, const char *const *argv) override;
  void help() override;
  const char *daemonName() const override;
  const char *daemonInfo() const override;
  void loadConfig() override;
  bool loadConfig(const String &pathname) override;
  synergy::Screen *createScreen() override;
  int mainLoop() override;
  int runInner(
      int argc, char **argv, ILogOutputter *outputter,
      StartupFunc startup) override;
  int standardStartup(int argc, char **argv) override;
  int foregroundStartup(int argc, char **argv) override;
  void startNode() override;
  std::string configSection() const override { return "server"; }

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
  void closeServerScreen(synergy::Screen *screen);
  void cleanupServer();
  bool initServer();
  void retryHandler(const Event &, void *);
  synergy::Screen *openServerScreen();
  PrimaryClient *openPrimaryClient(const String &name, synergy::Screen *screen);
  void handleScreenError(const Event &, void *);
  void handleSuspend(const Event &, void *);
  void handleResume(const Event &, void *);
  ClientListener *openClientListener(const NetworkAddress &address);
  Server *openServer(ServerConfig &config, PrimaryClient *primaryClient);
  void handleNoClients(const Event &, void *);
  bool startServer();
  Server *getServerPtr() { return m_server; }

  synergy::ServerArgs &args() const {
    return (synergy::ServerArgs &)argsBase();
  }

  static void reloadSignalHandler(Arch::ESignal, void *);
  static ServerApp &instance() { return (ServerApp &)App::instance(); }

private:
  void handleScreenSwitched(const Event &, void *data);
  ISocketFactory *getSocketFactory() const;
  NetworkAddress getAddress(const NetworkAddress &address) const;

  Server *m_server;
  EServerState m_serverState;
  synergy::Screen *m_serverScreen;
  PrimaryClient *m_primaryClient;
  ClientListener *m_listener;
  EventQueueTimer *m_timer;
  NetworkAddress *m_synergyAddress;
};

// configuration file name
#if SYSAPI_WIN32
#define USR_CONFIG_NAME "synergy.sgc"
#define SYS_CONFIG_NAME "synergy.sgc"
#elif SYSAPI_UNIX
#define USR_CONFIG_NAME ".synergy.conf"
#define SYS_CONFIG_NAME "synergy.conf"
#endif
