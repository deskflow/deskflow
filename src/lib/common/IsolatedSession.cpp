// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "IsolatedSession.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>
#include <QThread>
#include <atomic>
#include <optional>

namespace deskflow::lab {
namespace {
std::optional<IsolatedSession> activeSession;
std::atomic_bool startupSealed{false};
bool privateV4(const QString &s)
{
  const auto parts = s.split('.');
  if (parts.size() != 4)
    return false;
  int n[4];
  for (int i = 0; i < 4; ++i) {
    bool ok = false;
    n[i] = parts[i].toInt(&ok);
    if (!ok || n[i] < 0 || n[i] > 255 || QString::number(n[i]) != parts[i])
      return false;
  }
  return n[0] == 127 || n[0] == 10 || (n[0] == 192 && n[1] == 168) || (n[0] == 172 && n[1] >= 16 && n[1] <= 31) ||
         (n[0] == 169 && n[1] == 254);
}
bool safeExisting(const QString &p, bool directory)
{
  const QFileInfo f(p);
  return !f.isSymLink() && f.exists() && (directory ? f.isDir() : f.isFile()) && f.canonicalFilePath() == p;
}
} // namespace
bool IsolatedSession::activate(const QString &root, const QString &parent)
{
  // Bootstrap only on the application thread, before any Settings/IPC objects.
  if (!QCoreApplication::instance() || QThread::currentThread() != QCoreApplication::instance()->thread())
    return false;
  if (startupSealed || activeSession)
    return false;
  const auto s = inspect(root, parent);
  if (!s.valid())
    return false;
  activeSession = s;
  return true;
}
const IsolatedSession *IsolatedSession::active()
{
  return activeSession ? &*activeSession : nullptr;
}
void IsolatedSession::sealStartup()
{
  startupSealed = true;
}
IsolatedSession IsolatedSession::inspectLayout(const QString &requested, const QString &allowedParent)
{
  IsolatedSession s;
  const auto fail = [&s](const char *code) {
    s.error = QString::fromLatin1(code);
    return s;
  };
  const QString parent = QDir::cleanPath(allowedParent);
  const QFileInfo rootInfo(requested);
  if (!QDir::isAbsolutePath(parent) || !safeExisting(parent, true) || !rootInfo.isAbsolute() ||
      requested != QDir::cleanPath(requested) || rootInfo.absolutePath() != parent ||
      !QRegularExpression("\\A[0-9a-f]{32}\\z").match(rootInfo.fileName()).hasMatch() || !safeExisting(requested, true))
    return fail("invalid-session-root");
  s.root = requested;
  // No path, IPC, process, service or environment override can enter this schema.
  const auto manifest = requested + "/session.json";
  if (!safeExisting(manifest, false) || QFileInfo(manifest).size() > 4096)
    return fail("invalid-session-manifest");
  QFile file(manifest);
  if (!file.open(QIODevice::ReadOnly))
    return fail("unreadable-session-manifest");
  QJsonParseError parse;
  const auto doc = QJsonDocument::fromJson(file.readAll(), &parse);
  if (parse.error != QJsonParseError::NoError || !doc.isObject())
    return fail("invalid-session-json");
  const auto o = doc.object();
  QSet<QString> keys{"schema", "purpose", "session", "mode", "bindAddress", "peerAddress", "port", "tls", "peerChecks"};
  const bool candidate = o["schema"] == 2;
  if (!candidate) {
    s.runtime.mode = RuntimeProfile::Mode::NetworkTest;
    s.runtime.input = false;
    s.runtime.port = 34800;
  }
  if (candidate)
    keys.insert("runtime");
  const auto actual = o.keys();
  if (QSet<QString>(actual.begin(), actual.end()) != keys || (!candidate && o["schema"] != 1) ||
      o["purpose"] != "deskflow-lab-network" || o["session"] != rootInfo.fileName())
    return fail("invalid-session-schema");
  if (o["tls"] != true || o["peerChecks"] != true)
    return fail("security-downgrade-rejected");
  if (candidate) {
    bool ok = false;
    s.runtime = RuntimeProfile::decode(o["runtime"].toString().toUtf8(), &ok);
    if (!ok || s.runtime.mode != RuntimeProfile::Mode::Candidate || !s.runtime.permitsPort(o["port"].toInt()) ||
        o["port"] != o["port"].toInt())
      return fail("invalid-candidate-profile");
    s.port = s.runtime.port;
  } else if (o["port"] != 34800)
    return fail("test-port-required");
  s.mode = o["mode"].toString();
  s.bindAddress = o["bindAddress"].toString();
  s.peerAddress = o["peerAddress"].toString();
  if (!QStringList{"server", "client"}.contains(s.mode) || !privateV4(s.bindAddress) || !privateV4(s.peerAddress))
    return fail("explicit-private-addresses-required");
  for (const auto *dir : {"config", "config/DeskflowLab", "state", "runtime", "logs", "config/DeskflowLab/tls"})
    if (!safeExisting(requested + '/' + dir, true))
      return fail("missing-or-redirected-session-directory");
  s.settings = requested + "/config/DeskflowLab/DeskflowLab.conf";
  s.state = requested + "/state/DeskflowLab.state";
  s.identity = requested + "/config/DeskflowLab/tls/test-identity.pem";
  s.trustedServers = requested + "/config/DeskflowLab/tls/trusted-servers";
  s.trustedClients = requested + "/config/DeskflowLab/tls/trusted-clients";
  s.lockFile = requested + "/runtime/core.lock";
  // This name is session-specific on both platforms. No server is created here.
  s.ipcName = "deskflow-lab-ipc-" + rootInfo.fileName();
  return s;
}
IsolatedSession IsolatedSession::inspect(const QString &requested, const QString &parent)
{
  auto s = inspectLayout(requested, parent);
  if (!s.valid())
    return s;
  for (const auto &path : {s.settings, s.state, s.identity, s.trustedServers, s.trustedClients, s.lockFile}) {
    const QFileInfo f(path);
    if (f.exists() || f.isSymLink()) {
      s.error = "fresh-session-files-required";
      return s;
    }
  }
  return s;
}
QJsonObject IsolatedSession::plan() const
{
  if (!valid())
    return {{"valid", false}, {"error", error}, {"networkExecuted", false}};
  return {
      {"valid", true},
      {"schema", 1},
      {"root", root},
      {"settings", settings},
      {"state", state},
      {"identity", identity},
      {"trustedServers", trustedServers},
      {"trustedClients", trustedClients},
      {"ipcName", ipcName},
      {"lockFile", lockFile},
      {"mode", mode},
      {"port", port},
      {"runtime", runtime.preflight()},
      {"bindAddress", bindAddress},
      {"peerAddress", peerAddress},
      {"tls", true},
      {"peerChecks", true},
      {"networkExecuted", false},
      {"ipcCreated", false},
      {"credentialsCreated", false},
      {"inputEnabled", false},
      {"executionAvailable", false}
  };
}
} // namespace deskflow::lab
