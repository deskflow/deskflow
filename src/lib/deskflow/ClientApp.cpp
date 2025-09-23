/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ClientApp.h"

#include "arch/Arch.h"
#include "base/Event.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "client/Client.h"
#include "common/Constants.h"
#include "common/ExitCodes.h"
#include "deskflow/ArgParser.h"
#include "deskflow/ClientArgs.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/Screen.h"
#include "deskflow/ScreenException.h"
#include "net/NetworkAddress.h"
#include "net/SocketException.h"
#include "net/SocketMultiplexer.h"
#include "net/TCPSocketFactory.h"
#include "platform/Wayland.h"

#if SYSAPI_WIN32
#include "arch/win32/ArchMiscWindows.h"
#endif

#if WINAPI_MSWINDOWS
#include "platform/MSWindowsScreen.h"
#endif

#include <QFileInfo> // Must include before XWindowsScreen to avoid conflicts with xlib.h

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

#include <memory>
#include <sstream>
#include <stdio.h>

constexpr static auto s_retryTime = 1.0;

ClientApp::ClientApp(IEventQueue *events) : App(events, new deskflow::ClientArgs())
{
  // do nothing
}

void ClientApp::parseArgs(int argc, const char *const *argv)
{
  ArgParser argParser(this);
  bool result = argParser.parseClientArgs(args(), argc, argv);

  if (!result || args().m_shouldExitOk || args().m_shouldExitFail) {
    if (args().m_shouldExitOk) {
      bye(s_exitSuccess);
    } else {
      bye(s_exitArgs);
    }
  } else {
    // save server address
    if (!args().m_serverAddress.empty()) {
      try {
        *m_serverAddress = NetworkAddress(args().m_serverAddress, kDefaultPort);
        m_serverAddress->resolve();
      } catch (SocketAddressException &e) {
        // allow an address that we can't look up if we're restartable.
        // we'll try to resolve the address each time we connect to the
        // server.  a bad port will never get better.  patch by Brent
        // Priddy.
        if (!args().m_restartable || e.getError() == SocketAddressException::SocketError::BadPort) {
          LOG_CRIT("%s: %s" BYE, args().m_pname, e.what(), args().m_pname);
          bye(s_exitFailed);
        }
      }
    }
  }
}

void ClientApp::help()
{
  std::stringstream help;
  help << "\n\nClient Mode:\n\n"
       << "Usage: " << kAppId << "-core client"
       << " [--address <address>]"
       << " [--yscroll <delta>]"
       << " [--sync-language]"
       << " [--invert-scroll]"
#ifdef WINAPI_XWINDOWS
       << " [--display <display>]"
#endif
       << s_helpCommonArgs << " <server-address>"
       << "\n\n"
       << "Connect to a " << kAppName << " mouse/keyboard sharing server.\n"
       << "\n"
       << "  -a, --address <address>  local network interface address.\n"
       << s_helpGeneralArgs << s_helpSysInfo << "      --yscroll <delta>    defines the vertical scrolling delta,\n"
       << "                             which is 120 by default.\n"
       << "      --sync-language      enable language synchronization.\n"
       << "      --invert-scroll      invert scroll direction on this\n"
       << "                             computer.\n"
#if WINAPI_XWINDOWS
       << "      --display <display>  when in X mode, connect to the X server\n"
       << "                             at <display>.\n"
#endif
       << s_helpVersionArgs << "\n"
       << "* marks defaults.\n"

       << s_helpNoWayland

       << "\n"
       << "The server address is of the form: [<hostname>][:<port>].\n"
       << "The hostname must be the address or hostname of the server.\n"
       << "The port overrides the default port, " << kDefaultPort << ".\n";

  LOG_PRINT("%s", help.str().c_str());
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
          false, args().m_noHooks, getEvents(), args().m_enableLangSync, args().m_clientScrollDirection
      ),
      getEvents()
  );
#endif

#if defined(WINAPI_XWINDOWS) or defined(WINAPI_LIBEI)
  if (deskflow::platform::isWayland()) {
#if WINAPI_LIBEI
    LOG_INFO("using ei screen for wayland");
    return new deskflow::Screen(
        new deskflow::EiScreen(false, getEvents(), true, args().m_clientScrollDirection), getEvents()
    );
#else
    throw XNoEiSupport();
#endif
  }
#endif

#if WINAPI_XWINDOWS
  LOG_INFO("using legacy x windows screen");
  return new deskflow::Screen(
      new XWindowsScreen(args().m_display, false, args().m_yscroll, getEvents(), args().m_clientScrollDirection),
      getEvents()
  );

#endif

#if WINAPI_CARBON
  return new deskflow::Screen(
      new OSXScreen(getEvents(), false, args().m_enableLangSync, args().m_clientScrollDirection), getEvents()
  );
#endif
}

deskflow::Screen *ClientApp::openClientScreen()
{
  deskflow::Screen *screen = createScreen();
  getEvents()->addHandler(EventTypes::ScreenError, screen->getEventTarget(), [this](const auto &) {
    handleScreenError();
  });
  return screen;
}

void ClientApp::closeClientScreen(deskflow::Screen *screen)
{
  if (screen != nullptr) {
    getEvents()->removeHandler(EventTypes::ScreenError, screen->getEventTarget());
    delete screen;
  }
}

void ClientApp::handleClientRestart(const Event &, EventQueueTimer *timer)
{
  // discard old timer
  getEvents()->deleteTimer(timer);
  getEvents()->removeHandler(EventTypes::Timer, timer);

  // reconnect
  startClient();
}

void ClientApp::scheduleClientRestart(double retryTime)
{
  // install a timer and handler to retry later
  LOG_DEBUG("retry in %.0f seconds", retryTime);
  EventQueueTimer *timer = getEvents()->newOneShotTimer(retryTime, nullptr);
  getEvents()->addHandler(EventTypes::Timer, timer, [this, timer](const auto &e) { handleClientRestart(e, timer); });
}

void ClientApp::handleClientConnected() const
{
  LOG_IPC("connected to server");
}

void ClientApp::handleClientFailed(const Event &e)
{
  if ((++m_lastServerAddressIndex) < m_client->getLastResolvedAddressesCount()) {
    std::unique_ptr<Client::FailInfo> info(static_cast<Client::FailInfo *>(e.getData()));

    LOG_WARN("failed to connect to server=%s, trying next address", info->m_what.c_str());
    if (!m_suspended) {
      scheduleClientRestart(s_retryTime);
    }
  } else {
    m_lastServerAddressIndex = 0;
    handleClientRefused(e);
  }
}

void ClientApp::handleClientRefused(const Event &e)
{
  std::unique_ptr<Client::FailInfo> info(static_cast<Client::FailInfo *>(e.getData()));

  if (!args().m_restartable || !info->m_retry) {
    LOG_ERR("failed to connect to server: %s", info->m_what.c_str());
    getEvents()->addEvent(Event(EventTypes::Quit));
  } else {
    LOG_WARN("failed to connect to server: %s", info->m_what.c_str());
    if (!m_suspended) {
      scheduleClientRestart(s_retryTime);
    }
  }
}

void ClientApp::handleClientDisconnected()
{
  LOG_IPC("disconnected from server");
  if (!args().m_restartable) {
    getEvents()->addEvent(Event(EventTypes::Quit));
  } else if (!m_suspended) {
    scheduleClientRestart(s_retryTime);
  }
}

Client *ClientApp::openClient(const std::string &name, const NetworkAddress &address, deskflow::Screen *screen)
{
  auto *client = new Client(getEvents(), name, address, getSocketFactory(), screen, args());

  try {
    getEvents()->addHandler(EventTypes::ClientConnected, client->getEventTarget(), [this](const auto &) {
      handleClientConnected();
    });
    getEvents()->addHandler(EventTypes::ClientConnectionFailed, client->getEventTarget(), [this](const auto &e) {
      handleClientFailed(e);
    });
    getEvents()->addHandler(EventTypes::ClientConnectionRefused, client->getEventTarget(), [this](const auto &e) {
      handleClientRefused(e);
    });
    getEvents()->addHandler(EventTypes::ClientDisconnected, client->getEventTarget(), [this](const auto &) {
      handleClientDisconnected();
    });

  } catch (std::bad_alloc &ba) {
    delete client;
    throw ba;
  }

  return client;
}

void ClientApp::closeClient(Client *client)
{
  if (client == nullptr) {
    return;
  }
  using enum EventTypes;
  getEvents()->removeHandler(ClientConnected, client);
  getEvents()->removeHandler(ClientConnectionFailed, client);
  getEvents()->removeHandler(ClientConnectionRefused, client);
  getEvents()->removeHandler(ClientDisconnected, client);
  delete client;
}

bool ClientApp::startClient()
{
  double retryTime;
  deskflow::Screen *clientScreen = nullptr;
  try {
    if (m_clientScreen == nullptr) {
      clientScreen = openClientScreen();
      m_client = openClient(args().m_name, *m_serverAddress, clientScreen);
      m_clientScreen = clientScreen;
      LOG_NOTE("started client");
    }

    m_client->connect(m_lastServerAddressIndex);

    return true;
  } catch (ScreenUnavailableException &e) {
    LOG_WARN("secondary screen unavailable: %s", e.what());
    closeClientScreen(clientScreen);
    retryTime = e.getRetryTime();
  } catch (ScreenOpenFailureException &e) {
    LOG_CRIT("failed to start client: %s", e.what());
    closeClientScreen(clientScreen);
    return false;
  } catch (BaseException &e) {
    LOG_CRIT("failed to start client: %s", e.what());
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
  m_client = nullptr;
  m_clientScreen = nullptr;
}

int ClientApp::mainLoop()
{
  // create socket multiplexer.  this must happen after daemonization
  // on unix because threads evaporate across a fork().
  setSocketMultiplexer(std::make_unique<SocketMultiplexer>());

  // start client, etc
  appUtil().startNode();

  // run event loop.  if startClient() failed we're supposed to retry
  // later.  the timer installed by startClient() will take care of
  // that.
  DAEMON_RUNNING(true);

#if WINAPI_CARBON

  Thread thread(new TMethodJob<ClientApp>(this, &ClientApp::runEventsLoop, nullptr));

  // wait until carbon loop is ready
  OSXScreen *screen = dynamic_cast<OSXScreen *>(m_clientScreen->getPlatformScreen());
  screen->waitForCarbonLoop();

  runCocoaApp();
#else
  getEvents()->loop();
#endif

  DAEMON_RUNNING(false);

  // close down
  LOG_DEBUG1("stopping client");
  stopClient();
  LOG_NOTE("stopped client");

  return s_exitSuccess;
}

static int daemonMainLoopStatic(int argc, const char **argv)
{
  return ClientApp::instance().daemonMainLoop(argc, argv);
}

int ClientApp::standardStartup(int argc, char **argv)
{
  initApp(argc, argv);
  return mainLoop();
}

int ClientApp::runInner(int argc, char **argv, StartupFunc startup)
{
  // general initialization
  m_serverAddress = new NetworkAddress;
  args().m_pname = QFileInfo(argv[0]).fileName().toLocal8Bit().constData();

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
  LOG_DEBUG1("starting client");
  if (!startClient()) {
    bye(s_exitFailed);
  }
}

ISocketFactory *ClientApp::getSocketFactory() const
{
  return new TCPSocketFactory(getEvents(), getSocketMultiplexer());
}
