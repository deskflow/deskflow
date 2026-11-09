// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "DiscoveryScope.h"
#include "gui/IdentityBindings.h"
#include <QElapsedTimer>
#include <QMap>
#include <QSet>
#include <QTimer>
namespace deskflow::gui {
class NearbyDiscovery;
// Discovery hints never grant trust. Only bound SecureSocket reports commit.
class DeviceRecovery : public QObject
{
  Q_OBJECT
public:
  enum class State
  {
    Idle,
    Searching,
    Verifying,
    Backoff,
    Recovered,
    NeedsAction,
    Cancelled
  };
  DeviceRecovery(IdentityBindings &, NearbyDiscovery &, QObject *parent = nullptr, std::function<qint64()> clock = {});
  bool begin(const QString &fingerprint, const DiscoveryScope &);
  void cancel();
  void interfaceChanged(const DiscoveryScope &);
  void receive(const deskflow::auth::Report &);
  bool bindNumericEndpoint(const deskflow::auth::Context &, const QString &numeric);
  void failed(const deskflow::auth::Context &, bool retryable);
  void tick();
  State state() const
  {
    return m_state;
  }
  deskflow::auth::Context attempt() const
  {
    return m_context;
  }
Q_SIGNALS:
  void verificationRequested(const deskflow::auth::Context &, const QString &host, int port, const DiscoveryScope &);
  void verificationCancelled(const deskflow::auth::Context &);
  void recovered(const QString &fingerprint, const QString &host, int port);
  void stateChanged(deskflow::gui::DeviceRecovery::State);

private:
  struct Candidate
  {
    IdentityBindings::Endpoint endpoint;
    qint64 expires = 0;
  };
  IdentityBindings &m_bindings;
  QMap<QString, Candidate> m_candidates;
  QSet<QString> m_attempted;
  DiscoveryScope m_scope;
  deskflow::auth::Context m_context;
  deskflow::auth::Gate m_gate;
  QString m_fingerprint, m_current;
  IdentityBindings::Endpoint m_commitEndpoint;
  QElapsedTimer m_elapsed;
  std::function<qint64()> m_clock;
  QTimer m_timer;
  bool m_automatic = true, m_searching = false;
  quint64 m_generation = 0, m_bindingGeneration = 0;
  void cancelOwnedAuthentication();
  int m_tries = 0;
  qint64 m_due = 0, m_deadline = 0, m_sessionDeadline = 0;
  State m_state = State::Idle;
  bool running() const;
  void transition(State);
  void candidate(const QString &, const QString &, int);
  void removed(const QString &);
  void abandon(bool retryable);
};
} // namespace deskflow::gui
Q_DECLARE_METATYPE(deskflow::gui::DeviceRecovery::State)
