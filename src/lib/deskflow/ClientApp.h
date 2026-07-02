/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/App.h"
#include "coordination/RelayKeyEvent.h"
#include "net/NetworkAddress.h"

#include <QStringList>

namespace deskflow {
class Screen;
class ClientArgs;
} // namespace deskflow

class Event;
class Client;
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
  void loadConfig() override
  {
    // do nothing
  }
  bool loadConfig(const QString &) override
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
  void cancelClientRestart();
  void handleClientConnected();
  void handleClientFailed(const Event &e);
  void handleClientRefused(const Event &e);
  void handleClientDisconnected();
  Client *openClient(const std::string &name, const NetworkAddress &address, deskflow::Screen *screen);
  void closeClient(Client *client);
  bool startClient();
  void stopClient();
  //! Mesh v2: add server addresses for fleet pre-connect (deduped).
  void appendPreConnectHosts(const QStringList &hosts);
  //! Inject a fleet-relayed key onto the local client screen (mesh v2 cursor host).
  void injectRelayedKey(const deskflow::coordination::RelayKeyEvent &event);
  void registerKeyForwardHandler();
  void unregisterKeyForwardHandler();
  void handleCoordinationKeyForward(const Event &event);
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

  /**
   * @brief retryTime
   * @return next retry time based on number of current retries
   */
  double retryTime() const;

private:
  ISocketFactory *getSocketFactory() const;
  NetworkAddress &getCurrentServerAddress();
  void tryNextServer();

  bool m_suspended = false;
  Client *m_client = nullptr;
  deskflow::Screen *m_clientScreen = nullptr;
  // The pending reconnect timer, owned by the shared EventQueue. Tracked so
  // it can be cancelled when the client stops -- in auto mode the queue
  // outlives this ClientApp across role epochs, and a leaked timer would
  // fire its `this`-bound handler into the next epoch (use-after-free).
  EventQueueTimer *m_clientRestartTimer = nullptr;
  QList<NetworkAddress> m_serverAddresses;
  size_t m_currentServerIndex = 0;
  size_t m_lastServerAddressIndex = 0;
  uint m_retryCount = 0;
  bool m_keyForwardHandlerRegistered = false;
};
