/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
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
  explicit ClientApp(IEventQueue *events);
  ~ClientApp() override = default;

  //
  // IApp overrides
  //

  void parseArgs(int argc, const char *const *argv) override;
  void help() override;
  const char *daemonName() const override;
  const char *daemonInfo() const override;
  void loadConfig() override
  {
    // do nothing
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

  void updateStatus() const;
  void updateStatus(const std::string_view &) const;
  deskflow::Screen *openClientScreen();
  void closeClientScreen(deskflow::Screen *screen);
  void handleClientRestart(const Event &, EventQueueTimer *vtimer);
  void scheduleClientRestart(double retryTime);
  void handleClientConnected() const;
  void handleClientFailed(const Event &e);
  void handleClientRefused(const Event &e);
  void handleClientDisconnected();
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

  bool m_suspended = false;
  Client *m_client = nullptr;
  deskflow::Screen *m_clientScreen = nullptr;
  NetworkAddress *m_serverAddress = nullptr;
  size_t m_lastServerAddressIndex = 0;
};
