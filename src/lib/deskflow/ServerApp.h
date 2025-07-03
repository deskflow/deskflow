/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/Arch.h"
#include "arch/IArchMultithread.h"
#include "base/EventTypes.h"
#include "base/String.h"
#include "common/Constants.h"
#include "deskflow/App.h"
#include "net/NetworkAddress.h"
#include "server/Config.h"

#include <memory>

enum EServerState
{
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

class ServerApp : public App
{
  using ServerConfig = deskflow::server::Config;

public:
  explicit ServerApp(IEventQueue *events);
  ~ServerApp() override = default;

  //
  // IApp overrides
  //

  void parseArgs(int argc, const char *const *argv) override;
  void help() override;
  const char *daemonName() const override;
  const char *daemonInfo() const override;
  void loadConfig() override;
  bool loadConfig(const std::string &pathname) override;
  deskflow::Screen *createScreen() override;
  int mainLoop() override;
  int runInner(int argc, char **argv, StartupFunc startup) override;
  int standardStartup(int argc, char **argv) override;
  int foregroundStartup(int argc, char **argv) override;
  void startNode() override;

  //
  // App overrides
  //

  std::string configSection() const override
  {
    return "server";
  }

  //
  // Regular functions
  //

  void reloadConfig();
  void forceReconnect();
  void resetServer();
  void handleClientConnected(const Event &e, ClientListener *listener);
  void closeServer(Server *server);
  void stopRetryTimer();
  void updateStatus() const;
  void updateStatus(const std::string_view &msg) const;
  void closeClientListener(ClientListener *listen);
  void stopServer();
  void closePrimaryClient(PrimaryClient *primaryClient);
  void closeServerScreen(deskflow::Screen *screen);
  void cleanupServer();
  bool initServer();
  void retryHandler();
  deskflow::Screen *openServerScreen();
  PrimaryClient *openPrimaryClient(const std::string &name, deskflow::Screen *screen);
  void handleScreenError();
  void handleSuspend();
  void handleResume();
  ClientListener *openClientListener(const NetworkAddress &address);
  Server *openServer(ServerConfig &config, PrimaryClient *primaryClient);
  bool startServer();
  Server *getServerPtr()
  {
    return m_server;
  }

  deskflow::ServerArgs &args() const
  {
    return (deskflow::ServerArgs &)argsBase();
  }

  //
  // Static functions
  //

  static void reloadSignalHandler(Arch::ThreadSignal, void *);
  static ServerApp &instance()
  {
    return (ServerApp &)App::instance();
  }

private:
  void handleScreenSwitched() const;
  std::unique_ptr<ISocketFactory> getSocketFactory() const;
  NetworkAddress getAddress(const NetworkAddress &address) const;

  Server *m_server = nullptr;
  EServerState m_serverState = EServerState::kUninitialized;
  deskflow::Screen *m_serverScreen = nullptr;
  PrimaryClient *m_primaryClient = nullptr;
  ClientListener *m_listener = nullptr;
  EventQueueTimer *m_timer = nullptr;
  NetworkAddress *m_deskflowAddress = nullptr;
};
