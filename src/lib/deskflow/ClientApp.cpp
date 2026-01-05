/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ClientApp.h"

#include "base/Event.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "client/Client.h"
#include "common/ExitCodes.h"
#include "common/PlatformInfo.h"
#include "common/Settings.h"
#include "deskflow/Screen.h"
#include "deskflow/ScreenException.h"
#include "net/NetworkAddress.h"
#include "net/SocketException.h"
#include "net/SocketMultiplexer.h"
#include "net/TCPSocketFactory.h"

#if SYSAPI_WIN32
#include "arch/win32/ArchDaemonWindows.h"
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

#include <memory>

constexpr static auto s_retryTime = 1.0;

ClientApp::ClientApp(IEventQueue *events, const QString &processName) : App(events, processName)
{
  // do nothing
}

void ClientApp::parseArgs()
{
  // save server address
  if (const auto address = Settings::value(Settings::Client::RemoteHost).toString(); !address.isEmpty()) {
    try {
      *m_serverAddress = NetworkAddress(address.toStdString(), Settings::value(Settings::Core::Port).toInt());
      m_serverAddress->resolve();
    } catch (SocketAddressException &e) {
      // allow an address that we can't look up if we're restartable.
      // we'll try to resolve the address each time we connect to the
      // server.  a bad port will never get better.  patch by Brent
      // Priddy.
      if (e.getError() == SocketAddressException::SocketError::BadPort) {
        LOG_CRIT("%s: %s" BYE, qPrintable(processName()), e.what(), qPrintable(processName()));
        bye(s_exitFailed);
      }
    }
  }
}

const char *ClientApp::daemonName() const
{
  if (deskflow::platform::isWindows())
    return "Deskflow Client";
  return "deskflow-client";
}

const char *ClientApp::daemonInfo() const
{
  if (deskflow::platform::isWindows())
    return "Allows another computer to share it's keyboard and mouse with this computer.";
  return "";
}

deskflow::Screen *ClientApp::createScreen()
{
  const bool invertScrolling = Settings::value(Settings::Client::InvertScrollDirection).toBool();
#if WINAPI_MSWINDOWS
  return new deskflow::Screen(
      new MSWindowsScreen(
          false, Settings::value(Settings::Core::UseHooks).toBool(), getEvents(),
          Settings::value(Settings::Client::LanguageSync).toBool(), invertScrolling
      ),
      getEvents()
  );
#endif

#if defined(WINAPI_XWINDOWS) or defined(WINAPI_LIBEI)
  if (deskflow::platform::isWayland()) {
#if WINAPI_LIBEI
    LOG_INFO("using ei screen for wayland");
    return new deskflow::Screen(new deskflow::EiScreen(false, getEvents(), true, invertScrolling), getEvents());
#else
    throw XNoEiSupport();
#endif
  }
#endif

#if WINAPI_XWINDOWS
  LOG_INFO("using legacy x windows screen");
  return new deskflow::Screen(
      new XWindowsScreen(
          qPrintable(Settings::value(Settings::Core::Display).toString()), false,
          Settings::value(Settings::Client::ScrollSpeed).toInt(), getEvents(), invertScrolling
      ),
      getEvents()
  );

#endif

#if WINAPI_CARBON
  return new deskflow::Screen(
      new OSXScreen(getEvents(), false, Settings::value(Settings::Client::LanguageSync).toBool(), invertScrolling),
      getEvents()
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

  if (!info->m_retry) {
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
  if (!m_suspended) {
    scheduleClientRestart(s_retryTime);
  }
}

Client *ClientApp::openClient(const std::string &name, const NetworkAddress &address, deskflow::Screen *screen)
{
  auto *client = new Client(getEvents(), name, address, getSocketFactory(), screen);

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
      m_client = openClient(
          Settings::value(Settings::Core::ScreenName).toString().toStdString(), *m_serverAddress, clientScreen
      );
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

  scheduleClientRestart(retryTime);
  return true;
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
#if SYSAPI_WIN32
  ArchDaemonWindows::daemonRunning(true);
#endif

#if WINAPI_CARBON

  Thread thread(new TMethodJob<ClientApp>(this, &ClientApp::runEventsLoop, nullptr));

  // wait until carbon loop is ready
  OSXScreen *screen = dynamic_cast<OSXScreen *>(m_clientScreen->getPlatformScreen());
  screen->waitForCarbonLoop();

  runCocoaApp();
#else
  getEvents()->loop();
#endif

#if SYSAPI_WIN32
  ArchDaemonWindows::daemonRunning(false);
#endif

  // close down
  LOG_DEBUG1("stopping client");
  stopClient();
  LOG_NOTE("stopped client");

  return s_exitSuccess;
}

int ClientApp::start()
{
  initApp();
  return mainLoop();
}

int ClientApp::runInner(StartupFunc startup)
{
  // general initialization
  m_serverAddress = new NetworkAddress;

  int result;
  try {
    // run
    result = startup();
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
