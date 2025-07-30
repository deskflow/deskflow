/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ServerApp.h"

#include "arch/Arch.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "base/Path.h"
#include "deskflow/App.h"
#include "deskflow/ArgParser.h"
#include "deskflow/Screen.h"
#include "deskflow/ServerArgs.h"
#include "deskflow/XScreen.h"
#include "net/SocketMultiplexer.h"
#include "net/TCPSocketFactory.h"
#include "net/XSocket.h"
#include "server/ClientListener.h"
#include "server/ClientProxy.h"
#include "server/Config.h"
#include "server/PrimaryClient.h"
#include "server/Server.h"

// must be before screen header includes
#include <QFileInfo>

#if SYSAPI_WIN32
#include "arch/win32/ArchMiscWindows.h"
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

#if defined(WINAPI_XWINDOWS) or defined(WINAPI_LIBEI)
#include "platform/Wayland.h"
#endif

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>

using namespace deskflow::server;

//
// ServerApp
//

ServerApp::ServerApp(IEventQueue *events) : App(events, new deskflow::ServerArgs())
{
  // do nothing
}

void ServerApp::parseArgs(int argc, const char *const *argv)
{

  ArgParser argParser(this);
  bool result = argParser.parseServerArgs(args(), argc, argv);

  if (!result || args().m_shouldExitOk || args().m_shouldExitFail) {
    if (args().m_shouldExitOk) {
      m_bye(s_exitSuccess);
    } else {
      m_bye(s_exitArgs);
    }
  } else {
    if (!args().m_deskflowAddress.empty()) {
      try {
        *m_deskflowAddress = NetworkAddress(args().m_deskflowAddress, kDefaultPort);
        m_deskflowAddress->resolve();
      } catch (XSocketAddress &e) {
        LOG((CLOG_CRIT "%s: %s" BYE, args().m_pname, e.what(), args().m_pname));
        m_bye(s_exitArgs);
      }
    }
  }
}

void ServerApp::help()
{
  std::stringstream help;
  help << "Usage: " << args().m_pname

       << " --config <pathname>"
       << " [--address <address>]"

#if WINAPI_XWINDOWS
       << " [--display <display>]"
#endif

       << s_helpSysArgs << s_helpCommonArgs << "\n"
       << "\n"
       << "Start the " << kAppName << " mouse/keyboard sharing server.\n"
       << "\n"
       << "  -a, --address <address>  listen for clients on the given address.\n"
       << "  -c, --config <pathname>  path of the configuration file\n"
       << s_helpGeneralArgs
       << "      --disable-client-cert-check disable client SSL certificate \n"
          "                                     checking (deprecated)\n"
       << s_helpSysInfo << s_helpVersionArgs << "\n"

#if WINAPI_XWINDOWS
       << "      --display <display>  when in X mode, connect to the X server\n"
       << "                             at <display>.\n"
#endif

       << "* marks defaults.\n"

       << s_helpNoWayland

       << "\n"
       << "The argument for --address is of the form: [<hostname>][:<port>].  "
          "The\n"
       << "hostname must be the address or hostname of an interface on the "
       << "system.\n"
       << "The default is to listen on all interfaces.  The port overrides the\n"
       << "default port, " << kDefaultPort << ".\n";

  LOG((CLOG_PRINT "%s", help.str().c_str()));
}

void ServerApp::reloadSignalHandler(Arch::ThreadSignal, void *)
{
  IEventQueue *events = App::instance().getEvents();
  events->addEvent(Event(EventTypes::ServerAppReloadConfig, events->getSystemTarget()));
}

void ServerApp::reloadConfig()
{
  LOG((CLOG_DEBUG "reload configuration"));
  if (loadConfig(args().m_configFile)) {
    if (m_server != nullptr) {
      m_server->setConfig(*args().m_config);
    }
    LOG((CLOG_NOTE "reloaded configuration"));
  }
}

void ServerApp::loadConfig()
{
  const auto path = args().m_configFile;
  if (path.empty()) {
    LOG((CLOG_CRIT "no configuration path provided"));
    m_bye(s_exitConfig);
  }

  if (!loadConfig(path)) {
    LOG((CLOG_CRIT "%s: failed to load config: %s", args().m_pname, path.c_str()));
    m_bye(s_exitConfig);
  }
}

bool ServerApp::loadConfig(const std::string &pathname)
{
  try {
    // load configuration
    LOG((CLOG_DEBUG "opening configuration \"%s\"", pathname.c_str()));
    std::ifstream configStream(deskflow::filesystem::path(pathname));
    if (!configStream.is_open()) {
      LOG((CLOG_ERR "cannot open configuration \"%s\"", pathname.c_str()));
      return false;
    }
    configStream >> *args().m_config;
    LOG((CLOG_DEBUG "configuration read successfully"));
    return true;
  } catch (XConfigRead &e) {
    // report error in configuration file
    LOG((CLOG_ERR "cannot read configuration \"%s\": %s", pathname.c_str(), e.what()));
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
    updateStatus();
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
  EventQueueTimer *timer = m_events->newOneShotTimer(timeout, nullptr);
  m_events->addHandler(EventTypes::Timer, timer, [this](const auto &) { m_events->addEvent(Event(EventTypes::Quit)); });
  m_events->addHandler(EventTypes::ServerDisconnected, server, [this](const auto &) {
    m_events->addEvent(Event(EventTypes::Quit));
  });

  m_events->loop();

  m_events->removeHandler(EventTypes::Timer, timer);
  m_events->deleteTimer(timer);
  m_events->removeHandler(EventTypes::ServerDisconnected, server);

  // done with server
  delete server;
}

void ServerApp::stopRetryTimer()
{
  if (m_timer != nullptr) {
    m_events->removeHandler(EventTypes::Timer, m_timer);
    m_events->deleteTimer(m_timer);
    m_timer = nullptr;
  }
}

void ServerApp::updateStatus() const
{
  updateStatus("");
}

void ServerApp::updateStatus(const std::string_view &msg) const
{
  // do nothing
}

void ServerApp::closeClientListener(ClientListener *listen)
{
  if (listen != nullptr) {
    m_events->removeHandler(EventTypes::ClientListenerAccepted, listen);
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
    m_events->removeHandler(ScreenError, screen->getEventTarget());
    m_events->removeHandler(ScreenSuspend, screen->getEventTarget());
    m_events->removeHandler(ScreenResume, screen->getEventTarget());
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
    LOG((CLOG_DEBUG1 "retry server initialization"));
    m_serverState = Uninitialized;
    if (!initServer()) {
      m_events->addEvent(Event(EventTypes::Quit));
    }
    break;

  case InitializingToStart:
    LOG((CLOG_DEBUG1 "retry server initialization"));
    m_serverState = Uninitialized;
    if (!initServer()) {
      m_events->addEvent(Event(EventTypes::Quit));
    } else if (m_serverState == Initialized) {
      LOG((CLOG_DEBUG1 "starting server"));
      if (!startServer()) {
        m_events->addEvent(Event(EventTypes::Quit));
      }
    }
    break;

  case Starting:
    LOG((CLOG_DEBUG1 "retry starting server"));
    m_serverState = Initialized;
    if (!startServer()) {
      m_events->addEvent(Event(EventTypes::Quit));
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
    std::string name = args().m_config->getCanonicalName(args().m_name);
    serverScreen = openServerScreen();
    primaryClient = openPrimaryClient(name, serverScreen);
    m_serverScreen = serverScreen;
    m_primaryClient = primaryClient;
    m_serverState = Initialized;
    updateStatus();
    return true;
  } catch (XScreenUnavailable &e) {
    LOG((CLOG_WARN "primary screen unavailable: %s", e.what()));
    closePrimaryClient(primaryClient);
    closeServerScreen(serverScreen);
    updateStatus(std::string("primary screen unavailable: ") + e.what());
    retryTime = e.getRetryTime();
  } catch (XScreenOpenFailure &e) {
    LOG((CLOG_CRIT "failed to start server: %s", e.what()));
    closePrimaryClient(primaryClient);
    closeServerScreen(serverScreen);
    return false;
  } catch (XBase &e) {
    LOG((CLOG_CRIT "failed to start server: %s", e.what()));
    closePrimaryClient(primaryClient);
    closeServerScreen(serverScreen);
    return false;
  }

  if (args().m_restartable) {
    // install a timer and handler to retry later
    assert(m_timer == nullptr);
    LOG((CLOG_DEBUG "retry in %.0f seconds", retryTime));
    m_timer = m_events->newOneShotTimer(retryTime, nullptr);
    m_events->addHandler(EventTypes::Timer, m_timer, [this](const auto &) { retryHandler(); });
    m_serverState = Initializing;
    return true;
  } else {
    // don't try again
    return false;
  }
}

deskflow::Screen *ServerApp::openServerScreen()
{
  deskflow::Screen *screen = createScreen();
  m_events->addHandler(EventTypes::ScreenError, screen->getEventTarget(), [this](const auto &) {
    handleScreenError();
  });
  m_events->addHandler(EventTypes::ScreenSuspend, screen->getEventTarget(), [this](const auto &) { handleSuspend(); });
  m_events->addHandler(EventTypes::ScreenResume, screen->getEventTarget(), [this](const auto &) { handleResume(); });
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
    listener = openClientListener(args().m_config->getDeskflowAddress());
    m_server = openServer(*args().m_config, m_primaryClient);
    listener->setServer(m_server);
    m_server->setListener(listener);
    m_listener = listener;
    updateStatus();
    LOG((CLOG_NOTE "started server, waiting for clients"));
    m_serverState = Started;
    return true;
  } catch (XSocketAddressInUse &e) {
    if (args().m_restartable) {
      LOG((CLOG_ERR "cannot listen for clients: %s", e.what()));
    } else {
      LOG((CLOG_CRIT "cannot listen for clients: %s", e.what()));
    }
    closeClientListener(listener);
    updateStatus(std::string("cannot listen for clients: ") + e.what());
  } catch (XBase &e) {
    LOG((CLOG_CRIT "failed to start server: %s", e.what()));
    closeClientListener(listener);
    return false;
  }

  if (args().m_restartable) {
    // install a timer and handler to retry later
    assert(m_timer == nullptr);
    const auto retryTime = 10.0;
    LOG((CLOG_DEBUG "retry in %.0f seconds", retryTime));
    m_timer = m_events->newOneShotTimer(retryTime, nullptr);
    m_events->addHandler(EventTypes::Timer, m_timer, [this](const auto &) { retryHandler(); });
    m_serverState = Starting;
    return true;
  } else {
    // don't try again
    return false;
  }
}

deskflow::Screen *ServerApp::createScreen()
{
#if WINAPI_MSWINDOWS
  return new deskflow::Screen(new MSWindowsScreen(true, args().m_noHooks, m_events), m_events);
#endif

#if defined(WINAPI_XWINDOWS) or defined(WINAPI_LIBEI)
  if (deskflow::platform::isWayland()) {
#if WINAPI_LIBEI
    LOG((CLOG_INFO "using ei screen for wayland"));
    return new deskflow::Screen(new deskflow::EiScreen(true, m_events, true), m_events);
#else
    throw XNoEiSupport();
#endif
  }
#endif

#if WINAPI_XWINDOWS
  LOG((CLOG_INFO "using legacy x windows screen"));
  return new deskflow::Screen(new XWindowsScreen(args().m_display, true, 0, m_events), m_events);
#elif WINAPI_CARBON
  return new deskflow::Screen(new OSXScreen(m_events, true), m_events);
#endif
}

PrimaryClient *ServerApp::openPrimaryClient(const std::string &name, deskflow::Screen *screen)
{
  LOG((CLOG_DEBUG1 "creating primary screen"));
  return new PrimaryClient(name, screen);
}

void ServerApp::handleScreenError()
{
  LOG((CLOG_CRIT "error on screen"));
  m_events->addEvent(Event(EventTypes::Quit));
}

void ServerApp::handleSuspend()
{
  if (!m_suspended) {
    LOG((CLOG_INFO "suspend"));
    stopServer();
    m_suspended = true;
  }
}

void ServerApp::handleResume()
{
  if (m_suspended) {
    LOG((CLOG_INFO "resume"));
    startServer();
    m_suspended = false;
  }
}

ClientListener *ServerApp::openClientListener(const NetworkAddress &address)
{
  using enum SecurityLevel;
  auto securityLevel = PlainText;
  if (args().m_enableCrypto) {
    if (args().m_chkPeerCert) {
      securityLevel = PeerAuth;
    } else {
      securityLevel = Encrypted;
    }
  }

  auto *listen = new ClientListener(getAddress(address), getSocketFactory(), m_events, securityLevel);

  m_events->addHandler(EventTypes::ClientListenerAccepted, listen, [this, listen](const auto &e) {
    handleClientConnected(e, listen);
  });

  return listen;
}

Server *ServerApp::openServer(ServerConfig &config, PrimaryClient *primaryClient)
{
  auto *server = new Server(config, primaryClient, m_serverScreen, m_events, args());
  try {
    m_events->addHandler(EventTypes::ServerDisconnected, server, [this](const auto &) { updateStatus(); });
    m_events->addHandler(EventTypes::ServerScreenSwitched, server, [this](const auto &) { handleScreenSwitched(); });

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
  return std::make_unique<TCPSocketFactory>(m_events, getSocketMultiplexer());
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
  if (args().m_config->begin() == args().m_config->end()) {
    args().m_config->addScreen(args().m_name);
  }

  // set the contact address, if provided, in the config.
  // otherwise, if the config doesn't have an address, use
  // the default.
  if (m_deskflowAddress->isValid()) {
    args().m_config->setDeskflowAddress(*m_deskflowAddress);
  } else if (!args().m_config->getDeskflowAddress().isValid()) {
    args().m_config->setDeskflowAddress(NetworkAddress(kDefaultPort));
  }

  // canonicalize the primary screen name
  if (std::string primaryName = args().m_config->getCanonicalName(args().m_name); primaryName.empty()) {
    LOG((CLOG_CRIT "unknown screen name `%s'", args().m_name.c_str()));
    return s_exitFailed;
  }

  // start server, etc
  appUtil().startNode();

  // handle hangup signal by reloading the server's configuration
  ARCH->setSignalHandler(Arch::ThreadSignal::Hangup, &reloadSignalHandler, nullptr);
  m_events->addHandler(EventTypes::ServerAppReloadConfig, m_events->getSystemTarget(), [this](const auto &) {
    reloadConfig();
  });

  // handle force reconnect event by disconnecting clients.  they'll
  // reconnect automatically.
  m_events->addHandler(EventTypes::ServerAppForceReconnect, m_events->getSystemTarget(), [this](const auto &) {
    forceReconnect();
  });

  // to work around the sticky meta keys problem, we'll give users
  // the option to reset the state of the server.
  m_events->addHandler(EventTypes::ServerAppResetServer, m_events->getSystemTarget(), [this](const auto &) {
    resetServer();
  });

  // run event loop.  if startServer() failed we're supposed to retry
  // later.  the timer installed by startServer() will take care of
  // that.
  DAEMON_RUNNING(true);

#if WINAPI_CARBON

  Thread thread(new TMethodJob<ServerApp>(this, &ServerApp::runEventsLoop, nullptr));

  // wait until carbon loop is ready
  OSXScreen *screen = dynamic_cast<OSXScreen *>(m_serverScreen->getPlatformScreen());
  screen->waitForCarbonLoop();

  runCocoaApp();
#else
  m_events->loop();
#endif

  DAEMON_RUNNING(false);

  // close down
  LOG((CLOG_DEBUG1 "stopping server"));
  m_events->removeHandler(EventTypes::ServerAppForceReconnect, m_events->getSystemTarget());
  m_events->removeHandler(EventTypes::ServerAppReloadConfig, m_events->getSystemTarget());
  cleanupServer();
  updateStatus();
  LOG((CLOG_NOTE "stopped server"));

  return s_exitSuccess;
}

void ServerApp::resetServer()
{
  LOG((CLOG_DEBUG1 "resetting server"));
  stopServer();
  cleanupServer();
  startServer();
}

int ServerApp::runInner(int argc, char **argv, StartupFunc startup)
{
  // general initialization
  m_deskflowAddress = new NetworkAddress;
  args().m_config = std::make_shared<Config>(m_events);
  args().m_pname = QFileInfo(argv[0]).fileName().toLocal8Bit().constData();

  // run
  int result = startup(argc, argv);

  delete m_deskflowAddress;
  return result;
}

int daemonMainLoopStatic(int argc, const char **argv)
{
  return ServerApp::instance().daemonMainLoop(argc, argv);
}

int ServerApp::standardStartup(int argc, char **argv)
{
  initApp(argc, argv);

  // daemonize if requested
  if (args().m_daemon) {
    return ARCH->daemonize(daemonName(), daemonMainLoopStatic);
  } else {
    return mainLoop();
  }
}

int ServerApp::foregroundStartup(int argc, char **argv)
{
  initApp(argc, argv);

  // never daemonize
  return mainLoop();
}

const char *ServerApp::daemonName() const
{
#if SYSAPI_WIN32
  return "Deskflow Server";
#elif SYSAPI_UNIX
  return "deskflow-server";
#endif
}

const char *ServerApp::daemonInfo() const
{
#if SYSAPI_WIN32
  return "Shares this computers mouse and keyboard with other computers.";
#elif SYSAPI_UNIX
  return "";
#endif
}

void ServerApp::startNode()
{
  // start the server.  if this return false then we've failed and
  // we shouldn't retry.
  LOG((CLOG_DEBUG1 "starting server"));
  if (!startServer()) {
    m_bye(s_exitFailed);
  }
}
