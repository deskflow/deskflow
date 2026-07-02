/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "AutoModeRunner.h"

#include "base/EventQueue.h"
#include "base/Log.h"
#include "common/ExitCodes.h"
#include "common/Settings.h"
#include "coordination/Coordinator.h"
#include "coordination/FleetState.h"
#include "coordination/Peer.h"
#include "deskflow/ClientApp.h"
#include "deskflow/DeskflowException.h"
#include "deskflow/DisplayInvalidException.h"
#include "deskflow/ServerApp.h"

#include <QThread>

#include <chrono>
#include <functional>
#include <thread>

using deskflow::coordination::Coordinator;
using deskflow::coordination::CoordinatorConfig;
using deskflow::coordination::FleetState;
using deskflow::coordination::Role;
using deskflow::coordination::RoleDecision;

namespace {

QStringList preConnectHostsFromFleet(const FleetState &fleet, const std::string &serverAddress)
{
  QStringList hosts;
  const auto addHost = [&hosts](const std::string &address) {
    if (address.empty()) {
      return;
    }
    const auto host = QString::fromStdString(address);
    if (!hosts.contains(host)) {
      hosts << host;
    }
  };

  addHost(serverAddress);
  for (const auto &peer : fleet.peers) {
    addHost(peer.lan);
    addHost(peer.ip);
  }
  return hosts;
}

QStringList defaultPreConnectHosts(const std::string &serverAddress)
{
  QStringList hosts;
  if (!serverAddress.empty()) {
    const auto primary = QString::fromStdString(serverAddress);
    if (!hosts.contains(primary)) {
      hosts << primary;
    }
  }
  const auto peers = deskflow::coordination::parsePeerList(
      Settings::value(Settings::Coordination::Peers).toStringList().join(QLatin1Char(',')).toStdString()
  );
  for (const auto &peer : peers) {
    if (!peer.lan.empty()) {
      const auto lan = QString::fromStdString(peer.lan);
      if (!hosts.contains(lan)) {
        hosts << lan;
      }
    }
    if (!peer.ip.empty()) {
      const auto ip = QString::fromStdString(peer.ip);
      if (!hosts.contains(ip)) {
        hosts << ip;
      }
    }
  }
  return hosts;
}

CoordinatorConfig configFromSettings()
{
  CoordinatorConfig config;
  config.selfName = Settings::value(Settings::Core::ComputerName).toString().toStdString();
  config.meshPort = Settings::value(Settings::Coordination::Port).toInt();
  config.deskflowPort = Settings::value(Settings::Core::Port).toInt();
  config.token = Settings::value(Settings::Coordination::Token).toString().toStdString();
  // QSettings turns comma-separated INI values into a QStringList; accept
  // both that and a plain string by normalizing through a list join.
  config.peers = deskflow::coordination::parsePeerList(
      Settings::value(Settings::Coordination::Peers).toStringList().join(QLatin1Char(',')).toStdString()
  );
  const auto followCursor = Settings::value(Settings::Coordination::KeyboardFollowCursor);
  config.keyboardFollowCursor = followCursor.isValid() ? followCursor.toBool() : true;
  config.meshVersion = Settings::value(Settings::Coordination::MeshVersion).toInt();
  return config;
}

} // namespace

AutoModeRunner::AutoModeRunner(EventQueue &events, QString processName)
    : m_events(events),
      m_processName(std::move(processName))
{
  // do nothing
}

AutoModeRunner::~AutoModeRunner() = default;

void AutoModeRunner::run(QThread &coreThread)
{
  LOG_INFO("starting core in auto (coordinated) mode");
  QObject::connect(&coreThread, &QThread::started, [this, &coreThread] {
    epochLoop();
    coreThread.quit();
  });
  coreThread.start();
}

void AutoModeRunner::requestQuit()
{
  if (m_coordinator) {
    m_coordinator->requestQuit();
  }
}

void AutoModeRunner::epochLoop()
{
  const auto config = configFromSettings();
  if (config.selfName.empty() || config.peers.empty()) {
    LOG_CRIT(
        "auto mode requires core/computerName and coordination/peers settings "
        "(computerName=\"%s\" peers=\"%s\" from %s)",
        config.selfName.c_str(), qPrintable(Settings::value(Settings::Coordination::Peers).toString()),
        qPrintable(Settings::settingsFile())
    );
    m_exitCode = s_exitFailed;
    return;
  }

  m_coordinator = std::make_unique<Coordinator>(config);
  m_coordinator->setInterruptCallback([this] {
    // Only interrupt an actually running app; a stale Quit in the queue
    // would otherwise instantly kill the next epoch.
    if (m_appRunning) {
      m_events.addEvent(Event(EventTypes::Quit));
    }
  });
  if (!m_coordinator->start()) {
    LOG_CRIT("auto mode could not start the coordination mesh");
    m_exitCode = s_exitFailed;
    return;
  }
  m_coordinator->setEventQueue(&m_events);

  while (true) {
    const RoleDecision decision = m_coordinator->awaitRoleDecision();
    if (decision.quit) {
      break;
    }

    const int result = runEpoch(decision.role, decision.serverAddress);
    if (result != s_exitSuccess) {
      LOG_WARN("coordination: %s epoch ended with code %d", roleName(decision.role), result);
      // Pause briefly so a persistent failure cannot hot-loop. Plain
      // std sleep: Arch::sleep() requires an Arch-registered thread and
      // this is a QThread (it crashes in testCancelThread otherwise).
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    m_coordinator->notifyEpochEnded();
  }

  m_coordinator->stop();
  LOG_INFO("auto mode stopped");
}

int AutoModeRunner::runEpoch(Role role, const std::string &serverAddress)
{
  const auto meshVersion = Settings::value(Settings::Coordination::MeshVersion).toInt();
  const bool meshV2 = meshVersion >= 2;

  if (role == Role::Client) {
    QStringList hosts = defaultPreConnectHosts(serverAddress);
    if (meshV2) {
      const auto fleet = m_coordinator->fleetSnapshot();
      if (!fleet.links.empty()) {
        hosts = preConnectHostsFromFleet(fleet, serverAddress);
      }
    }
    Settings::setValue(Settings::Client::RemoteHost, hosts.join(QLatin1Char(',')));
  }

  m_coordinator->updateKeyboardRelayForRole(role);

  // Screen enter/leave handlers are scoped to this client epoch so stale
  // events from a prior epoch cannot set cursorScreenKnown after reset.
  const bool trackCursorHere = role == Role::Client;
  if (trackCursorHere) {
    m_events.addHandler(EventTypes::CoordinationScreenEntered, m_events.getSystemTarget(), [this](const auto &) {
      m_coordinator->notifyCursorHere(true);
    });
    m_events.addHandler(EventTypes::CoordinationScreenLeft, m_events.getSystemTarget(), [this](const auto &) {
      m_coordinator->notifyCursorHere(false);
    });
  }

  bool trackTopologyReady = false;
  ClientApp *clientAppPtr = nullptr;
  std::function<void(const Event &)> topologyReadyHandler;

  std::unique_ptr<App> app;
  if (role == Role::Server) {
    auto serverApp = std::make_unique<ServerApp>(&m_events, m_processName);
    serverApp->setCursorBroadcastCallback([this](const std::string &screenName) {
      m_coordinator->updateCursorHost(screenName);
    });
    serverApp->setFleetTopologyPublishCallback([this](auto links, auto screens) {
      m_coordinator->publishFleetTopology(std::move(links), std::move(screens));
    });
    serverApp->setFleetSnapshotCallback([this] { return m_coordinator->fleetSnapshot(); });
    app = std::move(serverApp);
  } else {
    auto clientApp = std::make_unique<ClientApp>(&m_events, m_processName);
    clientAppPtr = clientApp.get();
    if (meshV2) {
      topologyReadyHandler = [this, clientAppPtr, serverAddress](const Event &) {
        const auto fleet = m_coordinator->fleetSnapshot();
        if (fleet.links.empty()) {
          return;
        }
        clientAppPtr->appendPreConnectHosts(preConnectHostsFromFleet(fleet, serverAddress));
      };
      m_events.addHandler(
          EventTypes::CoordinationTopologyReady, m_events.getSystemTarget(), topologyReadyHandler
      );
      trackTopologyReady = true;
    }
    app = std::move(clientApp);
  }
  LOG_INFO(
      "coordination: starting %s epoch%s%s", roleName(role), serverAddress.empty() ? "" : " towards ",
      serverAddress.c_str()
  );

  m_appRunning = true;
  // A decision can land in the gap before this epoch's loop starts;
  // re-post the interrupt so it is never lost.
  if (m_coordinator->hasPendingDecision()) {
    m_events.addEvent(Event(EventTypes::Quit));
  }

  int result = s_exitFailed;
  try {
    result = app->runSynchronously();
  } catch (ExitAppException &e) {
    result = e.getCode();
  } catch (DisplayInvalidException &die) {
    LOG_CRIT("a display invalid exception error occurred: %s\n", die.what());
    std::this_thread::sleep_for(std::chrono::seconds(10));
  } catch (std::runtime_error &re) {
    LOG_CRIT("a runtime error occurred: %s\n", re.what());
  } catch (std::exception &e) {
    LOG_CRIT("an error occurred: %s\n", e.what());
  } catch (...) {
    LOG_CRIT("an unknown error occurred\n");
  }
  m_appRunning = false;

  if (trackCursorHere) {
    m_events.removeHandler(EventTypes::CoordinationScreenEntered, m_events.getSystemTarget());
    m_events.removeHandler(EventTypes::CoordinationScreenLeft, m_events.getSystemTarget());
  }
  if (trackTopologyReady) {
    m_events.removeHandler(EventTypes::CoordinationTopologyReady, m_events.getSystemTarget());
  }

  m_coordinator->updateKeyboardRelayForRole(Role::Init);

  m_exitCode = result;
  return result;
}
