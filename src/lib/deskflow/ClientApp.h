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
  explicit ClientApp(IEventQueue *events, const QString &processName = QString());
  ~ClientApp() override = default;

  //
  // IApp overrides
  //

  void parseArgs() override;
  const char *daemonName() const override;
  const char *daemonInfo() const override;
  void loadConfig() override
  {
    // do nothing
  }
  bool loadConfig(const std::string &) override
  {
    return false;
  }
  int start() override;
  int runInner(StartupFunc startup) override;
  deskflow::Screen *createScreen() override;
  int mainLoop() override;
  void startNode() override;

  //
  // Regular functions
  //
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
