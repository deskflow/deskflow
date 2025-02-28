/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/App.h"

namespace deskflow {
class Screen;
class ClientArgs;
} // namespace deskflow

class Event;
class Client;
class NetworkAddress;
class Thread;
class ISocketFactory;

class ClientApp : public App
{
public:
  ClientApp(IEventQueue *events);
  virtual ~ClientApp();

  //
  // IApp overrides
  //

  void parseArgs(int argc, const char *const *argv) override;
  void help() override;
  const char *daemonName() const override;
  const char *daemonInfo() const override;
  void loadConfig() override
  {
  }
  bool loadConfig(const std::string &pathname) override
  {
    return false;
  }
  int foregroundStartup(int argc, char **argv) override;
  int standardStartup(int argc, char **argv) override;
  int runInner(int argc, char **argv, StartupFunc startup) override;
  deskflow::Screen *createScreen() override;
  int mainLoop() override;
  void startNode() override;

  //
  // App overrides
  //

  std::string configSection() const override
  {
    return "client";
  }

  //
  // Regular functions
  //

  void updateStatus();
  void updateStatus(const std::string &msg);
  void resetRestartTimeout();
  double nextRestartTimeout();
  void handleScreenError(const Event &, void *);
  deskflow::Screen *openClientScreen();
  void closeClientScreen(deskflow::Screen *screen);
  void handleClientRestart(const Event &, void *vtimer);
  void scheduleClientRestart(double retryTime);
  void handleClientConnected(const Event &, void *);
  void handleClientFailed(const Event &e, void *);
  void handleClientRefused(const Event &e, void *);
  void handleClientDisconnected(const Event &, void *);
  Client *openClient(const std::string &name, const NetworkAddress &address, deskflow::Screen *screen);
  void closeClient(Client *client);
  bool startClient();
  void stopClient();
  Client *getClientPtr()
  {
    return m_client;
  }

  deskflow::ClientArgs &args() const
  {
    return (deskflow::ClientArgs &)argsBase();
  }

  //
  // Static functions
  //

  static ClientApp &instance()
  {
    return (ClientApp &)App::instance();
  }

private:
  ISocketFactory *getSocketFactory() const;

  Client *m_client;
  deskflow::Screen *m_clientScreen;
  NetworkAddress *m_serverAddress;
  size_t m_lastServerAddressIndex = 0;
};
