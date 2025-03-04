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
#include "base/TMethodEventJob.h"
#include "common/constants.h"
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
#include "platform/OSXDragSimulator.h"
#include "platform/OSXScreen.h"
#endif

#if defined(WINAPI_XWINDOWS) or defined(WINAPI_LIBEI)
#include "platform/wayland.h"
#endif

#if defined(MAC_OS_X_VERSION_10_7)
#include "base/TMethodJob.h"
#include "mt/Thread.h"
#endif

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>

using namespace deskflow::server;

//
// ServerApp
//

ServerApp::ServerApp(IEventQueue *events)
    : App(events, new deskflow::ServerArgs()),
      m_server(NULL),
      m_serverState(kUninitialized),
      m_serverScreen(NULL),
      m_primaryClient(NULL),
      m_listener(NULL),
      m_timer(NULL),
      m_deskflowAddress(NULL)
{
}

ServerApp::~ServerApp()
{
}

void ServerApp::parseArgs(int argc, const char *const *argv)
{

  ArgParser argParser(this);
  bool result = argParser.parseServerArgs(args(), argc, argv);

  if (!result || args().m_shouldExitOk || args().m_shouldExitFail) {
    if (args().m_shouldExitOk) {
      m_bye(kExitSuccess);
    } else {
      m_bye(kExitArgs);
    }
  } else {
    if (!args().m_deskflowAddress.empty()) {
      try {
        *m_deskflowAddress = NetworkAddress(args().m_deskflowAddress, kDefaultPort);
        m_deskflowAddress->resolve();
      } catch (XSocketAddress &e) {
        LOG((CLOG_CRIT "%s: %s" BYE, args().m_pname, e.what(), args().m_pname));
        m_bye(kExitArgs);
      }
    }
  }
}

void ServerApp::help()
{
  const auto userConfig = ARCH->concatPath(ARCH->getUserDirectory(), CONFIG_NAME);
  const auto sysConfig = ARCH->concatPath(ARCH->getSystemDirectory(), CONFIG_NAME);

  std::stringstream help;
  help << "Usage: " << args().m_pname

       << " [--address <address>]"
       << " [--config <pathname>]"

#if WINAPI_XWINDOWS
       << " [--display <display>] [--no-xinitthreads]"
#endif

#ifdef WINAPI_LIBEI
       << " [--no-wayland-ei]"
#endif

       << HELP_SYS_ARGS HELP_COMMON_ARGS "\n\n"
       << "Start the " << kAppName << " mouse/keyboard sharing server.\n"
       << "\n"
       << "  -a, --address <address>  listen for clients on the given address.\n"
       << "  -c, --config <pathname>  use the named configuration file "
       << "instead.\n" HELP_COMMON_INFO_1
       << "      --disable-client-cert-check disable client SSL certificate \n"
          "                                     checking (deprecated)\n"
       << HELP_SYS_INFO HELP_COMMON_INFO_2 << "\n"

#if WINAPI_XWINDOWS
       << "      --display <display>  when in X mode, connect to the X server\n"
       << "                             at <display>.\n"
       << "      --no-xinitthreads    do not call XInitThreads()\n"
#endif

       << HELP_SYS_INFO HELP_COMMON_INFO_2 "\n"
       << "* marks defaults.\n"

       << kHelpNoWayland

       << "\n"
       << "The argument for --address is of the form: [<hostname>][:<port>].  "
          "The\n"
       << "hostname must be the address or hostname of an interface on the "
       << "system.\n"
       << "The default is to listen on all interfaces.  The port overrides the\n"
       << "default port, " << kDefaultPort << ".\n"
       << "\n"
       << "If no configuration file pathname is provided then the first of the\n"
       << "following to load successfully sets the configuration:\n"
       << "  " << userConfig << "\n"
       << "  " << sysConfig << "\n";

  LOG((CLOG_PRINT "%s", help.str().c_str()));
}

void ServerApp::reloadSignalHandler(Arch::ESignal, void *)
{
  IEventQueue *events = App::instance().getEvents();
  events->addEvent(Event(events->forServerApp().reloadConfig(), events->getSystemTarget()));
}

void ServerApp::reloadConfig(const Event &, void *)
{
  LOG((CLOG_DEBUG "reload configuration"));
  if (loadConfig(args().m_configFile)) {
    if (m_server != NULL) {
      m_server->setConfig(*args().m_config);
    }
    LOG((CLOG_NOTE "reloaded configuration"));
  }
}

void ServerApp::loadConfig()
{
  bool loaded = false;
  std::string path;

  // load the config file, if specified
  if (!args().m_configFile.empty()) {
    path = args().m_configFile;
    loaded = loadConfig(path);
  }

  // load the default configuration if no explicit file given
  else {
    // get the user's home directory
    path = ARCH->getUserDirectory();
    if (!path.empty()) {
      // complete path
      path = ARCH->concatPath(path, CONFIG_NAME);

      // now try loading the user's configuration
      if (loadConfig(path)) {
        loaded = true;
        args().m_configFile = path;
      }
    }
    if (!loaded) {
      // try the system-wide config file
      path = ARCH->getSystemDirectory();
      if (!path.empty()) {
        path = ARCH->concatPath(path, CONFIG_NAME);
        if (loadConfig(path)) {
          loaded = true;
          args().m_configFile = path;
        }
      }
    }
  }

  if (!loaded) {
    LOG((CLOG_CRIT "%s: failed to load config: %s", args().m_pname, path.c_str()));
    m_bye(kExitConfig);
  }
}

bool ServerApp::loadConfig(const std::string &pathname)
{
  try {
    // load configuration
    LOG((CLOG_DEBUG "opening configuration \"%s\"", pathname.c_str()));
    std::ifstream configStream(deskflow::filesystem::path(pathname));
    if (!configStream.is_open()) {
      // report failure to open configuration as a debug message
      // since we try several paths and we expect some to be
      // missing.
      LOG((CLOG_DEBUG "cannot open configuration \"%s\"", pathname.c_str()));
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

void ServerApp::forceReconnect(const Event &, void *)
{
  if (m_server != NULL) {
    m_server->disconnect();
  }
}

void ServerApp::handleClientConnected(const Event &, void *vlistener)
{
  ClientListener *listener = static_cast<ClientListener *>(vlistener);
  ClientProxy *client = listener->getNextClient();
  if (client != NULL) {
    m_server->adoptClient(client);
    updateStatus();
  }
}

void ServerApp::handleClientsDisconnected(const Event &, void *)
{
  m_events->addEvent(Event(Event::kQuit));
}

void ServerApp::closeServer(Server *server)
{
  if (server == NULL) {
    return;
  }

  // tell all clients to disconnect
  server->disconnect();

  // wait for clients to disconnect for up to timeout seconds
  double timeout = 3.0;
  EventQueueTimer *timer = m_events->newOneShotTimer(timeout, NULL);
  m_events->adoptHandler(
      Event::kTimer, timer, new TMethodEventJob<ServerApp>(this, &ServerApp::handleClientsDisconnected)
  );
  m_events->adoptHandler(
      m_events->forServer().disconnected(), server,
      new TMethodEventJob<ServerApp>(this, &ServerApp::handleClientsDisconnected)
  );

  m_events->loop();

  m_events->removeHandler(Event::kTimer, timer);
  m_events->deleteTimer(timer);
  m_events->removeHandler(m_events->forServer().disconnected(), server);

  // done with server
  delete server;
}

void ServerApp::stopRetryTimer()
{
  if (m_timer != NULL) {
    m_events->removeHandler(Event::kTimer, m_timer);
    m_events->deleteTimer(m_timer);
    m_timer = NULL;
  }
}

void ServerApp::updateStatus()
{
  updateStatus("");
}

void ServerApp::updateStatus(const std::string &msg)
{
}

void ServerApp::closeClientListener(ClientListener *listen)
{
  if (listen != NULL) {
    m_events->removeHandler(m_events->forClientListener().connected(), listen);
    delete listen;
  }
}

void ServerApp::stopServer()
{
  if (m_serverState == kStarted) {
    closeServer(m_server);
    closeClientListener(m_listener);
    m_server = NULL;
    m_listener = NULL;
    m_serverState = kInitialized;
  } else if (m_serverState == kStarting) {
    stopRetryTimer();
    m_serverState = kInitialized;
  }
  assert(m_server == NULL);
  assert(m_listener == NULL);
}

void ServerApp::closePrimaryClient(PrimaryClient *primaryClient)
{
  delete primaryClient;
}

void ServerApp::closeServerScreen(deskflow::Screen *screen)
{
  if (screen != NULL) {
    m_events->removeHandler(m_events->forIScreen().error(), screen->getEventTarget());
    m_events->removeHandler(m_events->forIScreen().suspend(), screen->getEventTarget());
    m_events->removeHandler(m_events->forIScreen().resume(), screen->getEventTarget());
    delete screen;
  }
}

void ServerApp::cleanupServer()
{
  stopServer();
  if (m_serverState == kInitialized) {
    closePrimaryClient(m_primaryClient);
    closeServerScreen(m_serverScreen);
    m_primaryClient = NULL;
    m_serverScreen = NULL;
    m_serverState = kUninitialized;
  } else if (m_serverState == kInitializing || m_serverState == kInitializingToStart) {
    stopRetryTimer();
    m_serverState = kUninitialized;
  }
  assert(m_primaryClient == NULL);
  assert(m_serverScreen == NULL);
  assert(m_serverState == kUninitialized);
}

void ServerApp::retryHandler(const Event &, void *)
{
  // discard old timer
  assert(m_timer != NULL);
  stopRetryTimer();

  // try initializing/starting the server again
  switch (m_serverState) {
  case kUninitialized:
  case kInitialized:
  case kStarted:
    assert(0 && "bad internal server state");
    break;

  case kInitializing:
    LOG((CLOG_DEBUG1 "retry server initialization"));
    m_serverState = kUninitialized;
    if (!initServer()) {
      m_events->addEvent(Event(Event::kQuit));
    }
    break;

  case kInitializingToStart:
    LOG((CLOG_DEBUG1 "retry server initialization"));
    m_serverState = kUninitialized;
    if (!initServer()) {
      m_events->addEvent(Event(Event::kQuit));
    } else if (m_serverState == kInitialized) {
      LOG((CLOG_DEBUG1 "starting server"));
      if (!startServer()) {
        m_events->addEvent(Event(Event::kQuit));
      }
    }
    break;

  case kStarting:
    LOG((CLOG_DEBUG1 "retry starting server"));
    m_serverState = kInitialized;
    if (!startServer()) {
      m_events->addEvent(Event(Event::kQuit));
    }
    break;
  }
}

bool ServerApp::initServer()
{
  // skip if already initialized or initializing
  if (m_serverState != kUninitialized) {
    return true;
  }

  double retryTime;
  deskflow::Screen *serverScreen = NULL;
  PrimaryClient *primaryClient = NULL;
  try {
    std::string name = args().m_config->getCanonicalName(args().m_name);
    serverScreen = openServerScreen();
    primaryClient = openPrimaryClient(name, serverScreen);
    m_serverScreen = serverScreen;
    m_primaryClient = primaryClient;
    m_serverState = kInitialized;
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
    assert(m_timer == NULL);
    LOG((CLOG_DEBUG "retry in %.0f seconds", retryTime));
    m_timer = m_events->newOneShotTimer(retryTime, NULL);
    m_events->adoptHandler(Event::kTimer, m_timer, new TMethodEventJob<ServerApp>(this, &ServerApp::retryHandler));
    m_serverState = kInitializing;
    return true;
  } else {
    // don't try again
    return false;
  }
}

deskflow::Screen *ServerApp::openServerScreen()
{
  deskflow::Screen *screen = createScreen();
  screen->setEnableDragDrop(argsBase().m_enableDragDrop);
  m_events->adoptHandler(
      m_events->forIScreen().error(), screen->getEventTarget(),
      new TMethodEventJob<ServerApp>(this, &ServerApp::handleScreenError)
  );
  m_events->adoptHandler(
      m_events->forIScreen().suspend(), screen->getEventTarget(),
      new TMethodEventJob<ServerApp>(this, &ServerApp::handleSuspend)
  );
  m_events->adoptHandler(
      m_events->forIScreen().resume(), screen->getEventTarget(),
      new TMethodEventJob<ServerApp>(this, &ServerApp::handleResume)
  );
  return screen;
}

bool ServerApp::startServer()
{
  // skip if already started or starting
  if (m_serverState == kStarting || m_serverState == kStarted) {
    return true;
  }

  // initialize if necessary
  if (m_serverState != kInitialized) {
    if (!initServer()) {
      // hard initialization failure
      return false;
    }
    if (m_serverState == kInitializing) {
      // not ready to start
      m_serverState = kInitializingToStart;
      return true;
    }
    assert(m_serverState == kInitialized);
  }

  ClientListener *listener = NULL;
  try {
    listener = openClientListener(args().m_config->getDeskflowAddress());
    m_server = openServer(*args().m_config, m_primaryClient);
    listener->setServer(m_server);
    m_server->setListener(listener);
    m_listener = listener;
    updateStatus();
    LOG((CLOG_NOTE "started server, waiting for clients"));
    m_serverState = kStarted;
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
    assert(m_timer == NULL);
    const auto retryTime = 10.0;
    LOG((CLOG_DEBUG "retry in %.0f seconds", retryTime));
    m_timer = m_events->newOneShotTimer(retryTime, NULL);
    m_events->adoptHandler(Event::kTimer, m_timer, new TMethodEventJob<ServerApp>(this, &ServerApp::retryHandler));
    m_serverState = kStarting;
    return true;
  } else {
    // don't try again
    return false;
  }
}

deskflow::Screen *ServerApp::createScreen()
{
#if WINAPI_MSWINDOWS
  return new deskflow::Screen(
      new MSWindowsScreen(true, args().m_noHooks, args().m_stopOnDeskSwitch, m_events), m_events
  );
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
  return new deskflow::Screen(
      new XWindowsScreen(args().m_display, true, args().m_disableXInitThreads, 0, m_events), m_events
  );
#elif WINAPI_CARBON
  return new deskflow::Screen(new OSXScreen(m_events, true), m_events);
#endif
}

PrimaryClient *ServerApp::openPrimaryClient(const std::string &name, deskflow::Screen *screen)
{
  LOG((CLOG_DEBUG1 "creating primary screen"));
  return new PrimaryClient(name, screen);
}

void ServerApp::handleScreenError(const Event &, void *)
{
  LOG((CLOG_CRIT "error on screen"));
  m_events->addEvent(Event(Event::kQuit));
}

void ServerApp::handleSuspend(const Event &, void *)
{
  if (!m_suspended) {
    LOG((CLOG_INFO "suspend"));
    stopServer();
    m_suspended = true;
  }
}

void ServerApp::handleResume(const Event &, void *)
{
  if (m_suspended) {
    LOG((CLOG_INFO "resume"));
    startServer();
    m_suspended = false;
  }
}

ClientListener *ServerApp::openClientListener(const NetworkAddress &address)
{
  auto securityLevel = args().m_enableCrypto ? args().m_chkPeerCert ? SecurityLevel::PeerAuth : SecurityLevel::Encrypted
                                             : SecurityLevel::PlainText;

  ClientListener *listen = new ClientListener(getAddress(address), getSocketFactory(), m_events, securityLevel);

  m_events->adoptHandler(
      m_events->forClientListener().connected(), listen,
      new TMethodEventJob<ServerApp>(this, &ServerApp::handleClientConnected, listen)
  );

  return listen;
}

Server *ServerApp::openServer(ServerConfig &config, PrimaryClient *primaryClient)
{
  Server *server = new Server(config, primaryClient, m_serverScreen, m_events, args());
  try {
    m_events->adoptHandler(
        m_events->forServer().disconnected(), server, new TMethodEventJob<ServerApp>(this, &ServerApp::handleNoClients)
    );

    m_events->adoptHandler(
        m_events->forServer().screenSwitched(), server,
        new TMethodEventJob<ServerApp>(this, &ServerApp::handleScreenSwitched)
    );

  } catch (std::bad_alloc &ba) {
    delete server;
    throw ba;
  }

  return server;
}

void ServerApp::handleNoClients(const Event &, void *)
{
  updateStatus();
}

void ServerApp::handleScreenSwitched(const Event &e, void *)
{
}

ISocketFactory *ServerApp::getSocketFactory() const
{
  return new TCPSocketFactory(m_events, getSocketMultiplexer());
}

NetworkAddress ServerApp::getAddress(const NetworkAddress &address) const
{
  if (args().m_config->isClientMode()) {
    const auto clientAddress = args().m_config->getClientAddress();
    NetworkAddress addr(clientAddress.c_str(), kDefaultPort);
    addr.resolve();
    return addr;
  } else {
    return address;
  }
}

int ServerApp::mainLoop()
{
  // create socket multiplexer.  this must happen after daemonization
  // on unix because threads evaporate across a fork().
  SocketMultiplexer multiplexer;
  setSocketMultiplexer(&multiplexer);

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
  std::string primaryName = args().m_config->getCanonicalName(args().m_name);
  if (primaryName.empty()) {
    LOG((CLOG_CRIT "unknown screen name `%s'", args().m_name.c_str()));
    return kExitFailed;
  }

  // start server, etc
  appUtil().startNode();

  // handle hangup signal by reloading the server's configuration
  ARCH->setSignalHandler(Arch::kHANGUP, &reloadSignalHandler, NULL);
  m_events->adoptHandler(
      m_events->forServerApp().reloadConfig(), m_events->getSystemTarget(),
      new TMethodEventJob<ServerApp>(this, &ServerApp::reloadConfig)
  );

  // handle force reconnect event by disconnecting clients.  they'll
  // reconnect automatically.
  m_events->adoptHandler(
      m_events->forServerApp().forceReconnect(), m_events->getSystemTarget(),
      new TMethodEventJob<ServerApp>(this, &ServerApp::forceReconnect)
  );

  // to work around the sticky meta keys problem, we'll give users
  // the option to reset the state of the server.
  m_events->adoptHandler(
      m_events->forServerApp().resetServer(), m_events->getSystemTarget(),
      new TMethodEventJob<ServerApp>(this, &ServerApp::resetServer)
  );

  // run event loop.  if startServer() failed we're supposed to retry
  // later.  the timer installed by startServer() will take care of
  // that.
  DAEMON_RUNNING(true);

#if defined(MAC_OS_X_VERSION_10_7)

  Thread thread(new TMethodJob<ServerApp>(this, &ServerApp::runEventsLoop, NULL));

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
  m_events->removeHandler(m_events->forServerApp().forceReconnect(), m_events->getSystemTarget());
  m_events->removeHandler(m_events->forServerApp().reloadConfig(), m_events->getSystemTarget());
  cleanupServer();
  updateStatus();
  LOG((CLOG_NOTE "stopped server"));

  return kExitSuccess;
}

void ServerApp::resetServer(const Event &, void *)
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
  args().m_pname = ARCH->getBasename(argv[0]);

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
    m_bye(kExitFailed);
  }
}
