// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "RuntimeProfile.h"
#include "LabExecutionPolicy.h"
#include "PeerAuthentication.h"
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>
namespace deskflow::lab {
QString RuntimeProfile::name() const
{
  switch (mode) {
  case Mode::Preview:
    return "preview";
  case Mode::NetworkTest:
    return "network-test";
  case Mode::Candidate:
    return "candidate";
  }
  return {};
}
bool RuntimeProfile::valid() const
{
  return !name().isEmpty() && tls && peerChecks &&
         (mode == Mode::Candidate ? (input && port >= 1024 && port <= 65535) : (!input && port == 34800));
}
bool RuntimeProfile::permitsPort(int value) const
{
  return valid() && mode != Mode::Preview && port == value;
}
bool RuntimeProfile::executable() const
{
  return valid() && mode != Mode::Preview && kProbeNetworkExecutionEnabled &&
         (mode != Mode::Candidate || kCandidateExecutionEnabled);
}
QByteArray RuntimeProfile::encode() const
{
  if (!valid())
    return {};
  return QJsonDocument(
             QJsonObject{
                 {"schema", 1},
                 {"mode", name()},
                 {"port", port},
                 {"tls", tls},
                 {"peerChecks", peerChecks},
                 {"input", input}
             }
  ).toJson(QJsonDocument::Compact);
}
RuntimeProfile RuntimeProfile::decode(const QByteArray &bytes, bool *ok)
{
  if (ok)
    *ok = false;
  RuntimeProfile invalid;
  invalid.port = 0;
  if (bytes.size() > 1024)
    return invalid;
  const auto doc = QJsonDocument::fromJson(bytes);
  if (!doc.isObject())
    return invalid;
  const auto o = doc.object();
  RuntimeProfile p;
  const auto mode = o["mode"].toString();
  if (mode == "preview")
    p.mode = Mode::Preview;
  else if (mode == "network-test")
    p.mode = Mode::NetworkTest;
  else if (mode == "candidate")
    p.mode = Mode::Candidate;
  else
    return invalid;
  p.port = o["port"].toInt();
  p.tls = o["tls"] == true;
  p.peerChecks = o["peerChecks"] == true;
  p.input = o["input"].toBool();
  if (o.size() != 6 || o["schema"] != 1 || !o["input"].isBool() || !p.valid() || p.encode() != bytes)
    return invalid;
  if (ok)
    *ok = true;
  return p;
}
QJsonObject RuntimeProfile::preflight() const
{
  QJsonArray blockers;
  if (!valid())
    blockers.append("invalid-runtime-profile");
  else if (mode == Mode::Preview)
    blockers.append("preview-has-no-network-or-input");
  else {
    if (!kProbeNetworkExecutionEnabled)
      blockers.append("real-network-and-identity-authorization-pending");
    if (mode == Mode::Candidate && !kCandidateExecutionEnabled)
      blockers.append("input-and-candidate-installation-authorization-pending");
  }
  return {{"valid", valid()},           {"mode", name()},           {"port", port},
          {"canExecute", executable()}, {"tlsRequired", true},      {"peerChecksRequired", true},
          {"credentialsCopied", false}, {"writesPerformed", false}, {"productionNamespaceAllowed", false},
          {"inputRequested", input},    {"inputEnabled", false},    {"blockers", blockers}};
}
ProfileMigration ProfileMigration::prepare(const QByteArray &bytes, const RuntimeProfile &p)
{
  ProfileMigration result;
  auto fail = [&](const char *code) {
    result.error = code;
    return result;
  };
  if (!p.valid() || p.mode != RuntimeProfile::Mode::Candidate || bytes.size() > 8192)
    return fail("invalid-migration-request");
  const auto d = QJsonDocument::fromJson(bytes);
  if (!d.isObject())
    return fail("invalid-public-snapshot");
  auto o = d.object();
  const QSet<QString> expected{"schema",     "computerName", "remoteHost",  "port",      "tls",
                               "peerChecks", "languageSync", "identityPin", "autoStart", "processMode"};
  const auto keys = o.keys();
  if (QSet<QString>(keys.begin(), keys.end()) != expected || o["schema"] != 1)
    return fail("nonpublic-or-unknown-setting");
  if (o["tls"] != true || o["peerChecks"] != true || o["languageSync"] != false || !o["autoStart"].isBool() ||
      o["processMode"] != "desktop")
    return fail("unsafe-or-unsupported-settings");
  if (o["port"].toInt() < 1024 || o["port"].toInt() > 65535 || o["port"] != o["port"].toInt())
    return fail("invalid-source-port");
  const auto fp = o["identityPin"].toString();
  if (fp.isEmpty() || deskflow::auth::fingerprint(fp) != fp)
    return fail("verified-identity-required");
  const auto name = o["computerName"].toString(), host = o["remoteHost"].toString();
  const QRegularExpression controls("[\\x00-\\x1f\\x7f]");
  if (name.isEmpty() || name.size() > 128 || host.isEmpty() || host.size() > 253 || controls.match(name).hasMatch() ||
      controls.match(host).hasMatch() || name.contains('/') || name.contains('\\') || host.contains('/') ||
      host.contains('\\'))
    return fail("invalid-public-label");
  // Preserve the approved pin, input-method choice and role; never enable
  // persistence/startup during migration. Secret material is supplied separately.
  result.before = bytes;
  o["port"] = p.port;
  o["autoStart"] = false;
  result.after = QJsonDocument(o).toJson(QJsonDocument::Compact);
  return result;
}
QByteArray ProfileMigration::rollback(const QByteArray &current, QString *outError) const
{
  if (outError)
    outError->clear();
  if (!valid() || current != after) {
    if (outError)
      *outError = "snapshot-changed-rollback-refused";
    return {};
  }
  return before;
}
QJsonObject ProfileMigration::plan() const
{
  auto hash = [](const QByteArray &v) {
    return QString::fromLatin1(QCryptographicHash::hash(v, QCryptographicHash::Sha256).toHex());
  };
  return {
      {"valid", valid()},
      {"error", error},
      {"sourceDigest", valid() ? hash(before) : QString()},
      {"candidateDigest", valid() ? hash(after) : QString()},
      {"rollbackRequiresExactCandidate", true},
      {"credentialAction", "none-separate-reviewed-provisioning-required"},
      {"startupEnabled", false},
      {"languageSyncPreserved", valid()},
      {"writesPerformed", false}
  };
}
} // namespace deskflow::lab
