/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ServerApp.h"

#include "arch/Arch.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "common/ExitCodes.h"
#include "common/PlatformInfo.h"
#include "common/Settings.h"
#include "deskflow/App.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/Screen.h"
#include "deskflow/ScreenException.h"
#include "net/SocketException.h"
#include "net/SocketMultiplexer.h"
#include "net/TCPSocketFactory.h"
#include "server/ClientListener.h"
#include "server/ClientProxy.h"
#include "server/Config.h"
#include "server/PrimaryClient.h"
#include "server/Server.h"

// must be before screen header includes
#include <QFileInfo>

#if SYSAPI_WIN32
#include "arch/win32/ArchDaemonWindows.h"
#endif

#if WINAPI_MSWINDOWS
#include "platform/MSWindowsScreen.h"
#endif

#if WINAPI_XWINDOWS
#include "platform/XWindowsScreen.h"
#endif

#if WINAPI_LIBEI
#include "platform/EiScreen.h"
#endif

#if WINAPI_CARBON
#include "base/TMethodJob.h"
#include "mt/Thread.h"
#include "platform/OSXCocoaApp.h"
#include "platform/OSXScreen.h"
#endif

#include <fstream>

using namespace deskflow::server;

//
// ServerApp
//

ServerApp::ServerApp(IEventQueue *events, const QString &processName) : App(events, processName)
{
  m_name = Settings::value(Settings::Core::ScreenName).toString().toStdString();
  // do nothing
}

void ServerApp::parseArgs()
{
  if (const auto address = Settings::value(Settings::Core::Interface).toString(); !address.isEmpty()) {
    *m_deskflowAddress = NetworkAddress(address.toStdString(), Settings::value(Settings::Core::Port).toInt());
  } else {
    *m_deskflowAddress = NetworkAddress(Settings::value(Settings::Core::Port).toInt());
  }

  try {
    m_deskflowAddress->resolve();
  } catch (SocketAddressException &e) {
    LOG_CRIT("%s: %s" BYE, qPrintable(processName()), e.what(), qPrintable(processName()));
    bye(s_exitArgs);
  }
}

void ServerApp::reloadSignalHandler(Arch::ThreadSignal, void *)
{
  IEventQueue *events = App::instance().getEvents();
  events->addEvent(Event(EventTypes::ServerAppReloadConfig, events->getSystemTarget()));
}

QString ServerApp::currentConfig() const
{
  bool useExt = Settings::value(Settings::Server::ExternalConfig).toBool();
  return useExt ? Settings::value(Settings::Server::ExternalConfigFile).toString()
                : Settings::defaultValue(Settings::Server::ExternalConfigFile).toString();
}

void ServerApp::reloadConfig()
{
  LOG_DEBUG("reload configuration");
  if (loadConfig(currentConfig())) {
    if (m_server != nullptr) {
      m_server->setConfig(*m_config);
    }
    LOG_NOTE("reloaded configuration");
  }
}

void ServerApp::loadConfig()
{
  const auto path = currentConfig();
  if (path.isEmpty()) {
    LOG_CRIT("no configuration path provided");
    bye(s_exitConfig);
  }

  if (!loadConfig(path)) {
    LOG_CRIT("%s: failed to load config: %s", qPrintable(processName()), path.toStdString().c_str());
    bye(s_exitConfig);
  }
}

bool ServerApp::loadConfig(const QString &filename)
{
  const auto path = filename.toStdString();
  try {
    // load configuration
    LOG_DEBUG("opening configuration \"%s\"", path.c_str());
#ifdef SYSAPI_WIN32
    std::ifstream configStream(filename.toStdWString());
#else
    std::ifstream configStream(path);
#endif
    if (!configStream.is_open()) {
      LOG_ERR("cannot open configuration \"%s\"", path.c_str());
      return false;
    }
    configStream >> *m_config;
    LOG_DEBUG("configuration read successfully");
    return true;
  } catch (ServerConfigReadException &e) {
    // report error in configuration file
    LOG_ERR("cannot read configuration \"%s\": %s", path.c_str(), e.what());
  }
  return false;
}

void ServerApp::forceReconnect()
{
  if (m_server != nullptr) {
    m_server->disconnect();
  }
}

void ServerApp::handleClientConnected(const Event &, ClientListener *listener)
{
  ClientProxy *client = listener->getNextClient();
  if (client != nullptr) {
    m_server->adoptClient(client);
  }
}

void ServerApp::closeServer(Server *server)
{
  if (server == nullptr) {
    return;
  }

  // tell all clients to disconnect
  server->disconnect();

  // wait for clients to disconnect for up to timeout seconds
  double timeout = 3.0;
  EventQueueTimer *timer = getEvents()->newOneShotTimer(timeout, nullptr);
  getEvents()->addHandler(EventTypes::Timer, timer, [this](const auto &) {
    getEvents()->addEvent(Event(EventTypes::Quit));
  });
  getEvents()->addHandler(EventTypes::ServerDisconnected, server, [this](const auto &) {
    getEvents()->addEvent(Event(EventTypes::Quit));
  });

  getEvents()->loop();

  getEvents()->removeHandler(EventTypes::Timer, timer);
  getEvents()->deleteTimer(timer);
  getEvents()->removeHandler(EventTypes::ServerDisconnected, server);

  // done with server
  delete server;
}

void ServerApp::stopRetryTimer()
{
  if (m_timer != nullptr) {
    getEvents()->removeHandler(EventTypes::Timer, m_timer);
    getEvents()->deleteTimer(m_timer);
    m_timer = nullptr;
  }
}

void ServerApp::closeClientListener(ClientListener *listen)
{
  if (listen != nullptr) {
    getEvents()->removeHandler(EventTypes::ClientListenerAccepted, listen);
    delete listen;
  }
}

void ServerApp::stopServer()
{
  using enum ServerState;
  if (m_serverState == Started) {
    closeServer(m_server);
    closeClientListener(m_listener);
    m_server = nullptr;
    m_listener = nullptr;
    m_serverState = Initialized;
  } else if (m_serverState == Starting) {
    stopRetryTimer();
    m_serverState = Initialized;
  }
  assert(m_server == nullptr);
  assert(m_listener == nullptr);
}

void ServerApp::closePrimaryClient(PrimaryClient *primaryClient)
{
  delete primaryClient;
}

void ServerApp::closeServerScreen(deskflow::Screen *screen)
{
  if (screen != nullptr) {
    using enum EventTypes;
    getEvents()->removeHandler(ScreenError, screen->getEventTarget());
    getEvents()->removeHandler(ScreenSuspend, screen->getEventTarget());
    getEvents()->removeHandler(ScreenResume, screen->getEventTarget());
    delete screen;
  }
}

void ServerApp::cleanupServer()
{
  using enum ServerState;
  stopServer();
  if (m_serverState == Initialized) {
    closePrimaryClient(m_primaryClient);
    closeServerScreen(m_serverScreen);
    m_primaryClient = nullptr;
    m_serverScreen = nullptr;
    m_serverState = Uninitialized;
  } else if (m_serverState == Initializing || m_serverState == InitializingToStart) {
    stopRetryTimer();
    m_serverState = Uninitialized;
  }
  assert(m_primaryClient == nullptr);
  assert(m_serverScreen == nullptr);
  assert(m_serverState == Uninitialized);
}

void ServerApp::retryHandler()
{
  // discard old timer
  assert(m_timer != nullptr);
  stopRetryTimer();

  // try initializing/starting the server again
  switch (m_serverState) {
    using enum ServerState;
  case Uninitialized:
  case Initialized:
  case Started:
    assert(0 && "bad internal server state");
    break;

  case Initializing:
    LOG_DEBUG1("retry server initialization");
    m_serverState = Uninitialized;
    if (!initServer()) {
      getEvents()->addEvent(Event(EventTypes::Quit));
    }
    break;

  case InitializingToStart:
    LOG_DEBUG1("retry server initialization");
    m_serverState = Uninitialized;
    if (!initServer()) {
      getEvents()->addEvent(Event(EventTypes::Quit));
    } else if (m_serverState == Initialized) {
      LOG_DEBUG1("starting server");
      if (!startServer()) {
        getEvents()->addEvent(Event(EventTypes::Quit));
      }
    }
    break;

  case Starting:
    LOG_DEBUG1("retry starting server");
    m_serverState = Initialized;
    if (!startServer()) {
      getEvents()->addEvent(Event(EventTypes::Quit));
    }
    break;
  }
}

bool ServerApp::initServer()
{
  using enum ServerState;
  // skip if already initialized or initializing
  if (m_serverState != Uninitialized) {
    return true;
  }

  double retryTime;
  deskflow::Screen *serverScreen = nullptr;
  PrimaryClient *primaryClient = nullptr;
  try {
    std::string name = m_config->getCanonicalName(m_name);
    serverScreen = openServerScreen();
    primaryClient = openPrimaryClient(name, serverScreen);
    m_serverScreen = serverScreen;
    m_primaryClient = primaryClient;
    m_serverState = Initialized;
    return true;
  } catch (ScreenUnavailableException &e) {
    LOG_WARN("primary screen unavailable: %s", e.what());
    closePrimaryClient(primaryClient);
    closeServerScreen(serverScreen);
    retryTime = e.getRetryTime();
  } catch (ScreenOpenFailureException &e) {
    LOG_CRIT("failed to start server: %s", e.what());
    closePrimaryClient(primaryClient);
    closeServerScreen(serverScreen);
    return false;
  } catch (BaseException &e) {
    LOG_CRIT("failed to start server: %s", e.what());
    closePrimaryClient(primaryClient);
    closeServerScreen(serverScreen);
    return false;
  }

  return false;
}

deskflow::Screen *ServerApp::openServerScreen()
{
  deskflow::Screen *screen = createScreen();
  getEvents()->addHandler(EventTypes::ScreenError, screen->getEventTarget(), [this](const auto &) {
    handleScreenError();
  });
  getEvents()->addHandler(EventTypes::ScreenSuspend, screen->getEventTarget(), [this](const auto &) {
    handleSuspend();
  });
  getEvents()->addHandler(EventTypes::ScreenResume, screen->getEventTarget(), [this](const auto &) { handleResume(); });
  return screen;
}

bool ServerApp::startServer()
{
  using enum ServerState;
  // skip if already started or starting
  if (m_serverState == Starting || m_serverState == Started) {
    return true;
  }

  // initialize if necessary
  if (m_serverState != Initialized) {
    if (!initServer()) {
      // hard initialization failure
      return false;
    }
    if (m_serverState == Initializing) {
      // not ready to start
      m_serverState = InitializingToStart;
      return true;
    }
    assert(m_serverState == Initialized);
  }

  ClientListener *listener = nullptr;
  try {
    listener = openClientListener(m_config->getDeskflowAddress());
    m_server = openServer(*m_config, m_primaryClient);
    listener->setServer(m_server);
    m_server->setListener(listener);
    m_listener = listener;
    LOG_IPC("started server, waiting for clients");
    m_serverState = Started;
    return true;
  } catch (SocketAddressInUseException &e) {
    LOG_CRIT("cannot listen for clients: %s", e.what());
    closeClientListener(listener);
  } catch (BaseException &e) {
    LOG_CRIT("failed to start server: %s", e.what());
    closeClientListener(listener);
    return false;
  }

  return false;
}

deskflow::Screen *ServerApp::createScreen()
{
#if WINAPI_MSWINDOWS
  return new deskflow::Screen(
      new MSWindowsScreen(true, Settings::value(Settings::Core::UseHooks).toBool(), getEvents()), getEvents()
  );
#endif

#if defined(WINAPI_XWINDOWS) or defined(WINAPI_LIBEI)
  if (deskflow::platform::isWayland()) {
#if WINAPI_LIBEI
    LOG_INFO("using ei screen for wayland");
    return new deskflow::Screen(new deskflow::EiScreen(true, getEvents(), true), getEvents());
#else
    throw XNoEiSupport();
#endif
  }
#endif

#if WINAPI_XWINDOWS
  LOG_INFO("using legacy x windows screen");
  return new deskflow::Screen(
      new XWindowsScreen(qPrintable(Settings::value(Settings::Core::Display).toString()), true, 0, getEvents()),
      getEvents()
  );
#elif WINAPI_CARBON
  return new deskflow::Screen(new OSXScreen(getEvents(), true), getEvents());
#endif
}

PrimaryClient *ServerApp::openPrimaryClient(const std::string &name, deskflow::Screen *screen)
{
  LOG_DEBUG1("creating primary screen");
  return new PrimaryClient(name, screen);
}

void ServerApp::handleSuspend()
{
  if (!m_suspended) {
    LOG_INFO("suspend");
    stopServer();
    m_suspended = true;
  }
}

void ServerApp::handleResume()
{
  if (m_suspended) {
    LOG_INFO("resume");
    startServer();
    m_suspended = false;
  }
}

ClientListener *ServerApp::openClientListener(const NetworkAddress &address)
{
  using enum SecurityLevel;
  auto securityLevel = PlainText;
  if (Settings::value(Settings::Security::TlsEnabled).toBool()) {
    if (Settings::value(Settings::Security::CheckPeers).toBool()) {
      securityLevel = PeerAuth;
    } else {
      securityLevel = Encrypted;
    }
  }

  auto *listen = new ClientListener(getAddress(address), getSocketFactory(), getEvents(), securityLevel);

  getEvents()->addHandler(EventTypes::ClientListenerAccepted, listen, [this, listen](const auto &e) {
    handleClientConnected(e, listen);
  });

  return listen;
}

Server *ServerApp::openServer(ServerConfig &config, PrimaryClient *primaryClient)
{
  auto *server = new Server(config, primaryClient, m_serverScreen, getEvents());
  try {
    getEvents()->addHandler(EventTypes::ServerScreenSwitched, server, [this](const auto &) { handleScreenSwitched(); });

  } catch (std::bad_alloc &ba) {
    delete server;
    throw ba;
  }

  return server;
}

void ServerApp::handleScreenSwitched() const
{
  // do nothing
}

std::unique_ptr<ISocketFactory> ServerApp::getSocketFactory() const
{
  return std::make_unique<TCPSocketFactory>(getEvents(), getSocketMultiplexer());
}

NetworkAddress ServerApp::getAddress(const NetworkAddress &address) const
{
  return address;
}

int ServerApp::mainLoop()
{
  // create socket multiplexer.  this must happen after daemonization
  // on unix because threads evaporate across a fork().
  setSocketMultiplexer(std::make_unique<SocketMultiplexer>());

  // if configuration has no screens then add this system
  // as the default
  if (m_config->begin() == m_config->end()) {
    m_config->addScreen(m_name);
  }

  // set the contact address, if provided, in the config.
  // otherwise, if the config doesn't have an address, use
  // the default.
  if (m_deskflowAddress->isValid()) {
    m_config->setDeskflowAddress(*m_deskflowAddress);
  } else if (!m_config->getDeskflowAddress().isValid()) {
    m_config->setDeskflowAddress(NetworkAddress(kDefaultPort));
  }

  // canonicalize the primary screen name
  if (std::string primaryName = m_config->getCanonicalName(m_name); primaryName.empty()) {
    LOG_CRIT("unknown screen name `%s'", m_name.c_str());
    return s_exitFailed;
  }

  // start server, etc
  appUtil().startNode();

  // handle hangup signal by reloading the server's configuration
  ARCH->setSignalHandler(Arch::ThreadSignal::Hangup, &reloadSignalHandler, nullptr);
  getEvents()->addHandler(EventTypes::ServerAppReloadConfig, getEvents()->getSystemTarget(), [this](const auto &) {
    reloadConfig();
  });

  // handle force reconnect event by disconnecting clients.  they'll
  // reconnect automatically.
  getEvents()->addHandler(EventTypes::ServerAppForceReconnect, getEvents()->getSystemTarget(), [this](const auto &) {
    forceReconnect();
  });

  // to work around the sticky meta keys problem, we'll give users
  // the option to reset the state of the server.
  getEvents()->addHandler(EventTypes::ServerAppResetServer, getEvents()->getSystemTarget(), [this](const auto &) {
    resetServer();
  });

  // run event loop.  if startServer() failed we're supposed to retry
  // later.  the timer installed by startServer() will take care of
  // that.
#if SYSAPI_WIN32
  ArchDaemonWindows::daemonRunning(true);
#endif

#if WINAPI_CARBON

  Thread thread(new TMethodJob<ServerApp>(this, &ServerApp::runEventsLoop, nullptr));

  // wait until carbon loop is ready
  OSXScreen *screen = dynamic_cast<OSXScreen *>(m_serverScreen->getPlatformScreen());
  screen->waitForCarbonLoop();

  runCocoaApp();
#else
  getEvents()->loop();
#endif

#if SYSAPI_WIN32
  ArchDaemonWindows::daemonRunning(false);
#endif

  // close down
  LOG_DEBUG1("stopping server");
  getEvents()->removeHandler(EventTypes::ServerAppForceReconnect, getEvents()->getSystemTarget());
  getEvents()->removeHandler(EventTypes::ServerAppReloadConfig, getEvents()->getSystemTarget());
  cleanupServer();
  LOG_NOTE("stopped server");

  return s_exitSuccess;
}

void ServerApp::resetServer()
{
  LOG_DEBUG1("resetting server");
  stopServer();
  cleanupServer();
  startServer();
}

int ServerApp::runInner(StartupFunc startup)
{
  // general initialization
  m_deskflowAddress = new NetworkAddress;
  m_config = std::make_shared<Config>(getEvents());

  // run
  int result = startup();

  delete m_deskflowAddress;
  return result;
}

int ServerApp::start()
{
  initApp();
  return mainLoop();
}

const char *ServerApp::daemonName() const
{
  if (deskflow::platform::isWindows())
    return "Deskflow Server";
  return "deskflow-server";
}

const char *ServerApp::daemonInfo() const
{
  if (deskflow::platform::isWindows())
    return "Shares this computers mouse and keyboard with other computers.";
  return "";
}

void ServerApp::startNode()
{
  // start the server.  if this return false then we've failed and
  // we shouldn't retry.
  LOG_DEBUG1("starting server");
  if (!startServer()) {
    bye(s_exitFailed);
  }
}
