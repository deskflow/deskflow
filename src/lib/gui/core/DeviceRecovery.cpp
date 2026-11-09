// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "DeviceRecovery.h"
#include "DiscoveryCatalog.h"
#include "NearbyDiscovery.h"
#include "RecoveryProbe.h"
#include <QUuid>
namespace deskflow::gui {
DeviceRecovery::DeviceRecovery(
    IdentityBindings &bindings, NearbyDiscovery &discovery, QObject *parent, std::function<qint64()> clock
)
    : QObject(parent),
      m_bindings(bindings),
      m_clock(std::move(clock))
{
  m_elapsed.start();
  m_automatic = !m_clock;
  if (!m_clock)
    m_clock = [this] { return m_elapsed.elapsed(); };
  m_timer.setSingleShot(true);
  connect(&m_timer, &QTimer::timeout, this, &DeviceRecovery::tick);
  connect(
      &discovery, &NearbyDiscovery::found, this,
      [this](const QString &key, const QString &, const QString &host, int port) { candidate(key, host, port); }
  );
  connect(&discovery, &NearbyDiscovery::removed, this, &DeviceRecovery::removed);
  connect(&discovery, &NearbyDiscovery::searchingChanged, this, [this](bool active) {
    if (!running())
      return;
    if (active)
      m_searching = true;
    else if (m_searching)
      cancel();
  });
}
bool DeviceRecovery::running() const
{
  return m_state == State::Searching || m_state == State::Verifying || m_state == State::Backoff;
}
void DeviceRecovery::transition(State state)
{
  m_state = state;
  Q_EMIT stateChanged(state);
}
bool DeviceRecovery::begin(const QString &fp, const DiscoveryScope &scope)
{
  cancel();
  const auto normalized = IdentityBindings::normalizedFingerprint(fp);
  if (!scope.valid() || normalized.isEmpty() || !m_bindings.devices().contains(normalized))
    return false;
  m_scope = scope;
  m_fingerprint = normalized;
  m_tries = 0;
  m_due = m_clock();
  m_sessionDeadline = m_due + 120000;
  m_searching = false;
  transition(State::Searching);
  tick();
  return true;
}
void DeviceRecovery::cancelOwnedAuthentication()
{
  if (m_bindingGeneration && m_bindings.attempt().generation == m_bindingGeneration)
    m_bindings.cancel();
  m_bindingGeneration = 0;
}
void DeviceRecovery::cancel()
{
  ++m_generation;
  m_timer.stop();
  m_gate.cancel();
  cancelOwnedAuthentication();
  const auto old = m_context;
  m_context = {};
  m_current.clear();
  m_commitEndpoint = {};
  m_candidates.clear();
  m_attempted.clear();
  m_searching = false;
  transition(State::Cancelled);
  if (old.valid())
    Q_EMIT verificationCancelled(old);
}
void DeviceRecovery::interfaceChanged(const DiscoveryScope &scope)
{
  if (running() && scope != m_scope)
    cancel();
}
void DeviceRecovery::candidate(const QString &key, const QString &host, int port)
{
  if (!running() || key.isEmpty() || key.size() > 300 || !DiscoveryCatalog::validEndpoint(host, port))
    return;
  const auto canonical = DiscoveryCatalog::canonicalHost(host);
  const auto endpointKey = canonical + ':' + QString::number(port);
  if (m_candidates.contains(key) && m_candidates[key].endpoint != IdentityBindings::Endpoint{canonical, port})
    removed(key);
  if (!m_candidates.contains(key) && m_candidates.size() >= 32)
    return;
  m_candidates[key] = {{canonical, port}, m_clock() + 120000};
  if (!m_attempted.contains(endpointKey))
    tick();
}
void DeviceRecovery::removed(const QString &key)
{
  if (!running())
    return;
  const bool current = key == m_current;
  m_candidates.remove(key);
  if (current)
    abandon(true);
}
void DeviceRecovery::abandon(bool retryable)
{
  const auto old = m_context;
  m_context = {};
  m_gate.cancel();
  cancelOwnedAuthentication();
  m_current.clear();
  m_commitEndpoint = {};
  if (old.valid())
    Q_EMIT verificationCancelled(old);
  if (!retryable || m_tries >= 6 || m_clock() >= m_sessionDeadline) {
    m_timer.stop();
    transition(State::NeedsAction);
    return;
  }
  m_due = m_clock() + qMin(16000, 1000 * (1 << qMax(0, qMin(m_tries - 1, 4))));
  transition(State::Backoff);
  tick();
}
void DeviceRecovery::failed(const deskflow::auth::Context &context, bool retryable)
{
  if (m_state == State::Verifying && context == m_context)
    abandon(retryable);
}
void DeviceRecovery::tick()
{
  if (!running())
    return;
  const auto now = m_clock();
  if (now < 0 || now >= m_sessionDeadline) {
    abandon(false);
    return;
  }
  for (const auto &key : m_candidates.keys())
    if (m_candidates[key].expires <= now) {
      const bool current = key == m_current;
      m_candidates.remove(key);
      if (current) {
        abandon(true);
        return;
      }
    }
  if (m_state == State::Verifying) {
    if (now >= m_deadline) {
      abandon(true);
      return;
    }
  } else if (now >= m_due) {
    m_current.clear();
    for (auto it = m_candidates.cbegin(); it != m_candidates.cend(); ++it) {
      const auto key = it->endpoint.host + ':' + QString::number(it->endpoint.port);
      if (!m_attempted.contains(key)) {
        m_current = it.key();
        m_attempted.insert(key);
        break;
      }
    }
    if (!m_current.isEmpty()) {
      if (m_tries >= 6) {
        abandon(false);
        return;
      }
      ++m_tries;
      m_context = {QUuid::createUuid().toString(QUuid::Id128), m_fingerprint, ++m_generation};
      m_gate.begin(m_context);
      const auto endpoint = m_candidates[m_current].endpoint;
      m_commitEndpoint = endpoint;
      if (!m_bindings.beginAuthentication(m_context, endpoint)) {
        abandon(false);
        return;
      }
      m_bindingGeneration = m_bindings.attempt().generation;
      m_deadline = now + 10000;
      transition(State::Verifying);
      Q_EMIT verificationRequested(m_context, endpoint.host, endpoint.port, m_scope);
    } else
      transition(State::Searching);
  }
  if (m_automatic && running()) {
    const auto deadline = m_state == State::Verifying ? m_deadline
                          : m_state == State::Backoff ? m_due
                                                      : m_sessionDeadline;
    m_timer.start(static_cast<int>(qMax<qint64>(1, qMin(deadline, m_sessionDeadline) - m_clock())));
  }
}
bool DeviceRecovery::bindNumericEndpoint(const deskflow::auth::Context &context, const QString &numeric)
{
  if (m_state != State::Verifying || context != m_context || !m_candidates.contains(m_current) ||
      !validProbeAddress(m_scope, QHostAddress(numeric)) || QHostAddress(numeric).toString() != numeric)
    return false;
  m_commitEndpoint = {numeric, m_candidates[m_current].endpoint.port};
  const bool begun = m_bindings.beginAuthentication(context, m_commitEndpoint);
  m_bindingGeneration = begun ? m_bindings.attempt().generation : 0;
  return begun;
}
void DeviceRecovery::receive(const deskflow::auth::Report &report)
{
  if (m_state != State::Verifying || report.context != m_context || !m_candidates.contains(m_current))
    return;
  const auto candidate = m_candidates[m_current];
  if (m_clock() >= candidate.expires || m_clock() >= m_deadline || m_clock() >= m_sessionDeadline) {
    abandon(true);
    return;
  }
  if (!m_gate.consume(report))
    return;
  if (report.outcome == deskflow::auth::Outcome::Started) {
    m_bindings.consumeAuthentication(report);
    return;
  }
  if (report.outcome != deskflow::auth::Outcome::Verified) {
    abandon(
        report.outcome == deskflow::auth::Outcome::WrongIdentity ||
        report.outcome == deskflow::auth::Outcome::MissingCertificate
    );
    return;
  }
  if (!m_bindings.consumeAuthentication(report)) {
    abandon(false);
    return;
  }
  const auto committed = m_commitEndpoint;
  const auto fingerprint = m_fingerprint;
  m_timer.stop();
  m_context = {};
  m_current.clear();
  m_gate.cancel();
  transition(State::Recovered);
  Q_EMIT recovered(fingerprint, committed.host, committed.port);
}
} // namespace deskflow::gui
