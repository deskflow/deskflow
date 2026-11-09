// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "CoreProcess.h"
#include "DeviceRecovery.h"
#include "RecoveryProbe.h"
namespace deskflow::gui {
struct AutomaticRecoveryPolicy
{
  bool enabled = false;
  QString fingerprint;
  DiscoveryScope network;
  bool valid() const;
  QByteArray encode() const;
  static AutomaticRecoveryPolicy decode(const QByteArray &);
  std::optional<DiscoveryScope> current(const QVector<DiscoveryScope> &) const;
};
class AutomaticRecovery : public QObject
{
  Q_OBJECT
public:
  enum class State
  {
    Disabled,
    Paused,
    Watching,
    Debouncing,
    Stopping,
    Searching,
    WaitingNetwork,
    Resuming,
    NeedsAction
  };
  Q_ENUM(State)
  AutomaticRecovery(
      CoreProcess &, IdentityBindings &, NearbyDiscovery &, ScopedAddressResolver &, TlsProbeExecutor &,
      DiscoveryScopeSource, QObject *parent = nullptr, std::function<qint64()> clock = {}
  );
  bool configure(const AutomaticRecoveryPolicy &);
  bool prepareStart(const QString &host, int port);
  AutomaticRecoveryPolicy policy() const
  {
    return m_policy;
  }
  State state() const
  {
    return m_state;
  }
  void networkChanged();
  void powerChanged(bool sleeping);
  void cancel();
  void tick();
Q_SIGNALS:
  void stateChanged(deskflow::gui::AutomaticRecovery::State);
  void verifiedEndpoint(const QString &fingerprint, const QString &host, int port);

private:
  CoreProcess &m_core;
  IdentityBindings &m_bindings;
  NearbyDiscovery &m_discovery;
  TlsProbeExecutor &m_executor;
  DeviceRecovery m_recovery;
  RecoveryProbeBroker m_broker;
  DiscoveryScopeSource m_scopes;
  AutomaticRecoveryPolicy m_policy;
  State m_state = State::Disabled;
  QTimer m_timer;
  QElapsedTimer m_elapsed;
  std::function<qint64()> m_clock;
  bool m_realClock = false, m_sleeping = false, m_blocked = false, m_cycle = false, m_finishing = false;
  int m_rounds = 0;
  qint64 m_due = 0, m_deadline = 0, m_stopDeadline = 0, m_connectedAt = -1, m_sleepAt = 0;
  void trigger(int delay = 1500);
  void halt(State);
  void transition(State);
  void armTimer();
  void interrupted();
  bool eligible() const;
};
} // namespace deskflow::gui
Q_DECLARE_METATYPE(deskflow::gui::AutomaticRecovery::State)
