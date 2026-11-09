// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "MainWindow.h"
#include "gui/TlsUtility.h"
#include "gui/dialogs/ServerConfigDialog.h"
#include "gui/widgets/ActionButton.h"
#include "gui/widgets/ConnectionHome.h"
#include "gui/widgets/DesktopWorkspaceView.h"
#include "ui_MainWindow.h"
#include <QApplication>
#include <QLabel>
#include <QMessageBox>
#include <QScopeGuard>
#include <QScreen>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <limits>
#if defined(Q_OS_MACOS) && !defined(DESKFLOW_LAB_BUILD)
#include <ApplicationServices/ApplicationServices.h>
#endif

using namespace deskflow::gui;
using namespace deskflow::display;

void MainWindow::setupDesktopWorkspace()
{
  m_desktopWorkspace = new DesktopWorkspaceView(m_home);
  m_desktopWorkspace->setProperty("externalAddAction", true);
  m_home->setDesktopWidget(m_desktopWorkspace);
#if defined(Q_OS_MACOS)
  auto *permission = new QWidget(m_desktopWorkspace);
  permission->setObjectName("receiverPermissionSummary");
  auto *permissionRow = new QHBoxLayout(permission);
  permissionRow->setContentsMargins(0, 0, 0, 0);
  auto *permissionText = new QLabel(permission);
  permissionText->setObjectName("receiverPermissionText");
  permissionText->setWordWrap(true);
  permissionText->setTextFormat(Qt::PlainText);
  auto *check = new ActionButton(tr("Check permission"), permission);
  check->setObjectName("receiverCheckPermission");
  permissionRow->addWidget(permissionText, 1);
  permissionRow->addWidget(check);
  qobject_cast<QVBoxLayout *>(m_desktopWorkspace->layout())->insertWidget(3, permission);
  connect(check, &QPushButton::clicked, this, &MainWindow::refreshDesktopWorkspace);
#endif
  connect(
      m_desktopWorkspace, &DesktopWorkspaceView::applyRequested, this,
      [this](const QByteArray &state, const QByteArray &layout) { saveDesktopWorkspace(state, layout, true); }
  );
  connect(m_desktopWorkspace, &DesktopWorkspaceView::saveDraftRequested, this, [this](const QByteArray &state) {
    saveDesktopWorkspace(state, {}, false);
  });
  connect(
      &m_coreProcess, &CoreProcess::workspaceApplyFinished, this,
      [this](const QString &id, const QString &outcome, const QString &reason) {
        if (id != m_workspacePendingRequest)
          return;
        m_workspacePendingRequest.clear();
        if (outcome == "applied") {
          const bool recorded = Settings::saveConfig([this] {
            Settings::setValue(Settings::Server::WorkspaceApplied, m_workspacePendingState);
          });
          m_workspaceSaveStatus = recorded ? tr("The current sharing core is using the last confirmed arrangement.")
                                           : tr("Applied now, but the confirmed arrangement could not be saved for "
                                                "restart. The previous restart arrangement is preserved.");
        } else if (outcome == "rejected")
          m_workspaceSaveStatus =
              tr("Saved draft; the core rejected this change (%1). The previous active arrangement is preserved.")
                  .arg(reason);
        else
          m_workspaceSaveStatus = tr("Saved draft; the application result is unknown (%1). Review the current "
                                     "connection before applying again.")
                                      .arg(reason);
        m_workspacePendingState.clear();
        refreshDesktopWorkspace();
      }
  );
  connect(m_desktopWorkspace, &DesktopWorkspaceView::addComputerRequested, this, &MainWindow::showAddComputer);
  connect(
      &m_coreProcess, &CoreProcess::workspaceSnapshotReceived, this,
      [this](const QString &controller, const QByteArray &bytes, const QString &fingerprint) {
        const auto decoded = WorkspaceSnapshot::decode(bytes);
        if (!decoded)
          return;
        m_workspaceSnapshot = decoded;
        m_workspaceController = controller;
        m_workspaceSourceFingerprint = fingerprint;
        m_workspaceLive = true;
        if (m_coreProcess.mode() == CoreMode::Server) {
          m_desktopWorkspace->setAppliedLayout(
              decoded->applied.layout.placements.isEmpty() ? QByteArray() : decoded->applied.layout.encode()
          );
          if (!decoded->planComplete &&
              m_workspaceSaveStatus == tr("The current sharing core is using the last confirmed arrangement."))
            m_workspaceSaveStatus =
                tr("The saved arrangement is retained. Some screen routes are unavailable after a connection or screen "
                   "change.");
        }
        if (m_coreProcess.mode() == CoreMode::Server) {
          auto cache = decodeTopologies(Settings::value(Settings::Server::MonitorTopologies).toByteArray());
          for (auto it = decoded->topologies.cbegin(); it != decoded->topologies.cend(); ++it)
            cache[it.key()] = *it;
          Settings::setValue(Settings::Server::MonitorTopologies, encodeTopologies(cache));
        }
        refreshDesktopWorkspace();
      }
  );
  connect(&m_coreProcess, &CoreProcess::processStateChanged, this, [this](ProcessState state) {
    if (state == ProcessState::Starting || state == ProcessState::Stopping || state == ProcessState::Stopped) {
      m_workspaceLive = false;
      m_workspaceSourceFingerprint.clear();
    }
    refreshDesktopWorkspace();
  });
  connect(&m_coreProcess, &CoreProcess::connectionStateChanged, this, [this](ConnectionState state) {
    if (m_coreProcess.mode() == CoreMode::Client && state != ConnectionState::Connected) {
      m_workspaceLive = false;
      m_workspaceSourceFingerprint.clear();
    }
    refreshDesktopWorkspace();
  });
#ifndef DESKFLOW_LAB_BUILD
  const auto capture = [this] {
    m_localWorkspaceTopology = localTopology();
    refreshDesktopWorkspace();
  };
  const auto observe = [this, capture](QScreen *screen) {
    connect(screen, &QScreen::geometryChanged, this, capture);
    connect(screen, &QScreen::physicalDotsPerInchChanged, this, capture);
  };
  for (auto *screen : QGuiApplication::screens())
    observe(screen);
  connect(qApp, &QGuiApplication::screenAdded, this, [capture, observe](QScreen *screen) {
    observe(screen);
    capture();
  });
  connect(qApp, &QGuiApplication::screenRemoved, this, [capture](QScreen *) { capture(); });
  capture();
#endif
  refreshDesktopWorkspace();
}

void MainWindow::refreshDesktopWorkspace()
{
  if (!m_desktopWorkspace || m_workspaceSaving)
    return;
  const auto role = m_coreProcess.mode();
  const auto local = Settings::value(Settings::Core::ComputerName).toString();
  if (role != m_workspaceRole) {
    m_workspaceRole = role;
    m_workspaceSnapshot.reset();
    m_workspaceLive = false;
    m_loadedWorkspace.clear();
    m_loadedWorkspaceLayout.clear();
    m_workspaceSaveStatus.clear();
  }
  const bool controller = role == CoreMode::Server;
#if defined(Q_OS_MACOS)
  findChild<QWidget *>("receiverPermissionSummary")->setVisible(!controller);
#ifdef DESKFLOW_LAB_BUILD
  findChild<QLabel *>("receiverPermissionText")
      ->setText(tr("System permission is not checked in this isolated review."));
#else
  findChild<QLabel *>("receiverPermissionText")
      ->setText(
          AXIsProcessTrusted()
              ? tr("Input control permission is granted to this application.")
              : tr("macOS does not trust this running application. Review its identity in Accessibility settings, then "
                   "check again. This check does not request or reset permission.")
      );
#endif
#endif
  DeviceSnapshots devices;
  if (controller) {
    m_serverConfig.ensureLocalServer();
    const auto cache = decodeTopologies(Settings::value(Settings::Server::MonitorTopologies).toByteArray());
    for (const auto &computer : std::as_const(m_serverConfig).computers()) {
      if (computer.isNull())
        continue;
      DeviceSnapshot d;
      d.id = computer.name();
      d.displayName = computer.name();
      d.local = d.id == local;
      const auto legacy = m_serverConfig.desktopLayout();
      if (legacy.error.isEmpty())
        for (const auto &entry : legacy.desktops)
          if (entry.name == d.id.toStdString() && !entry.displayName.empty())
            d.displayName = QString::fromStdString(entry.displayName);
      const bool online = m_workspaceLive && m_workspaceSnapshot && m_workspaceSnapshot->onlineDevices.contains(d.id);
      d.connection = d.local || online ? Connection::Live : Connection::Offline;
      d.authenticated = online && TlsUtility::isEnabled() && Settings::value(Settings::Security::CheckPeers).toBool();
      if (m_workspaceLive && m_workspaceSnapshot)
        d.identityReference = m_workspaceSnapshot->identityReferences.value(d.id);
#ifndef DESKFLOW_LAB_BUILD
      if (d.local && d.identityReference.isEmpty())
        d.identityReference = QString::fromLatin1(
            TlsUtility::certFingerprint(Settings::value(Settings::Security::Certificate).toString()).toHex()
        );
#endif
      if (d.local) {
        d.topologyCapability = d.routingCapability = Capability::Supported;
        if (m_localWorkspaceTopology.error().isEmpty()) {
          d.topology = m_localWorkspaceTopology;
          d.observation = Observation::Observed;
        } else {
          d.observation = Observation::Unavailable;
          d.reason = tr("System screen detection is unavailable in this session. No dimensions have been guessed.");
        }
      } else if (online && m_workspaceSnapshot->topologies.contains(d.id)) {
        d.topology = m_workspaceSnapshot->topologies[d.id];
        d.observation = Observation::Observed;
        d.topologyCapability = d.routingCapability = Capability::Supported;
      } else if (online && !m_workspaceSnapshot->topologyCapable.contains(d.id)) {
        d.observation = Observation::Unsupported;
        d.topologyCapability = d.routingCapability = Capability::Unsupported;
        if (m_workspaceSnapshot->legacyDesktops.contains(d.id)) {
          const auto desktop = m_workspaceSnapshot->legacyDesktops[d.id];
          d.reason = tr("Compatibility mode reports a %1 × %2 input-unit desktop. Individual screen count and physical "
                        "size are unknown; update this computer for screen arrangement.")
                         .arg(desktop.width())
                         .arg(desktop.height());
        }
      } else if (cache.contains(d.id)) {
        d.topology = cache[d.id];
        d.observation = Observation::Cached;
      }
      devices.append(d);
    }
    m_desktopWorkspace->setRole(DesktopWorkspaceView::Role::Controller);
    m_desktopWorkspace->setSnapshot(devices);
    const auto saved = m_serverConfig.workspaceState(), layout = m_serverConfig.monitorLayoutBytes();
    if (saved != m_loadedWorkspace || layout != m_loadedWorkspaceLayout) {
      m_desktopWorkspace->setDraft(
          saved, m_workspaceSnapshot && !m_workspaceSnapshot->applied.layout.placements.isEmpty()
                     ? m_workspaceSnapshot->applied.layout.encode()
                     : QByteArray()
      );
      m_loadedWorkspace = saved;
      m_loadedWorkspaceLayout = layout;
    }
    m_desktopWorkspace->setReadOnlyReason(
        m_serverConfig.useExternalConfig() ? tr("An external configuration file controls sharing. Edit that file in "
                                                "Settings; this canvas cannot overwrite it.")
        : !m_workspacePendingRequest.isEmpty()
            ? tr("Waiting for the current core to confirm this plan. Your draft is saved.")
            : QString()
    );
    m_desktopWorkspace->setSubmitAvailable(
        Settings::isWritable() && !m_serverConfig.useExternalConfig() && m_workspacePendingRequest.isEmpty()
    );
    m_desktopWorkspace->setActionsAvailable(!m_serverConfig.useExternalConfig(), false);
    // A cached acknowledgement describes a previous core, not a stopped session.
    if (!m_workspaceLive)
      m_desktopWorkspace->setActiveRouting(
          tr("Sharing is paused. The saved arrangement has not been confirmed by the running core.")
      );
    else if (!m_workspaceSaveStatus.isEmpty())
      m_desktopWorkspace->setActiveRouting(m_workspaceSaveStatus);
    else if (m_workspaceSnapshot->perDisplayActive)
      m_desktopWorkspace->setActiveRouting(
          tr("The running core reports individual-screen routing. Save confirmation is shown separately.")
      );
    else
      m_desktopWorkspace->setActiveRouting(
          tr("The running core is using compatibility routes. Individual-screen routing has not been confirmed.")
      );
  } else {
    const bool effective = m_workspaceLive && !m_workspaceSourceFingerprint.isEmpty() && m_workspaceSnapshot &&
                           m_workspaceSnapshot->perDisplayActive &&
                           !m_workspaceSnapshot->applied.layout.placements.isEmpty();
    if (effective)
      for (const auto &id : m_workspaceSnapshot->devices) {
        bool placed = false;
        for (const auto &p : m_workspaceSnapshot->applied.layout.placements)
          if (p.device == id) {
            placed = true;
            break;
          }
        if (!placed)
          continue; // Never auto-position un-applied devices in a receiver view.
        DeviceSnapshot d;
        d.id = id;
        d.displayName = id;
        d.local = id == local;
        d.connection = m_workspaceSnapshot->onlineDevices.contains(id) ? Connection::Live : Connection::Offline;
        if (m_workspaceSnapshot->topologies.contains(id)) {
          d.topology = m_workspaceSnapshot->topologies[id];
          d.observation = Observation::Observed;
        } else
          d.observation = Observation::Unknown;
        // Relayed device names are not local trust records. Receiver mode is read-only.
        devices.append(d);
      }
    const auto controllerName = m_home->state().peerName.isEmpty() ? m_workspaceController : m_home->state().peerName;
    m_desktopWorkspace->setRole(DesktopWorkspaceView::Role::Receiver, controllerName, effective);
    m_desktopWorkspace->setSnapshot(devices);
    m_desktopWorkspace->setDraft(
        effective ? m_workspaceSnapshot->applied.encode() : QByteArray(),
        effective ? m_workspaceSnapshot->applied.layout.encode() : QByteArray()
    );
    m_desktopWorkspace->setActionsAvailable(false, false);
    m_desktopWorkspace->setSubmitAvailable(false);
    m_desktopWorkspace->setActiveRouting(
        effective
            ? tr("Read-only arrangement reported by the verified controller.")
            : tr("Screen arrangement is managed on the controller. No current applied arrangement has been received.")
    );
  }
}

void MainWindow::saveDesktopWorkspace(const QByteArray &state, const QByteArray &layout, bool apply)
{
  if (m_coreProcess.mode() != CoreMode::Server || m_serverConfig.useExternalConfig() || m_workspaceSaving) {
    m_desktopWorkspace->rejectSave(tr("This computer cannot edit the active arrangement."));
    return;
  }
  if (state != m_desktopWorkspace->workspace().state().encode()) {
    m_desktopWorkspace->rejectSave(tr("The screen snapshot changed. Review the draft before saving."));
    return;
  }
  if (apply) {
    const auto candidate = m_desktopWorkspace->workspace().candidateForApply();
    if (!candidate || candidate->encode() != layout) {
      m_desktopWorkspace->rejectSave(tr("The current screens do not support this route plan. The draft is retained."));
      return;
    }
  }
  const auto previous = m_serverConfig;
  auto next = WorkspaceState::decode(state);
  if (!next || next->revision == std::numeric_limits<quint64>::max()) {
    m_desktopWorkspace->rejectSave(tr("The configuration version is invalid."));
    return;
  }
  ++next->revision;
  const auto savedState = next->encode();
  if (!m_serverConfig.setWorkspace(savedState, layout, apply)) {
    m_desktopWorkspace->rejectSave(
        tr("The arrangement is invalid. Your draft and previous configuration are preserved.")
    );
    return;
  }
  m_workspaceSaving = true;
  const bool saved = Settings::saveConfig([this] { m_serverConfig.commit(); });
  m_workspaceSaving = false;
  if (!saved) {
    m_serverConfig = previous;
    m_desktopWorkspace->rejectSave(
        tr("The file could not be saved. Your draft and previous configuration are preserved.")
    );
    return;
  }
  m_loadedWorkspace = savedState;
  m_loadedWorkspaceLayout = m_serverConfig.monitorLayoutBytes();
  const auto active = m_workspaceSnapshot && !m_workspaceSnapshot->applied.layout.placements.isEmpty()
                          ? m_workspaceSnapshot->applied.layout.encode()
                          : QByteArray();
  m_desktopWorkspace->acknowledgeSaved(savedState, active);
  m_workspaceSaveStatus =
      apply ? tr("Saved. Waiting for the core to confirm this arrangement; it is not yet marked active.")
            : tr("Draft saved. The active input routes have not changed.");
  if (apply) {
    m_workspacePendingState = savedState;
    m_workspacePendingRequest = m_coreProcess.applyWorkspace(savedState);
    if (m_workspacePendingRequest.isEmpty()) {
      m_workspacePendingState.clear();
      m_workspaceSaveStatus =
          tr("Saved; not yet marked active. Start or update the sharing core, then apply this arrangement.");
    }
  }
  refreshDesktopWorkspace();
}

void MainWindow::showDesktopArrangement()
{
  if (m_coreProcess.mode() != CoreMode::Server)
    return;
  m_homePages->setCurrentWidget(m_home);
  refreshDesktopWorkspace();
  m_desktopWorkspace->beginEdit();
}

void MainWindow::showAdvancedServerSettings()
{
  if (m_coreProcess.mode() != CoreMode::Server)
    return;
  ServerConfigDialog dialog(this, m_serverConfig);
  dialog.showAdvancedOnly();
  if (dialog.exec() == QDialog::Accepted) {
    refreshDesktopWorkspace();
    if (m_coreProcess.isStarted())
      m_coreProcess.restart();
  }
}
