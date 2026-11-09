// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "ConnectionDiagnostics.h"
#include "gui/dialogs/DiagnosticExportDialog.h"
#include <QDateTime>
#include <QEvent>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMetaEnum>
#include <QPushButton>
#include <QTimer>

using deskflow::core::ConnectionRefusal;
using deskflow::core::ConnectionState;
using deskflow::core::ProcessState;

ConnectionDiagnostics::ConnectionDiagnostics(QWidget *parent) : QFrame(parent), m_timer(new QTimer(this))
{
  m_timer->setObjectName("diagnosticTimer");
  setObjectName("connectionDiagnostics");
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(16, 12, 16, 12);
  layout->setSpacing(6);
  auto *heading = new QHBoxLayout;
  m_state = new QLabel(this);
  m_state->setObjectName("diagnosticState");
  auto titleFont = font();
  titleFont.setPointSizeF(titleFont.pointSizeF() + 4);
  titleFont.setBold(true);
  m_state->setFont(titleFont);
  m_state->setWordWrap(true);
  m_state->setFixedHeight(m_state->fontMetrics().lineSpacing() * 2 + 4);
  m_role = new QLabel(this);
  m_role->setObjectName("diagnosticRole");
  m_role->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  heading->addWidget(m_state, 1);
  heading->addWidget(m_role);
  layout->addLayout(heading);
  m_peer = new QLabel(this);
  m_peer->setObjectName("diagnosticPeer");
  m_peer->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_peer->setMinimumWidth(1);
  auto peerFont = font();
  peerFont.setBold(true);
  m_peer->setFont(peerFont);
  layout->addWidget(m_peer);
  m_detail = new QLabel(this);
  m_detail->setObjectName("diagnosticReason");
  m_detail->setWordWrap(true);
  m_detail->setFixedHeight(m_detail->fontMetrics().lineSpacing() * 2 + 4);
  layout->addWidget(m_detail);
  m_metrics = new QLabel(this);
  m_metrics->setObjectName("diagnosticMetrics");
  m_metrics->setWordWrap(true);
  layout->addWidget(m_metrics);
  auto *actions = new QHBoxLayout;
  m_action = new QPushButton(this);
  m_action->setObjectName("diagnosticAction");
  m_action->setMinimumHeight(34);
  m_action->setProperty("primaryAction", true);
  m_showDetails = new QPushButton(tr("Details"), this);
  m_showDetails->setObjectName("diagnosticDetailsButton");
  m_showDetails->setMinimumHeight(34);
  m_showDetails->setCheckable(true);
  actions->addWidget(m_action);
  actions->addWidget(m_showDetails);
  m_export = new QPushButton(tr("Export diagnostics"), this);
  m_export->setObjectName("diagnosticExportButton");
  m_export->setMinimumHeight(34);
  actions->addWidget(m_export);
  actions->addStretch();
  connect(m_export, &QPushButton::clicked, this, [this] {
    DiagnosticExportDialog dialog(diagnosticSnapshot(), this);
    dialog.exec();
  });
  layout->addLayout(actions);
  m_details = new QLabel(this);
  m_details->setObjectName("diagnosticDetails");
  m_details->setWordWrap(true);
  m_details->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
  m_details->hide();
  layout->addWidget(m_details);
  for (auto *label : {m_state, m_role, m_peer, m_detail, m_metrics, m_details})
    label->setTextFormat(Qt::PlainText);
  connect(m_showDetails, &QPushButton::toggled, this, [this](bool shown) {
    m_details->setVisible(shown);
    render();
  });
  connect(m_action, &QPushButton::clicked, this, [this] {
    if (m_actionPending)
      return;
    if (phase() == Phase::Idle && !m_server) {
      Q_EMIT manualConnectionRequested();
      return;
    }
    if (phase() == Phase::NeedsAction) {
      if (m_attention == Attention::Recovery)
        Q_EMIT recoverySettingsRequested();
      else
        Q_EMIT settingsRequested();
      return;
    }
    m_actionPending = true;
    m_action->setEnabled(false);
    Q_EMIT reconnectRequested();
  });
  m_timer->setInterval(1000);
  m_timer->setTimerType(Qt::PreciseTimer); // Do not display an extra second after an early coarse wake-up.
  connect(m_timer, &QTimer::timeout, this, &ConnectionDiagnostics::render);
  render();
}

void ConnectionDiagnostics::record(const QString &event)
{
  // Allowlisted transition codes only. Never subscribe to raw logLine, keyboard,
  // clipboard, window titles, addresses or certificate bytes for this history.
  if (m_events.size() == 64)
    m_events.removeAt(0);
  m_events.append(
      QJsonObject{{"event", event}, {"atUtc", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}}
  );
}
void ConnectionDiagnostics::setIdleServerMode(bool server)
{
  if (phase() == Phase::Idle && m_server != server) {
    m_server = server;
    m_port = 0;
    render();
  }
}
void ConnectionDiagnostics::beginAttempt(bool server, const QString &address, int port, bool tls, bool peerChecks)
{
  // A retry can restart the core and emit a fresh attempt for the same endpoint.
  // Keep that attempt inside the existing recovery cycle instead of briefly
  // flipping the UI back to Connecting/Verifying between retry notifications.
  const auto boundedAddress = address.left(1024);
  const bool continuingRecovery = m_retryCycle && m_attention == Attention::None && !m_server && !server &&
                                  m_address == boundedAddress && m_port == port;
  m_userPaused = false;
  m_server = server;
  m_address = boundedAddress;
  m_port = port;
  m_tls = tls;
  m_peerChecks = peerChecks;
  m_active = true;
  m_secure = false;
  m_fingerprint = false;
  m_discovering = false;
  m_retryPending = continuingRecovery;
  m_retryCycle = continuingRecovery;
  m_actionPending = false;
  m_attention = Attention::None;
  m_clientCount = 0;
  m_process = ProcessState::Starting;
  m_connection = ConnectionState::Disconnected;
  if (!continuingRecovery) {
    m_deadline = QDeadlineTimer();
    m_timer->stop();
  }
  record("attempt-starting");
  render();
}
void ConnectionDiagnostics::processStateChanged(ProcessState state)
{
  m_process = state;
  if (state == ProcessState::Stopped || state == ProcessState::Stopping) {
    const bool continuingRecovery = m_retryCycle || m_retryPending;
    m_active = false;
    m_secure = false;
    m_fingerprint = false;
    m_discovering = false;
    m_retryPending = continuingRecovery;
    m_retryCycle = continuingRecovery;
    m_connection = ConnectionState::Disconnected;
    m_clientCount = 0;
    if (!continuingRecovery)
      m_timer->stop();
  }
  if (state == ProcessState::RetryPending)
    m_retryPending = m_retryCycle = true;
  m_actionPending = false;
  record(QString("process-%1").arg(QMetaEnum::fromType<ProcessState>().valueToKey(int(state))));
  render();
}
void ConnectionDiagnostics::connectionIntentChanged(bool wanted)
{
  m_userPaused = !wanted;
  if (!wanted) {
    m_attention = Attention::None;
    m_retryPending = false;
    m_retryCycle = false;
    m_discovering = false;
    m_active = false;
    m_actionPending = false;
    m_timer->stop();
  }
  record(wanted ? "user-started" : "user-paused");
  render();
}
void ConnectionDiagnostics::connectionStateChanged(ConnectionState state)
{
  if (!m_active)
    return; // Late IPC after a stop cannot resurrect a connection.
  if (state == ConnectionState::Disconnected) {
    m_secure = false;
    m_fingerprint = false;
    m_clientCount = 0;
  } else if (state == ConnectionState::Connecting && (m_connection != ConnectionState::Connecting || m_retryPending)) {
    m_secure = false;
    m_fingerprint = false;
    m_retryPending = false;
  } else if (state == ConnectionState::Connected) {
    m_retryPending = false;
    if (m_secure) {
      m_retryCycle = false;
      m_timer->stop();
    }
    if (m_attention != Attention::Identity)
      m_attention = Attention::None;
  }
  m_connection = state;
  record(QString("connection-%1").arg(QMetaEnum::fromType<ConnectionState>().valueToKey(int(state))));
  render();
}
void ConnectionDiagnostics::secureSocket(bool enabled)
{
  if (!m_active)
    return;
  m_secure = enabled;
  if (enabled && m_connection == ConnectionState::Connected) {
    m_retryPending = false;
    m_retryCycle = false;
    m_timer->stop();
  }
  record(enabled ? "tls-observed" : "tls-reset");
  render();
}
void ConnectionDiagnostics::peerFingerprintObserved()
{
  if (!m_active || m_connection == ConnectionState::Connected)
    return;
  m_fingerprint = true;
  m_retryPending = false;
  record("peer-verification-started");
  render();
}
void ConnectionDiagnostics::retryIn(int seconds)
{
  if (!m_active || m_server || m_connection == ConnectionState::Connected || m_process != ProcessState::Started)
    return;
  m_secure = false;
  m_fingerprint = false;
  m_retryPending = true;
  m_retryCycle = true;
  m_deadline.setRemainingTime(qint64(qMax(0, seconds)) * 1000);
  if (seconds > 0)
    m_timer->start();
  record("retry-scheduled");
  render();
}
void ConnectionDiagnostics::requireAttention(Attention reason)
{
  m_attention = reason;
  m_actionPending = false;
  record(QString("attention-%1").arg(QMetaEnum::fromType<Attention>().valueToKey(int(reason))));
  render();
}
void ConnectionDiagnostics::connectionRefused(ConnectionRefusal reason)
{
  if (!m_active)
    return;
  switch (reason) {
  case ConnectionRefusal::AlreadyConnected:
    requireAttention(Attention::DuplicateName);
    break;
  case ConnectionRefusal::UnknownClient:
    requireAttention(Attention::UnknownClient);
    break;
  case ConnectionRefusal::IncompatibleVersion:
    requireAttention(Attention::Version);
    break;
  case ConnectionRefusal::ProtocolError:
    requireAttention(Attention::Protocol);
    break;
  }
}
void ConnectionDiagnostics::connectedClientsChanged(const QStringList &clients)
{
  if (!m_active || !m_server)
    return;
  m_clientCount = clients.size();
  record("client-count-changed");
  render();
}
void ConnectionDiagnostics::setDiscoveryActive(bool active)
{
  m_discovering = active;
  record(active ? "discovery-started" : "discovery-stopped");
  render();
}
void ConnectionDiagnostics::setReconnectAllowed(bool allowed)
{
  m_reconnectAllowed = allowed;
  if (!allowed)
    m_actionPending = false;
  render();
}

ConnectionDiagnostics::Phase ConnectionDiagnostics::phase() const
{
  if (m_attention != Attention::None)
    return Phase::NeedsAction;
  if (m_process == ProcessState::Stopping)
    return Phase::Stopping;
  if (m_userPaused && !m_active)
    return Phase::Paused;
  if (m_retryCycle)
    return Phase::Recovering;
  if (m_process == ProcessState::RetryPending)
    return m_retryCycle ? Phase::Recovering : Phase::Retrying;
  if (!m_active)
    return m_discovering ? Phase::Discovering : Phase::Idle;
  if (m_process == ProcessState::Starting)
    return m_retryCycle ? Phase::Recovering : Phase::Starting;
  if ((m_connection == ConnectionState::Connected || (m_server && m_clientCount > 0)) && (!m_tls || !m_peerChecks))
    return Phase::NeedsAction;
  if (m_server && m_clientCount > 0)
    return Phase::Connected;
  if (m_connection == ConnectionState::Connected)
    return m_secure ? Phase::Connected : m_retryCycle ? Phase::Recovering : Phase::Verifying;
  if (m_retryPending)
    return Phase::Retrying;
  if (m_server && m_connection == ConnectionState::Listening)
    return Phase::Listening;
  if (m_fingerprint || m_secure)
    return Phase::Verifying;
  return Phase::Connecting;
}
QString ConnectionDiagnostics::phaseCode() const
{
  return QString::fromLatin1(QMetaEnum::fromType<Phase>().valueToKey(int(phase())));
}
QString ConnectionDiagnostics::summary() const
{
  switch (phase()) {
  case Phase::Paused:
    return tr("Paused by you");
  case Phase::Idle:
    return tr("Not connected");
  case Phase::Starting:
    return tr("Starting Deskflow");
  case Phase::Discovering:
    return tr("Finding nearby devices");
  case Phase::Connecting:
    return tr("Connecting");
  case Phase::Verifying:
    return tr("Verifying secure connection");
  case Phase::Recovering:
    return tr("Reconnecting and checking identity");
  case Phase::Connected:
    return tr("Connected");
  case Phase::Listening:
    return tr("Ready for a connection");
  case Phase::Retrying:
    return tr("Retrying connection");
  case Phase::Stopping:
    return tr("Disconnecting");
  case Phase::NeedsAction:
    return tr("Action needed");
  }
  return {};
}
QString ConnectionDiagnostics::detail() const
{
  if (phase() == Phase::NeedsAction) {
    switch (m_attention) {
    case Attention::Identity:
      return tr("Compare fingerprints on both computers before trusting this device.");
    case Attention::Recovery:
      return tr(
          "Automatic recovery is paused. Review the saved device, allowed network and prepared identity before "
          "retrying."
      );
    case Attention::Address:
      return tr("No server address is configured. Enter a hostname or choose a recent server.");
    case Attention::StartFailed:
      return tr(
          "The core could not start. Check the server layout and required system permissions; no permission is granted "
          "automatically."
      );
    case Attention::Daemon:
      return tr(
          "The background service is not responding to this window. Its network connection state is unknown. Review "
          "service settings."
      );
    case Attention::DuplicateName:
      return tr("Another client has this name. Check for duplicate processes or an old session.");
    case Attention::UnknownClient:
      return tr(
          "The server does not recognize this computer. Add its name in the server’s computer settings, then retry."
      );
    case Attention::Version:
      return tr(
          "The server rejected this protocol version. Check compatible Deskflow versions and protocol settings on both "
          "computers."
      );
    case Attention::Protocol:
      return tr("The server reported a protocol error. Review the versions and protocol settings on both computers.");
    default:
      return tr(
          "The core reports a connection without both TLS and peer checking enabled. Review security settings before "
          "relying on this connection."
      );
    }
  }
  switch (phase()) {
  case Phase::Paused:
    return tr("You stopped this connection. Automatic recovery stays off until you choose Connect again.");
  case Phase::Connected:
    return m_server
               ? tr("%1 client(s) reported by the core. Per-client encryption and input delivery are not measured here."
                 )
                     .arg(m_clientCount)
               : tr("The core reports a connection and TLS, with peer checking enabled. Move the pointer to confirm "
                    "input reaches this computer.");
  case Phase::Verifying:
    return tr("Waiting for the core’s connection and TLS confirmation. Input delivery has not been confirmed.");
  case Phase::Recovering:
    return m_deadline.remainingTime() > 0 ? tr("The previous attempt did not complete. The next retry is in %1 s; "
                                               "identity is checked again before input starts.")
                                                .arg((m_deadline.remainingTime() + 999) / 1000)
                                          : tr("Retrying now. The previous attempt did not complete; identity is "
                                               "checked again before input starts.");
  case Phase::Retrying: {
    const auto ms = m_deadline.remainingTime();
    return ms > 0 ? tr("The core will retry in %1 s. The last attempt did not complete; no detailed network cause was "
                       "reported.")
                        .arg((ms + 999) / 1000)
                  : tr("Retry is due. Waiting for a new core connection event; success has not been reported.");
  }
  case Phase::Listening:
    return tr(
        "This computer is waiting for clients. Confirm the client points to this server, then verify its identity when "
        "prompted."
    );
  case Phase::Connecting:
    return tr(
        "Waiting for a connection from the core. Check the peer address and local network if this continues; the cause "
        "is not yet known."
    );
  case Phase::Discovering:
    return tr(
        "Searching for candidate addresses. Finding a device does not authorize a connection or trust its identity."
    );
  case Phase::Starting:
    return tr("Starting the core process. A running process alone does not confirm a network connection.");
  case Phase::Stopping:
    return tr("Waiting for the core to stop. Late connection events are ignored.");
  default:
    return m_server ? tr("Open My desktop → Add computer to review local network discovery settings.")
                    : tr("Choose a nearby or recent computer, or select Manual connection to enter an address.");
  }
}
QByteArray ConnectionDiagnostics::diagnosticSnapshot() const
{
  return QJsonDocument(
             QJsonObject{
                 {"schema", 1},
                 {"phase", phaseCode()},
                 {"server", m_server},
                 {"coreProcessState", int(m_process)},
                 {"coreConnectionState", int(m_connection)},
                 {"tlsConfigured", m_tls},
                 {"peerChecksConfigured", m_peerChecks},
                 {"tlsObserved", m_secure},
                 {"attention", int(m_attention)},
                 {"inputDeliveryVerified", false},
                 {"latencyMeasured", false},
                 {"events", m_events}
             }
  ).toJson(QJsonDocument::Compact);
}
void ConnectionDiagnostics::refreshPeer()
{
  QString text = m_server              ? tr("This computer · port %1").arg(m_port)
                 : m_address.isEmpty() ? tr("No active server")
                                       : tr("Server · %1 · port %2").arg(m_address).arg(m_port);
  m_peer->setText(m_peer->fontMetrics().elidedText(text, Qt::ElideMiddle, qMax(1, m_peer->width())));
  m_peer->setToolTip(text);
  m_peer->setAccessibleName(text);
}
void ConnectionDiagnostics::render()
{
  const auto currentPhase = phase();
  if (m_deadline.hasExpired() || (currentPhase != Phase::Retrying && currentPhase != Phase::Recovering))
    m_timer->stop();
  const bool dark = palette().color(QPalette::Window).lightness() < 128;
  const auto accent = phase() == Phase::Connected     ? (dark ? "#70e0b4" : "#176c50")
                      : phase() == Phase::NeedsAction ? (dark ? "#ffcc80" : "#8a430c")
                                                      : (dark ? "#92bafe" : "#275cad");
  const auto style = QString(
                         "QFrame#connectionDiagnostics { background:%1; border:1px solid %2; border-left:4px solid %3; "
                         "border-radius:12px; }"
  )
                         .arg(dark ? "#242c39" : "#ffffff", dark ? "#48576b" : "#ccd7e5", accent);
  if (styleSheet() != style)
    setStyleSheet(style);
  m_state->setText(summary());
  m_state->setFixedHeight(m_state->fontMetrics().lineSpacing() * 2 + 4);
  m_showDetails->setText(tr("Details"));
  m_state->setAccessibleName(summary());
  m_role->setText(
      m_port == 0 ? tr("No active role")
      : m_server  ? tr("SERVER · sends input")
                  : tr("CLIENT · receives input")
  );
  m_role->setVisible(m_port > 0);
  m_peer->setVisible(m_port > 0);
  m_detail->setText(detail());
  m_detail->setFixedHeight(m_detail->fontMetrics().lineSpacing() * 2 + 4);
  m_metrics->setText(tr("Latency: not measured"));
  m_metrics->setVisible(m_showDetails->isChecked() && m_port > 0);
  m_export->setVisible(m_showDetails->isChecked());
  const auto p = phase();
  const bool manual = p == Phase::Idle && !m_server;
  const bool action =
      manual || p == Phase::Retrying || p == Phase::Recovering || p == Phase::NeedsAction || p == Phase::Connecting;
  m_action->setVisible(action);
  m_action->setEnabled(action && !m_actionPending && (manual || p == Phase::NeedsAction || m_reconnectAllowed));
  m_action->setText(
      p == Phase::NeedsAction ? (m_attention == Attention::Recovery ? tr("Review recovery") : tr("Review settings"))
      : manual                ? tr("Manual connection")
                              : tr("Retry now")
  );
  const auto privacy =
      tr("Diagnostics keep up to 64 state codes. No typed text, clipboard, addresses or fingerprints are exported.");
  m_details->setText(
      p == Phase::Connected
          ? tr("Connected but unable to cross screens? Check mouse capture, the lock hotkey, Spaces or app privileges."
            ) + "\n\n" +
                privacy
          : privacy
  );
  // A keyboard-focus border adds two pixels under the shared style. Keep
  // the full styled height even when a narrow window compresses this row.
  for (auto *button : findChildren<QPushButton *>(QString(), Qt::FindDirectChildrenOnly))
    button->setMinimumHeight(qMax(36, button->fontMetrics().height() + 16));
  refreshPeer();
  Q_EMIT summaryChanged(summary());
}
void ConnectionDiagnostics::resizeEvent(QResizeEvent *event)
{
  QFrame::resizeEvent(event);
  refreshPeer();
}
void ConnectionDiagnostics::changeEvent(QEvent *event)
{
  QFrame::changeEvent(event);
  if (event->type() == QEvent::PaletteChange || event->type() == QEvent::LanguageChange ||
      event->type() == QEvent::FontChange)
    render();
}
