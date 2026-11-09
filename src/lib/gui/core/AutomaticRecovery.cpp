// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "AutomaticRecovery.h"
#include "NearbyDiscovery.h"
#include <QJsonDocument>
namespace deskflow::gui {
bool AutomaticRecoveryPolicy::valid() const
{
  return !enabled ||
         (deskflow::auth::fingerprint(fingerprint) == fingerprint && !fingerprint.isEmpty() && network.valid());
}
QByteArray AutomaticRecoveryPolicy::encode() const
{
  if (!valid())
    return {};
  return QJsonDocument(
             QJsonObject{
                 {"schema", 1},
                 {"enabled", enabled},
                 {"identity", fingerprint},
                 {"index", int(network.index)},
                 {"name", network.name},
                 {"address", network.address.toString()},
                 {"prefix", network.prefix}
             }
  ).toJson(QJsonDocument::Compact);
}
AutomaticRecoveryPolicy AutomaticRecoveryPolicy::decode(const QByteArray &bytes)
{
  if (bytes.size() > 2048)
    return {};
  const auto d = QJsonDocument::fromJson(bytes);
  if (!d.isObject())
    return {};
  const auto o = d.object();
  AutomaticRecoveryPolicy p{
      o["enabled"].toBool(),
      o["identity"].toString(),
      {quint32(o["index"].toInt()), o["name"].toString(), QHostAddress(o["address"].toString()), o["prefix"].toInt()}
  };
  return o.size() == 7 && o["schema"] == 1 && o["enabled"].isBool() && p.valid() && p.encode() == bytes
             ? p
             : AutomaticRecoveryPolicy{};
}
std::optional<DiscoveryScope> AutomaticRecoveryPolicy::current(const QVector<DiscoveryScope> &scopes) const
{
  if (!enabled || !valid())
    return {};
  const auto mask = 0xffffffffu >> network.prefix;
  for (const auto &s : scopes)
    if (s.valid() && s.index == network.index && s.name == network.name && s.prefix == network.prefix &&
        (s.address.toIPv4Address() & ~mask) == (network.address.toIPv4Address() & ~mask))
      return s;
  return {}; // A different adapter or subnet requires a fresh explicit choice.
}
AutomaticRecovery::AutomaticRecovery(
    CoreProcess &core, IdentityBindings &bindings, NearbyDiscovery &discovery, ScopedAddressResolver &resolver,
    TlsProbeExecutor &executor, DiscoveryScopeSource scopes, QObject *parent, std::function<qint64()> clock
)
    : QObject(parent),
      m_core(core),
      m_bindings(bindings),
      m_discovery(discovery),
      m_executor(executor),
      m_recovery(bindings, discovery, this, clock),
      m_broker(m_recovery, resolver, executor, scopes, this),
      m_scopes(std::move(scopes)),
      m_clock(std::move(clock))
{
  m_elapsed.start();
  m_realClock = !m_clock;
  if (!m_clock)
    m_clock = [this] { return m_elapsed.elapsed(); };
  m_timer.setSingleShot(true);
  connect(&m_timer, &QTimer::timeout, this, &AutomaticRecovery::tick);
  connect(&core, &CoreProcess::connectionIntentChanged, this, [this](bool wanted) {
    if (!wanted) {
      cancel();
      return;
    }
    m_blocked = false;
    m_cycle = false;
    m_rounds = 0;
    trigger(5000);
  });
  connect(&core, &CoreProcess::connectionStateChanged, this, [this](auto state) {
    if (state == deskflow::core::ConnectionState::Connected) {
      m_connectedAt = m_clock();
      halt(State::Watching);
    } else if (m_state != State::Stopping && m_state != State::Searching)
      trigger(5000);
  });
  connect(
      &core, &CoreProcess::processStateChanged, this,
      [this](auto) {
        if (m_state == State::Stopping)
          tick();
      },
      Qt::QueuedConnection
  );
  connect(&core, &CoreProcess::retryIn, this, [this](int) {
    if (!m_cycle)
      trigger(5000);
  });
  connect(&core, &CoreProcess::connectionRefused, this, [this] {
    if (m_policy.enabled && m_core.connectionRequested() && m_core.expectedPeerFingerprint() == m_policy.fingerprint) {
      m_blocked = true;
      halt(State::NeedsAction);
    }
  });
  connect(&core, &CoreProcess::error, this, [this] {
    if (m_policy.enabled && m_core.connectionRequested() && m_core.expectedPeerFingerprint() == m_policy.fingerprint) {
      m_blocked = true;
      halt(State::NeedsAction);
    }
  });
  connect(&core, &CoreProcess::inputExecutionUnavailable, this, [this] {
    if (m_policy.enabled && m_core.connectionRequested() && m_core.expectedPeerFingerprint() == m_policy.fingerprint) {
      m_blocked = true;
      halt(State::NeedsAction);
    }
  });
  connect(&core, &CoreProcess::authenticationResult, this, [this](const deskflow::auth::Report &r) {
    if (m_policy.enabled && r.context.expected == m_policy.fingerprint &&
        r.outcome != deskflow::auth::Outcome::Started && r.outcome != deskflow::auth::Outcome::Verified) {
      m_blocked = true;
      halt(State::NeedsAction);
    }
  });
  connect(&core, &CoreProcess::powerStateChanged, this, &AutomaticRecovery::powerChanged);
  connect(&m_recovery, &DeviceRecovery::stateChanged, this, [this](auto state) {
    if (m_finishing || m_state != State::Searching)
      return;
    if (state == DeviceRecovery::State::NeedsAction) {
      m_blocked = true;
      halt(State::NeedsAction);
    } else if (state == DeviceRecovery::State::Cancelled)
      interrupted();
  });
  connect(&m_recovery, &DeviceRecovery::recovered, this, [this](const QString &fp, const QString &host, int port) {
    if (m_state != State::Searching || !eligible() || !m_core.recoveryQuiescent() || fp != m_policy.fingerprint)
      return;
    m_finishing = true;
    m_discovery.stop();
    m_finishing = false;
    transition(State::Resuming);
    m_timer.stop();
    Q_EMIT verifiedEndpoint(fp, host, port);
    if (!m_core.resumeForAddressRecovery(fp, host, port)) {
      m_blocked = true;
      halt(State::NeedsAction);
    }
  });
}
bool AutomaticRecovery::configure(const AutomaticRecoveryPolicy &p)
{
  if (!p.valid() || (p.enabled && !m_bindings.devices().contains(p.fingerprint)))
    return false;
  cancel();
  m_policy = p;
  m_blocked = false;
  m_cycle = false;
  m_rounds = 0;
  transition(!p.enabled ? State::Disabled : m_core.connectionRequested() ? State::Watching : State::Paused);
  if (m_core.connectionRequested())
    trigger(5000);
  return true;
}
bool AutomaticRecovery::prepareStart(const QString &host, int port)
{
  if (!m_policy.enabled || !m_policy.valid() || !m_bindings.devices().contains(m_policy.fingerprint) ||
      !m_core.recoveryQuiescent() || m_core.connectionRequested() || m_core.mode() != Settings::CoreMode::Client ||
      !Settings::value(Settings::Security::TlsEnabled).toBool() ||
      !Settings::value(Settings::Security::CheckPeers).toBool())
    return false;
  const auto endpoint = m_bindings.devices().value(m_policy.fingerprint).endpoint;
  if (endpoint != IdentityBindings::Endpoint{host, port} ||
      (!m_core.expectedPeerFingerprint().isEmpty() && m_core.expectedPeerFingerprint() != m_policy.fingerprint))
    return false;
  return m_core.setExpectedPeerFingerprint(m_policy.fingerprint);
}
bool AutomaticRecovery::eligible() const
{
  return m_policy.enabled && m_policy.valid() && !m_blocked && !m_sleeping && m_core.recoveryPermitted() &&
         m_core.expectedPeerFingerprint() == m_policy.fingerprint &&
         m_bindings.devices().contains(m_policy.fingerprint);
}
void AutomaticRecovery::transition(State s)
{
  if (m_state != s) {
    m_state = s;
    Q_EMIT stateChanged(s);
  }
}
void AutomaticRecovery::halt(State s)
{
  m_timer.stop();
  m_finishing = true;
  m_recovery.cancel();
  m_discovery.stop();
  m_finishing = false;
  transition(s);
}
void AutomaticRecovery::cancel()
{
  halt(m_policy.enabled ? State::Paused : State::Disabled);
  m_cycle = false;
  m_rounds = 0;
}
void AutomaticRecovery::trigger(int delay)
{
  if (!eligible() || m_core.connectionState() == deskflow::core::ConnectionState::Connected)
    return;
  const auto now = m_clock();
  if (m_connectedAt >= 0 && now - m_connectedAt >= 30000) {
    m_cycle = false;
    m_rounds = 0;
  }
  m_connectedAt = -1;
  if (!m_cycle) {
    m_cycle = true;
    m_deadline = now + 120000;
  }
  if (now >= m_deadline || m_rounds >= 3) {
    m_blocked = true;
    halt(State::NeedsAction);
    return;
  }
  if (m_state == State::Searching || m_state == State::Stopping || m_state == State::Debouncing)
    return;
  m_due = now + delay;
  transition(State::Debouncing);
  armTimer();
}
void AutomaticRecovery::armTimer()
{
  if (m_realClock)
    m_timer.start(int(qMax<qint64>(1, qMin(m_due, m_deadline) - m_clock())));
}
void AutomaticRecovery::interrupted()
{
  if (!eligible()) {
    halt(State::Paused);
    return;
  }
  halt(State::WaitingNetwork);
  trigger(qMin(16000, 1000 * (1 << qMin(m_rounds, 4))));
}
void AutomaticRecovery::networkChanged()
{
  if (m_state == State::Searching || m_state == State::Stopping || m_state == State::Debouncing)
    halt(State::WaitingNetwork);
  trigger();
}
void AutomaticRecovery::powerChanged(bool sleeping)
{
  if (m_sleeping == sleeping)
    return;
  m_sleeping = sleeping;
  if (sleeping) {
    m_sleepAt = m_clock();
    halt(State::Paused);
  } else {
    if (m_cycle)
      m_deadline += qMax<qint64>(0, m_clock() - m_sleepAt);
    trigger();
  }
}
void AutomaticRecovery::tick()
{
  if (m_blocked || !m_cycle)
    return;
  if (!eligible()) {
    halt(m_policy.enabled ? State::Paused : State::Disabled);
    return;
  }
  if (m_clock() >= m_deadline) {
    m_blocked = true;
    halt(State::NeedsAction);
    return;
  }
  if (m_core.connectionState() == deskflow::core::ConnectionState::Connected) {
    halt(State::Watching);
    return;
  }
  if (m_state == State::Searching) {
    m_recovery.tick();
    if (m_state == State::Searching) {
      m_due = m_deadline;
      armTimer();
    }
    return;
  }
  if (m_state != State::Stopping && m_clock() < m_due) {
    armTimer();
    return;
  }
  const auto scope = m_policy.current(m_scopes());
  if (!scope) {
    halt(State::WaitingNetwork);
    m_due = m_deadline;
    armTimer();
    return;
  }
  // Do not stop the old input process if probing itself cannot run.
  if (!m_executor.ready()) {
    m_blocked = true;
    halt(State::NeedsAction);
    return;
  }
  if (m_state != State::Stopping) {
    m_stopDeadline = m_clock() + 5000;
    transition(State::Stopping);
    if (!m_core.holdForAddressRecovery()) {
      m_blocked = true;
      halt(State::NeedsAction);
      return;
    }
  }
  if (!m_core.recoveryQuiescent()) {
    if (m_clock() >= m_stopDeadline) {
      m_blocked = true;
      halt(State::NeedsAction);
      return;
    }
    m_due = qMin(m_stopDeadline, m_deadline);
    armTimer();
    return;
  }
  if (++m_rounds > 3) {
    m_blocked = true;
    halt(State::NeedsAction);
    return;
  }
  // start() emits a stop notification first; begin the recovery after that.
  m_finishing = true;
  m_discovery.start(*scope);
  m_finishing = false;
  if (!m_recovery.begin(m_policy.fingerprint, *scope)) {
    m_blocked = true;
    halt(State::NeedsAction);
    return;
  }
  transition(State::Searching);
  m_due = m_deadline;
  armTimer();
}
} // namespace deskflow::gui
