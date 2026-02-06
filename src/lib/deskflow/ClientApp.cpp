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
  // save server addresses (comma-separated list supported)
  if (const auto addressList = Settings::value(Settings::Client::RemoteHost).toString(); !addressList.isEmpty()) {
    const int port = Settings::value(Settings::Core::Port).toInt();
    const QStringList addresses = addressList.split(',', Qt::SkipEmptyParts);

    for (const QString &addr : addresses) {
      const QString trimmedAddr = addr.trimmed();
      if (trimmedAddr.isEmpty()) {
        continue;
      }

      try {
        NetworkAddress netAddr(trimmedAddr.toStdString(), port);
        netAddr.resolve();
        m_serverAddresses.append(netAddr);
        LOG_DEBUG("added server address: %s", qPrintable(trimmedAddr));
      } catch (SocketAddressException &e) {
        // allow an address that we can't look up if we're restartable.
        // we'll try to resolve the address each time we connect to the
        // server.  a bad port will never get better.
        if (e.getError() == SocketAddressException::SocketError::BadPort) {
          LOG_CRIT("%s: %s" BYE, qPrintable(processName()), e.what(), qPrintable(processName()));
          bye(s_exitFailed);
        } else {
          // Still add it - we'll try to resolve later
          NetworkAddress netAddr(trimmedAddr.toStdString(), port);
          m_serverAddresses.append(netAddr);
          LOG_WARN("could not resolve address '%s': %s (will retry later)", qPrintable(trimmedAddr), e.what());
        }
      }
    }

    if (m_serverAddresses.isEmpty()) {
      LOG_CRIT("%s: no valid server addresses specified" BYE, qPrintable(processName()), qPrintable(processName()));
      bye(s_exitFailed);
    }

    LOG_NOTE("configured %zu server address(es)", static_cast<size_t>(m_serverAddresses.size()));
  }
}

const char *ClientApp::daemonName() const
{
  if (deskflow::platform::isWindows())
    return "Deskflow Client";
  return "deskflow-client";
}

deskflow::Screen *ClientApp::createScreen()
{
#if WINAPI_MSWINDOWS
  return new deskflow::Screen(
      new MSWindowsScreen(
          false, Settings::value(Settings::Core::UseHooks).toBool(), getEvents(),
          Settings::value(Settings::Client::LanguageSync).toBool()
      ),
      getEvents()
  );
#endif

#if defined(WINAPI_XWINDOWS) or defined(WINAPI_LIBEI)
  if (deskflow::platform::isWayland()) {
#if WINAPI_LIBEI
    LOG_INFO("using ei screen for wayland");
    return new deskflow::Screen(new deskflow::EiScreen(false, getEvents(), true), getEvents());
#else
    throw XNoEiSupport();
#endif
  }
#endif

#if WINAPI_XWINDOWS
  LOG_INFO("using legacy x windows screen");
  return new deskflow::Screen(
      new XWindowsScreen(qPrintable(Settings::value(Settings::Core::Display).toString()), false, getEvents()),
      getEvents()
  );

#endif

#if WINAPI_CARBON
  return new deskflow::Screen(
      new OSXScreen(getEvents(), false, Settings::value(Settings::Client::LanguageSync).toBool()), getEvents()
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

void ClientApp::handleClientConnected()
{
  LOG_IPC("connected to server");
  // Reset server index on successful connection
  m_currentServerIndex = 0;
  m_lastServerAddressIndex = 0;
}

void ClientApp::handleClientFailed(const Event &e)
{
  if ((++m_lastServerAddressIndex) < m_client->getLastResolvedAddressesCount()) {
    // Try next resolved address for current hostname
    std::unique_ptr<Client::FailInfo> info(static_cast<Client::FailInfo *>(e.getData()));

    LOG_WARN("failed to connect to server=%s, trying next resolved address", info->m_what.c_str());
    if (!m_suspended) {
      scheduleClientRestart(s_retryTime);
    }
  } else {
    // All resolved addresses exhausted, try next server in list
    m_lastServerAddressIndex = 0;
    tryNextServer();

    if (m_currentServerIndex == 0) {
      // We've cycled through all servers, treat as refused
      handleClientRefused(e);
    } else {
      std::unique_ptr<Client::FailInfo> info(static_cast<Client::FailInfo *>(e.getData()));
      LOG_WARN("failed to connect to server=%s, trying next server in list", info->m_what.c_str());
      if (!m_suspended) {
        scheduleClientRestart(s_retryTime);
      }
    }
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
          Settings::value(Settings::Core::ComputerName).toString().toStdString(), getCurrentServerAddress(),
          clientScreen
      );
      m_clientScreen = clientScreen;
      LOG_NOTE("started client");
    }

    m_client->setServerAddress(getCurrentServerAddress());
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

#if WINAPI_CARBON

  Thread thread(new TMethodJob<ClientApp>(this, &ClientApp::runEventsLoop, nullptr));

  // wait until carbon loop is ready
  OSXScreen *screen = dynamic_cast<OSXScreen *>(m_clientScreen->getPlatformScreen());
  screen->waitForCarbonLoop();

  runCocoaApp();
#else
  getEvents()->loop();
#endif

  // close down
  LOG_DEBUG("stopping client");
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
  int result;
  try {
    // run
    result = startup();
  } catch (...) {
    throw;
  }

  return result;
}

NetworkAddress &ClientApp::getCurrentServerAddress()
{
  if (m_serverAddresses.isEmpty()) {
    throw std::runtime_error("No server addresses configured");
  }
  return m_serverAddresses[m_currentServerIndex];
}

void ClientApp::tryNextServer()
{
  if (m_serverAddresses.size() > 1) {
    m_currentServerIndex = (m_currentServerIndex + 1) % m_serverAddresses.size();
    LOG_DEBUG("switching to server %zu of %zu", m_currentServerIndex + 1, m_serverAddresses.size());
  }
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
