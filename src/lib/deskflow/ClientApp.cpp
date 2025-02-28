/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ClientApp.h"

#include "arch/Arch.h"
#include "base/Event.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "base/TMethodEventJob.h"
#include "client/Client.h"
#include "common/constants.h"
#include "deskflow/ArgParser.h"
#include "deskflow/ClientArgs.h"
#include "deskflow/Screen.h"
#include "deskflow/XScreen.h"
#include "deskflow/protocol_types.h"
#include "net/NetworkAddress.h"
#include "net/SocketMultiplexer.h"
#include "net/TCPSocketFactory.h"
#include "net/XSocket.h"
#include "platform/wayland.h"

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

#include <memory>
#include <sstream>
#include <stdio.h>

#define RETRY_TIME 1.0

ClientApp::ClientApp(IEventQueue *events)
    : App(events, new deskflow::ClientArgs()),
      m_client(NULL),
      m_clientScreen(NULL),
      m_serverAddress(NULL)
{
}

ClientApp::~ClientApp()
{
}

void ClientApp::parseArgs(int argc, const char *const *argv)
{
  ArgParser argParser(this);
  bool result = argParser.parseClientArgs(args(), argc, argv);

  if (!result || args().m_shouldExitOk || args().m_shouldExitFail) {
    if (args().m_shouldExitOk) {
      m_bye(kExitSuccess);
    } else {
      m_bye(kExitArgs);
    }
  } else {
    // save server address
    if (!args().m_serverAddress.empty()) {
      try {
        *m_serverAddress = NetworkAddress(args().m_serverAddress, kDefaultPort);
        m_serverAddress->resolve();
      } catch (XSocketAddress &e) {
        // allow an address that we can't look up if we're restartable.
        // we'll try to resolve the address each time we connect to the
        // server.  a bad port will never get better.  patch by Brent
        // Priddy.
        if (!args().m_restartable || e.getError() == XSocketAddress::kBadPort) {
          LOG((CLOG_CRIT "%s: %s" BYE, args().m_pname, e.what(), args().m_pname));
          m_bye(kExitFailed);
        }
      }
    }
  }
}

void ClientApp::help()
{
  std::stringstream help;
  help << "Usage: " << args().m_pname << " [--address <address>]"
       << " [--yscroll <delta>]"
       << " [--sync-language]"
       << " [--invert-scroll]"
       << " [--host]"
#ifdef WINAPI_XWINDOWS
       << " [--display <display>]"
       << " [--no-xinitthreads]"
#endif
#ifdef WINAPI_LIBEI
       << " [--use-x-window]"
#endif
       << HELP_SYS_ARGS << HELP_COMMON_ARGS << " <server-address>"
       << "\n\n"
       << "Connect to a " << kAppName << " mouse/keyboard sharing server.\n"
       << "\n"
       << "  -a, --address <address>  local network interface address.\n"
       << HELP_COMMON_INFO_1 << HELP_SYS_INFO << "      --yscroll <delta>    defines the vertical scrolling delta,\n"
       << "                             which is 120 by default.\n"
       << "      --sync-language      enable language synchronization.\n"
       << "      --invert-scroll      invert scroll direction on this\n"
       << "                             computer.\n"
       << "      --host               act as a host; invert server/client mode\n"
       << "                             and listen instead of connecting.\n"
#if WINAPI_XWINDOWS
       << "      --display <display>  when in X mode, connect to the X server\n"
       << "                             at <display>.\n"
       << "      --no-xinitthreads    do not call XInitThreads()\n"
#endif
       << HELP_COMMON_INFO_2 << "\n"
       << "* marks defaults.\n"

       << kHelpNoWayland

       << "\n"
       << "The server address is of the form: [<hostname>][:<port>].\n"
       << "The hostname must be the address or hostname of the server.\n"
       << "The port overrides the default port, " << kDefaultPort << ".\n";

  LOG((CLOG_PRINT "%s", help.str().c_str()));
}

const char *ClientApp::daemonName() const
{
#if SYSAPI_WIN32
  return "Deskflow Client";
#elif SYSAPI_UNIX
  return "deskflow-client";
#endif
}

const char *ClientApp::daemonInfo() const
{
#if SYSAPI_WIN32
  return "Allows another computer to share it's keyboard and mouse with this "
         "computer.";
#elif SYSAPI_UNIX
  return "";
#endif
}

deskflow::Screen *ClientApp::createScreen()
{
#if WINAPI_MSWINDOWS
  return new deskflow::Screen(
      new MSWindowsScreen(
          false, args().m_noHooks, args().m_stopOnDeskSwitch, m_events, args().m_enableLangSync,
          args().m_clientScrollDirection
      ),
      m_events
  );
#endif

#if defined(WINAPI_XWINDOWS) or defined(WINAPI_LIBEI)
  if (deskflow::platform::isWayland()) {
#if WINAPI_LIBEI
    LOG((CLOG_INFO "using ei screen for wayland"));
    return new deskflow::Screen(new deskflow::EiScreen(false, m_events, true), m_events);
#else
    throw XNoEiSupport();
#endif
  }
#endif

#if WINAPI_XWINDOWS
  LOG((CLOG_INFO "using legacy x windows screen"));
  return new deskflow::Screen(
      new XWindowsScreen(
          args().m_display, false, args().m_disableXInitThreads, args().m_yscroll, m_events,
          args().m_clientScrollDirection
      ),
      m_events
  );

#endif

#if WINAPI_CARBON
  return new deskflow::Screen(
      new OSXScreen(m_events, false, args().m_enableLangSync, args().m_clientScrollDirection), m_events
  );
#endif
}

void ClientApp::updateStatus()
{
  updateStatus("");
}

void ClientApp::updateStatus(const std::string &msg)
{
}

void ClientApp::resetRestartTimeout()
{
  // retry time can nolonger be changed
  // s_retryTime = 0.0;
}

double ClientApp::nextRestartTimeout()
{
  // retry at a constant rate (Issue 52)
  return RETRY_TIME;

  /*
  // choose next restart timeout.  we start with rapid retries
  // then slow down.
  if (s_retryTime < 1.0) {
  s_retryTime = 1.0;
  }
  else if (s_retryTime < 3.0) {
  s_retryTime = 3.0;
  }
  else {
  s_retryTime = 5.0;
  }
  return s_retryTime;
  */
}

void ClientApp::handleScreenError(const Event &, void *)
{
  LOG((CLOG_CRIT "error on screen"));
  m_events->addEvent(Event(Event::kQuit));
}

deskflow::Screen *ClientApp::openClientScreen()
{
  deskflow::Screen *screen = createScreen();
  screen->setEnableDragDrop(argsBase().m_enableDragDrop);
  m_events->adoptHandler(
      m_events->forIScreen().error(), screen->getEventTarget(),
      new TMethodEventJob<ClientApp>(this, &ClientApp::handleScreenError)
  );
  return screen;
}

void ClientApp::closeClientScreen(deskflow::Screen *screen)
{
  if (screen != NULL) {
    m_events->removeHandler(m_events->forIScreen().error(), screen->getEventTarget());
    delete screen;
  }
}

void ClientApp::handleClientRestart(const Event &, void *vtimer)
{
  // discard old timer
  EventQueueTimer *timer = static_cast<EventQueueTimer *>(vtimer);
  m_events->deleteTimer(timer);
  m_events->removeHandler(Event::kTimer, timer);

  // reconnect
  startClient();
}

void ClientApp::scheduleClientRestart(double retryTime)
{
  // install a timer and handler to retry later
  LOG((CLOG_DEBUG "retry in %.0f seconds", retryTime));
  EventQueueTimer *timer = m_events->newOneShotTimer(retryTime, NULL);
  m_events->adoptHandler(
      Event::kTimer, timer, new TMethodEventJob<ClientApp>(this, &ClientApp::handleClientRestart, timer)
  );
}

void ClientApp::handleClientConnected(const Event &, void *)
{
  LOG((CLOG_NOTE "connected to server"));
  resetRestartTimeout();
  updateStatus();
}

void ClientApp::handleClientFailed(const Event &e, void *)
{
  if ((++m_lastServerAddressIndex) < m_client->getLastResolvedAddressesCount()) {
    std::unique_ptr<Client::FailInfo> info(static_cast<Client::FailInfo *>(e.getData()));

    updateStatus(std::string("Failed to connect to server: ") + info->m_what + " Trying next address...");
    LOG((CLOG_WARN "failed to connect to server=%s, trying next address", info->m_what.c_str()));
    if (!m_suspended) {
      scheduleClientRestart(nextRestartTimeout());
    }
  } else {
    m_lastServerAddressIndex = 0;
    handleClientRefused(e, nullptr);
  }
}

void ClientApp::handleClientRefused(const Event &e, void *)
{
  std::unique_ptr<Client::FailInfo> info(static_cast<Client::FailInfo *>(e.getData()));

  updateStatus(std::string("Failed to connect to server: ") + info->m_what);
  if (!args().m_restartable || !info->m_retry) {
    LOG((CLOG_ERR "failed to connect to server: %s", info->m_what.c_str()));
    m_events->addEvent(Event(Event::kQuit));
  } else {
    LOG((CLOG_WARN "failed to connect to server: %s", info->m_what.c_str()));
    if (!m_suspended) {
      scheduleClientRestart(nextRestartTimeout());
    }
  }
}

void ClientApp::handleClientDisconnected(const Event &, void *)
{
  LOG((CLOG_NOTE "disconnected from server"));
  if (!args().m_restartable) {
    m_events->addEvent(Event(Event::kQuit));
  } else if (!m_suspended) {
    scheduleClientRestart(nextRestartTimeout());
  }
  updateStatus();
}

Client *ClientApp::openClient(const std::string &name, const NetworkAddress &address, deskflow::Screen *screen)
{
  Client *client = new Client(m_events, name, address, getSocketFactory(), screen, args());

  try {
    m_events->adoptHandler(
        m_events->forClient().connected(), client->getEventTarget(),
        new TMethodEventJob<ClientApp>(this, &ClientApp::handleClientConnected)
    );

    m_events->adoptHandler(
        m_events->forClient().connectionFailed(), client->getEventTarget(),
        new TMethodEventJob<ClientApp>(this, &ClientApp::handleClientFailed)
    );

    m_events->adoptHandler(
        m_events->forClient().connectionRefused(), client->getEventTarget(),
        new TMethodEventJob<ClientApp>(this, &ClientApp::handleClientRefused)
    );

    m_events->adoptHandler(
        m_events->forClient().disconnected(), client->getEventTarget(),
        new TMethodEventJob<ClientApp>(this, &ClientApp::handleClientDisconnected)
    );

  } catch (std::bad_alloc &ba) {
    delete client;
    throw ba;
  }

  return client;
}

void ClientApp::closeClient(Client *client)
{
  if (client == NULL) {
    return;
  }

  m_events->removeHandler(m_events->forClient().connected(), client);
  m_events->removeHandler(m_events->forClient().connectionFailed(), client);
  m_events->removeHandler(m_events->forClient().connectionRefused(), client);
  m_events->removeHandler(m_events->forClient().disconnected(), client);
  delete client;
}

int ClientApp::foregroundStartup(int argc, char **argv)
{
  initApp(argc, argv);

  // never daemonize
  return mainLoop();
}

bool ClientApp::startClient()
{
  double retryTime;
  deskflow::Screen *clientScreen = NULL;
  try {
    if (m_clientScreen == NULL) {
      clientScreen = openClientScreen();
      m_client = openClient(args().m_name, *m_serverAddress, clientScreen);
      m_clientScreen = clientScreen;
      LOG((CLOG_NOTE "started client"));
    }

    m_client->connect(m_lastServerAddressIndex);

    updateStatus();
    return true;
  } catch (XScreenUnavailable &e) {
    LOG((CLOG_WARN "secondary screen unavailable: %s", e.what()));
    closeClientScreen(clientScreen);
    updateStatus(std::string("secondary screen unavailable: ") + e.what());
    retryTime = e.getRetryTime();
  } catch (XScreenOpenFailure &e) {
    LOG((CLOG_CRIT "failed to start client: %s", e.what()));
    closeClientScreen(clientScreen);
    return false;
  } catch (XBase &e) {
    LOG((CLOG_CRIT "failed to start client: %s", e.what()));
    closeClientScreen(clientScreen);
    return false;
  }

  if (args().m_restartable) {
    scheduleClientRestart(retryTime);
    return true;
  } else {
    // don't try again
    return false;
  }
}

void ClientApp::stopClient()
{
  closeClient(m_client);
  closeClientScreen(m_clientScreen);
  m_client = NULL;
  m_clientScreen = NULL;
}

int ClientApp::mainLoop()
{
  // create socket multiplexer.  this must happen after daemonization
  // on unix because threads evaporate across a fork().
  SocketMultiplexer multiplexer;
  setSocketMultiplexer(&multiplexer);

  // start client, etc
  appUtil().startNode();

  // run event loop.  if startClient() failed we're supposed to retry
  // later.  the timer installed by startClient() will take care of
  // that.
  DAEMON_RUNNING(true);

#if defined(MAC_OS_X_VERSION_10_7)

  Thread thread(new TMethodJob<ClientApp>(this, &ClientApp::runEventsLoop, NULL));

  // wait until carbon loop is ready
  OSXScreen *screen = dynamic_cast<OSXScreen *>(m_clientScreen->getPlatformScreen());
  screen->waitForCarbonLoop();

  runCocoaApp();
#else
  m_events->loop();
#endif

  DAEMON_RUNNING(false);

  // close down
  LOG((CLOG_DEBUG1 "stopping client"));
  stopClient();
  updateStatus();
  LOG((CLOG_NOTE "stopped client"));

  return kExitSuccess;
}

static int daemonMainLoopStatic(int argc, const char **argv)
{
  return ClientApp::instance().daemonMainLoop(argc, argv);
}

int ClientApp::standardStartup(int argc, char **argv)
{
  initApp(argc, argv);

  // daemonize if requested
  if (args().m_daemon) {
    return ARCH->daemonize(daemonName(), &daemonMainLoopStatic);
  } else {
    return mainLoop();
  }
}

int ClientApp::runInner(int argc, char **argv, StartupFunc startup)
{
  // general initialization
  m_serverAddress = new NetworkAddress;
  args().m_pname = ARCH->getBasename(argv[0]);

  int result;
  try {
    // run
    result = startup(argc, argv);
  } catch (...) {
    delete m_serverAddress;

    throw;
  }

  return result;
}

void ClientApp::startNode()
{
  // start the client.  if this return false then we've failed and
  // we shouldn't retry.
  LOG((CLOG_DEBUG1 "starting client"));
  if (!startClient()) {
    m_bye(kExitFailed);
  }
}

ISocketFactory *ClientApp::getSocketFactory() const
{
  return new TCPSocketFactory(m_events, getSocketMultiplexer());
}
