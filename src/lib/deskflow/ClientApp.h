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

#include "deskflow/App.h"

namespace deskflow {
class Screen;
}
class Event;
class Client;
class NetworkAddress;
class Thread;
class ISocketFactory;

namespace deskflow {
class ClientArgs;
}

class ClientApp : public App
{
public:
  ClientApp(IEventQueue *events, CreateTaskBarReceiverFunc createTaskBarReceiver);
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
  bool loadConfig(const String &pathname) override
  {
    return false;
  }
  int foregroundStartup(int argc, char **argv) override;
  int standardStartup(int argc, char **argv) override;
  int runInner(int argc, char **argv, ILogOutputter *outputter, StartupFunc startup) override;
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
  void updateStatus(const String &msg);
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
  Client *openClient(const String &name, const NetworkAddress &address, deskflow::Screen *screen);
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
