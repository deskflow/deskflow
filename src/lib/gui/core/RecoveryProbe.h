// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "DiscoveryScope.h"
#include "common/PeerAuthentication.h"
#include "common/RuntimeProfile.h"
#include <QElapsedTimer>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <memory>
namespace deskflow::lab {
class CredentialLease;
}
namespace deskflow::gui {
class DeviceRecovery;
using ServiceNameSource = std::function<QStringList(const QString &, int)>;
bool validProbeAddress(const DiscoveryScope &, const QHostAddress &);
class ScopedAddressResolver : public QObject
{
  Q_OBJECT
public:
  using QObject::QObject;
  virtual void start(const deskflow::auth::Context &, const DiscoveryScope &, const QString &host, int port) = 0;
  virtual void cancel() = 0;
Q_SIGNALS:
  void addresses(
      const deskflow::auth::Context &, const DiscoveryScope &, const QString &host, const QStringList &, int ttlMs
  );
  void failed(const deskflow::auth::Context &, bool retryable);
};
ScopedAddressResolver *createScopedAddressResolver(QObject *, DiscoveryScopeSource, ServiceNameSource);
class TlsProbeExecutor : public QObject
{
  Q_OBJECT
public:
  using QObject::QObject;
  virtual void start(const deskflow::auth::Context &, const DiscoveryScope &, const QString &numeric, int port) = 0;
  virtual void cancel() = 0;
  virtual bool selectPreparedSession(const QString &root) = 0;
  virtual bool selectCandidateProfile(const QString &, const QString &, const QString &)
  {
    return false;
  }
  virtual bool ready() const
  {
    return false;
  }
  virtual bool permitsPort(int port) const
  {
    return port == 34800;
  }
Q_SIGNALS:
  void completed(const deskflow::auth::Context &, const QString &numeric, const deskflow::auth::Report &);
  void failed(const deskflow::auth::Context &, bool retryable);
};
// Strict bounded parser used by the actual child process, not a log scraper.
class ProbeOutput
{
public:
  explicit ProbeOutput(deskflow::auth::Context context) : m_context(std::move(context))
  {
    m_gate.begin(m_context);
  }
  bool feed(const QByteArray &);
  std::optional<deskflow::auth::Report> finish(int exitCode, bool normalExit);

private:
  deskflow::auth::Context m_context;
  deskflow::auth::Gate m_gate;
  QByteArray m_buffer;
  int m_total = 0, m_records = 0;
  bool m_invalid = false;
  std::optional<deskflow::auth::Report> m_terminal;
};
class ProcessTlsProbe final : public TlsProbeExecutor
{
  Q_OBJECT
public:
  ProcessTlsProbe(QObject *parent, DiscoveryScopeSource source, deskflow::lab::RuntimeProfile profile = {});
  ~ProcessTlsProbe() override;
  void start(const deskflow::auth::Context &, const DiscoveryScope &, const QString &, int) override;
  void cancel() override;
  bool selectPreparedSession(const QString &) override;
  bool selectCandidateProfile(const QString &, const QString &, const QString &) override;
  bool ready() const override;
  bool permitsPort(int port) const override
  {
#ifdef DESKFLOW_CANDIDATE_BUILD
    if (m_profile.mode == deskflow::lab::RuntimeProfile::Mode::Candidate)
      return port >= 1024 && port <= 65535;
#endif
    return m_profile.permitsPort(port);
  }

private:
  DiscoveryScopeSource m_scopes;
  deskflow::lab::RuntimeProfile m_profile;
  std::unique_ptr<deskflow::lab::CredentialLease> m_credentials;
  QProcess *m_process = nullptr;
  QTimer m_deadline, m_scopeCheck;
  deskflow::auth::Context m_context;
  DiscoveryScope m_scope;
  QString m_numeric, m_root;
  std::unique_ptr<ProbeOutput> m_output;
  quint64 m_epoch = 0;
  QString m_profileSettings, m_profileIdentity, m_profileExpected;
  QString parentDirectory() const;
  void dispose(bool notify, bool retryable);
};
class RecoveryProbeBroker final : public QObject
{
  Q_OBJECT
public:
  RecoveryProbeBroker(
      DeviceRecovery &, ScopedAddressResolver &, TlsProbeExecutor &, DiscoveryScopeSource, QObject *parent = nullptr
  );
  void expire(); // timer and deterministic offline timeout entry use same path
private:
  DeviceRecovery &m_recovery;
  ScopedAddressResolver &m_resolver;
  TlsProbeExecutor &m_executor;
  DiscoveryScopeSource m_scopes;
  deskflow::auth::Context m_context, m_childContext;
  DiscoveryScope m_scope;
  QString m_host, m_numeric;
  int m_port = 0;
  QStringList m_remaining;
  QTimer m_timeout;
  QElapsedTimer m_lease;
  int m_leaseMs = 0;
  bool m_probing = false;
  void request(const deskflow::auth::Context &, const QString &, int, const DiscoveryScope &);
  void stop();
  void fail(bool retryable);
  void next();
  bool current() const;
};
} // namespace deskflow::gui
