// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "RecoveryProbe.h"
#include "DeviceRecovery.h"
#include "DiscoveryCatalog.h"
#include "common/CandidatePolicy.h"
#include "common/CredentialStore.h"
#include "common/IsolatedSession.h"
#include "common/LabExecutionPolicy.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcessEnvironment>
#include <QUuid>
namespace deskflow::gui {
#ifdef DESKFLOW_PROBE_PROCESS_TEST
// Defined only by the inert child-process fixture, never by a delivered target.
QString probeProcessFixtureParent();
constexpr bool kPreparedProcessExecution = true;
constexpr bool kCandidateProfileExecution = true;
#else
constexpr bool kPreparedProcessExecution = deskflow::lab::kProbeNetworkExecutionEnabled;
constexpr bool kCandidateProfileExecution = deskflow::lab::kCandidateExecutionEnabled;
#endif
bool validProbeAddress(const DiscoveryScope &scope, const QHostAddress &address)
{
  if (!scope.valid() || !DiscoveryScope::privateAddress(address) || address == scope.address)
    return false;
  const auto n = address.toIPv4Address(), local = scope.address.toIPv4Address(), mask = 0xffffffffu >> scope.prefix;
  return (n & ~mask) == (local & ~mask) && (n & mask) != 0 && (n & mask) != mask;
}
bool ProbeOutput::feed(const QByteArray &bytes)
{
  if (m_invalid || bytes.size() > 8192 - m_total) {
    m_invalid = true;
    return false;
  }
  m_total += bytes.size();
  m_buffer += bytes;
  while (m_buffer.contains('\n')) {
    const auto at = m_buffer.indexOf('\n');
    const auto line = m_buffer.left(at);
    m_buffer.remove(0, at + 1);
    const auto doc = QJsonDocument::fromJson(line);
    const auto o = doc.object();
    if (++m_records > 2 || !doc.isObject() || o.size() != 2 || o["event"] != "peerAuthentication" ||
        !o["payload"].isString())
      return !(m_invalid = true);
    const auto report = deskflow::auth::Report::decode(o["payload"].toString());
    if (!report || report->context != m_context || !m_gate.consume(*report))
      return !(m_invalid = true);
    if (report->outcome != deskflow::auth::Outcome::Started)
      m_terminal = report;
  }
  if (m_buffer.size() > 4096)
    return !(m_invalid = true);
  return true;
}
std::optional<deskflow::auth::Report> ProbeOutput::finish(int code, bool normal)
{
  if (m_invalid || !normal || !m_buffer.isEmpty() || m_records != 2 || !m_terminal)
    return {};
  // Commit only after the worker has exited successfully AND cleaned up.
  if (m_terminal->outcome == deskflow::auth::Outcome::Verified && code != 0)
    return {};
  if (m_terminal->outcome != deskflow::auth::Outcome::Verified && code != 4)
    return {};
  return m_terminal;
}
ProcessTlsProbe::ProcessTlsProbe(QObject *parent, DiscoveryScopeSource source, deskflow::lab::RuntimeProfile profile)
    : TlsProbeExecutor(parent),
      m_scopes(std::move(source)),
      m_profile(profile)
{
  m_deadline.setSingleShot(true);
  m_deadline.setInterval(5000);
  connect(&m_deadline, &QTimer::timeout, this, [this] { dispose(true, true); });
  m_scopeCheck.setInterval(250);
  connect(&m_scopeCheck, &QTimer::timeout, this, [this] {
    if (!m_scopes().contains(m_scope))
      dispose(true, false);
  });
}
ProcessTlsProbe::~ProcessTlsProbe()
{
  cancel();
}
QString ProcessTlsProbe::parentDirectory() const
{
  if (!m_profileSettings.isEmpty())
    return deskflow::lab::CredentialStore::candidateSessionsDirectory(m_profileSettings);
#ifdef DESKFLOW_PROBE_PROCESS_TEST
  return probeProcessFixtureParent();
#elif defined(DESKFLOW_CANDIDATE_BUILD)
  if (!m_profileSettings.isEmpty())
    return deskflow::lab::CredentialStore::candidateSessionsDirectory(m_profileSettings);
  return QDir::cleanPath(QStringLiteral(DESKFLOW_CANDIDATE_SESSIONS_ROOT));
#elif defined(DESKFLOW_LAB_RUNS_ROOT)
  return QDir::cleanPath(QStringLiteral(DESKFLOW_LAB_RUNS_ROOT) + "/../../dual-end-test");
#else
  return {}; // No generic production credential import or network launcher.
#endif
}
bool ProcessTlsProbe::selectCandidateProfile(const QString &settings, const QString &identity, const QString &expected)
{
  cancel();
  m_credentials.reset();
  m_profileSettings.clear();
  m_profileIdentity.clear();
  m_profileExpected.clear();
  if (!kPreparedProcessExecution || !kCandidateProfileExecution ||
      !deskflow::candidate::securityProfile(settings, identity, true, true) ||
      deskflow::auth::fingerprint(expected) != expected || expected.isEmpty())
    return false;
  if (!deskflow::lab::CredentialStore::recoverCandidateSessions(settings))
    return false;
  m_profileSettings = settings;
  m_profileIdentity = identity;
  m_profileExpected = expected;
  return ready();
}
bool ProcessTlsProbe::ready() const
{
  if (!m_profile.valid())
    return false;
  if (m_profileSettings.isEmpty())
    return bool(m_credentials);
  QString error;
  return bool(deskflow::lab::CredentialStore::acquireCandidateProfile(
      m_profileSettings, m_profileIdentity, m_profileExpected, &error
  ));
}
bool ProcessTlsProbe::selectPreparedSession(const QString &root)
{
  if (!kPreparedProcessExecution || m_process || !m_profile.valid() ||
      m_profile.mode == deskflow::lab::RuntimeProfile::Mode::Preview ||
      (m_profile.mode == deskflow::lab::RuntimeProfile::Mode::Candidate && !deskflow::lab::kCandidateExecutionEnabled))
    return false;
  QString error;
  auto lease = deskflow::lab::CredentialStore::acquire(root, parentDirectory(), &error);
  if (!lease)
    return false;
  m_profileSettings.clear();
  m_profileIdentity.clear();
  m_profileExpected.clear();
  m_credentials = std::move(lease);
  return true;
}
void ProcessTlsProbe::cancel()
{
  dispose(false, false);
}
void ProcessTlsProbe::dispose(bool notify, bool retryable)
{
  const auto context = m_context;
  ++m_epoch;
  m_context = {};
  m_deadline.stop();
  m_scopeCheck.stop();
  m_output.reset();
  auto *process = m_process;
  m_process = nullptr;
  const auto root = m_root;
  m_root.clear();
  if (process) {
    disconnect(process, nullptr, this, nullptr);
    if (process->state() != QProcess::NotRunning) {
      // Delete/cleanup only once this exact child is known stopped. No global ps
      // lookup and no other process/service is terminated.
      const auto parent = parentDirectory();
      connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), process, [process, root, parent] {
        if (!root.isEmpty())
          deskflow::lab::CredentialStore::recover(root, parent);
        process->deleteLater();
      });
      process->kill();
      process->setParent(nullptr);
    } else {
      process->deleteLater();
      if (!root.isEmpty())
        deskflow::lab::CredentialStore::recover(root, parentDirectory());
    }
  } else if (!root.isEmpty())
    deskflow::lab::CredentialStore::recover(root, parentDirectory());
  if (!m_profileSettings.isEmpty())
    m_credentials.reset();
  if (notify && context.valid())
    Q_EMIT failed(context, retryable);
}
void ProcessTlsProbe::start(
    const deskflow::auth::Context &context, const DiscoveryScope &scope, const QString &numeric, int port
)
{
  cancel();
  m_context = context;
  m_scope = scope;
  m_numeric = numeric;
  if (!m_profileSettings.isEmpty()) {
    QString error;
    m_credentials = deskflow::lab::CredentialStore::acquireCandidateProfile(
        m_profileSettings, m_profileIdentity, context.expected, &error
    );
    if (context.expected != m_profileExpected || !m_credentials ||
        !deskflow::lab::CredentialStore::ensurePrivateCandidateDirectory(parentDirectory())) {
      dispose(true, false);
      return;
    }
  }
  if (!context.valid() || context.expected.isEmpty() || !permitsPort(port) ||
      !validProbeAddress(scope, QHostAddress(numeric)) || !m_scopes().contains(scope) || !kPreparedProcessExecution ||
      !m_credentials ||
      (m_profile.mode == deskflow::lab::RuntimeProfile::Mode::Candidate && !deskflow::lab::kCandidateExecutionEnabled
      )) {
    dispose(true, false);
    return;
  }
  const auto requestedRoot = parentDirectory() + '/' + context.session;
  QJsonObject manifest{
      {"schema", 1},
      {"purpose", "deskflow-lab-network"},
      {"session", context.session},
      {"mode", "client"},
      {"bindAddress", scope.address.toString()},
      {"peerAddress", numeric},
      {"port", port},
      {"tls", true},
      {"peerChecks", true}
  };
  if (m_profile.mode == deskflow::lab::RuntimeProfile::Mode::Candidate) {
    auto selected = m_profile;
    selected.port = port;
    manifest["schema"] = 2;
    manifest["runtime"] = QString::fromUtf8(selected.encode());
  }
  if (!deskflow::lab::CredentialStore::createSession(requestedRoot, parentDirectory(), manifest).isEmpty()) {
    dispose(true, false);
    return;
  }
  m_root = requestedRoot;
  if (!deskflow::lab::CredentialStore::provision(m_root, parentDirectory(), m_credentials->material()).isEmpty()) {
    dispose(true, false);
    return;
  }
#ifdef DESKFLOW_PROBE_PROCESS_TEST
  const auto executable = QCoreApplication::applicationFilePath();
#elif defined(DESKFLOW_CANDIDATE_BUILD) && defined(Q_OS_WIN)
  const auto executable = QCoreApplication::applicationDirPath() + "/deskflow-candidate-network-core.exe";
#elif defined(DESKFLOW_CANDIDATE_BUILD)
  const auto executable = QCoreApplication::applicationDirPath() + "/deskflow-candidate-network-core";
#elif defined(Q_OS_WIN)
  const auto executable = QCoreApplication::applicationDirPath() + "/deskflow-lab-network-core.exe";
#else
  const auto executable = QCoreApplication::applicationDirPath() + "/deskflow-lab-network-core";
#endif
  const QFileInfo file(executable);
  if (file.isSymLink() || !file.isExecutable() || file.canonicalFilePath() != executable) {
    dispose(true, false);
    return;
  }
  m_output = std::make_unique<ProbeOutput>(context);
  auto *process = new QProcess(this);
  m_process = process;
  const auto epoch = m_epoch;
  // Keep only OS-required path/temp values; never inherit Qt plugin/loader or
  // credential overrides into the helper. Program path is always absolute.
  QProcessEnvironment env;
  const auto system = QProcessEnvironment::systemEnvironment();
  for (const auto &key : {"SystemRoot", "WINDIR", "TEMP", "TMP", "TMPDIR", "LANG"})
    if (system.contains(key))
      env.insert(key, system.value(key));
  process->setProcessEnvironment(env);
  process->setWorkingDirectory(m_root);
  process->setStandardErrorFile(QProcess::nullDevice());
  connect(process, &QProcess::readyReadStandardOutput, this, [this, process, epoch] {
    if (epoch != m_epoch || process != m_process)
      return;
    if (!m_output->feed(process->readAllStandardOutput()))
      dispose(true, false);
  });
  connect(process, &QProcess::errorOccurred, this, [this, process, epoch](QProcess::ProcessError error) {
    if (epoch == m_epoch && process == m_process && error == QProcess::FailedToStart)
      dispose(true, false);
  });
  connect(
      process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
      [this, process, epoch, context, numeric](int code, QProcess::ExitStatus status) {
        if (epoch != m_epoch || process != m_process)
          return;
        const bool parsed = m_output->feed(process->readAllStandardOutput());
        const auto report = parsed ? m_output->finish(code, status == QProcess::NormalExit) : std::nullopt;
        bool scopeValid = m_scopes().contains(m_scope);
        if (!m_profileSettings.isEmpty()) {
          QString error;
          auto current = deskflow::lab::CredentialStore::acquireCandidateProfile(
              m_profileSettings, m_profileIdentity, m_profileExpected, &error
          );
          scopeValid = scopeValid && current && m_credentials &&
                       current->material().identity == m_credentials->material().identity &&
                       current->material().servers == m_credentials->material().servers;
        }
        dispose(false, false);
        if (scopeValid && report)
          Q_EMIT completed(context, numeric, *report);
        else
          Q_EMIT failed(context, scopeValid && code == 4);
      }
  );
  QStringList arguments{
      "--run", m_root, "--peer-auth-context", context.encode(), "--interface-index", QString::number(scope.index)
  };
  if (!m_profileSettings.isEmpty())
    arguments << "--candidate-profile" << m_profileSettings;
  m_deadline.start();
  m_scopeCheck.start();
  process->start(executable, arguments);
}
RecoveryProbeBroker::RecoveryProbeBroker(
    DeviceRecovery &recovery, ScopedAddressResolver &resolver, TlsProbeExecutor &executor, DiscoveryScopeSource source,
    QObject *parent
)
    : QObject(parent),
      m_recovery(recovery),
      m_resolver(resolver),
      m_executor(executor),
      m_scopes(std::move(source))
{
  m_timeout.setSingleShot(true);
  connect(&m_timeout, &QTimer::timeout, this, &RecoveryProbeBroker::expire);
  connect(&recovery, &DeviceRecovery::verificationRequested, this, &RecoveryProbeBroker::request);
  connect(&recovery, &DeviceRecovery::verificationCancelled, this, [this](const auto &c) {
    if (c == m_context)
      stop();
  });
  connect(&resolver, &ScopedAddressResolver::failed, this, [this](const auto &c, bool retryable) {
    if (c == m_context)
      fail(retryable);
  });
  connect(
      &resolver, &ScopedAddressResolver::addresses, this,
      [this](const auto &c, const auto &scope, const QString &host, const QStringList &addresses, int ttl) {
        if (c != m_context || !current() || scope != m_scope || host != m_host || m_probing)
          return;
        if (ttl <= 0 || addresses.isEmpty() || addresses.size() > 8) {
          fail(true);
          return;
        }
        m_remaining.clear();
        for (const auto &a : addresses) {
          const QHostAddress value(a);
          if (a != value.toString() || !validProbeAddress(scope, value)) {
            fail(false);
            return;
          }
          if (!m_remaining.contains(a))
            m_remaining << a;
        }
        m_probing = true;
        m_lease.start();
        m_leaseMs = qMin(ttl, 10000);
        next();
      }
  );
  connect(&executor, &TlsProbeExecutor::failed, this, [this](const auto &c, bool retryable) {
    if (c == m_childContext && current()) {
      if (retryable)
        next();
      else
        fail(false);
    }
  });
  connect(
      &executor, &TlsProbeExecutor::completed, this,
      [this](const auto &c, const QString &numeric, const deskflow::auth::Report &report) {
        if (c != m_childContext || !current() || numeric != m_numeric || report.context != c || !report.valid())
          return;
        if (m_lease.elapsed() >= m_leaseMs) {
          fail(true);
          return;
        }
        if (report.outcome == deskflow::auth::Outcome::Verified) {
          if (!m_recovery.bindNumericEndpoint(m_context, numeric)) {
            fail(false);
            return;
          }
          const auto saved = m_context;
          auto committed = report;
          committed.context = saved;
          stop();
          m_recovery.receive({saved, report.sequence, deskflow::auth::Outcome::Started, {}});
          m_recovery.receive(committed);
        } else if (report.outcome == deskflow::auth::Outcome::WrongIdentity ||
                   report.outcome == deskflow::auth::Outcome::MissingCertificate)
          next();
        else
          fail(false);
      }
  );
}
bool RecoveryProbeBroker::current() const
{
  return m_context.valid() && m_recovery.attempt() == m_context && m_scopes().contains(m_scope);
}
void RecoveryProbeBroker::stop()
{
  m_context = {};
  m_childContext = {};
  m_probing = false;
  m_remaining.clear();
  m_timeout.stop();
  m_resolver.cancel();
  m_executor.cancel();
}
void RecoveryProbeBroker::fail(bool retryable)
{
  const auto c = m_context;
  stop();
  if (c.valid())
    m_recovery.failed(c, retryable);
}
void RecoveryProbeBroker::expire()
{
  fail(true);
}
void RecoveryProbeBroker::request(
    const deskflow::auth::Context &c, const QString &host, int port, const DiscoveryScope &scope
)
{
  stop();
  m_context = c;
  m_host = host;
  m_port = port;
  m_scope = scope;
  if (!current() || c.expected.isEmpty() || !m_executor.permitsPort(port) ||
      !DiscoveryCatalog::validEndpoint(host, port)) {
    fail(false);
    return;
  }
  m_timeout.start(9000);
  m_resolver.start(c, scope, host, port);
}
void RecoveryProbeBroker::next()
{
  if (!current() || (m_probing && m_lease.elapsed() >= m_leaseMs)) {
    fail(false);
    return;
  }
  if (m_remaining.isEmpty()) {
    fail(true);
    return;
  }
  m_numeric = m_remaining.takeFirst();
  m_childContext = {QUuid::createUuid().toString(QUuid::Id128), m_context.expected, m_context.generation};
  m_executor.start(m_childContext, m_scope, m_numeric, m_port);
}
} // namespace deskflow::gui
