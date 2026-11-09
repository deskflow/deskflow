// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "DesktopWorkspace.h"
#include "PeerAuthentication.h"
#include <QCryptographicHash>

namespace deskflow::display {
inline QString workspacePlanId(const WorkspaceState &state)
{
  return QString::fromLatin1(QCryptographicHash::hash(state.encode(), QCryptographicHash::Sha256).toHex());
}
inline std::optional<Layout> workspaceLayoutOnRestart(
    const QByteArray &confirmedBytes, const QByteArray &draftBytes, const QByteArray &legacyLayout, bool external
)
{
  if (external)
    return {};
  const auto confirmed = WorkspaceState::decode(confirmedBytes);
  if (confirmed)
    return confirmed->layout;
  const auto draft = WorkspaceState::decode(draftBytes);
  if (draft && draft->revision)
    return {};
  return Layout::decode(legacyLayout);
}
struct WorkspaceApplyRequest
{
  QString id, session;
  quint64 generation = 0;
  WorkspaceState state;
  QByteArray encode() const
  {
    return QJsonDocument(
               QJsonObject{
                   {"id", id},
                   {"session", session},
                   {"generation", QString::number(generation)},
                   {"state", QJsonDocument::fromJson(state.encode()).object()}
               }
    ).toJson(QJsonDocument::Compact);
  }
  static std::optional<WorkspaceApplyRequest> decode(const QByteArray &bytes)
  {
    if (bytes.size() > 14000)
      return {};
    const auto o = QJsonDocument::fromJson(bytes).object();
    WorkspaceApplyRequest r;
    r.id = o["id"].toString();
    r.session = o["session"].toString();
    r.generation = auth::counter(o["generation"]);
    const auto state = WorkspaceState::decode(QJsonDocument(o["state"].toObject()).toJson(QJsonDocument::Compact));
    if (o.size() != 4 || !auth::identifier(r.id) || !auth::identifier(r.session) || !r.generation || !state ||
        !state->revision)
      return {};
    r.state = *state;
    return r;
  }
};
struct WorkspaceApplyResult
{
  QString id, session, planId, error;
  quint64 generation = 0, revision = 0;
  bool accepted = false;
  QByteArray encode() const
  {
    return QJsonDocument(
               QJsonObject{
                   {"id", id},
                   {"session", session},
                   {"generation", QString::number(generation)},
                   {"revision", QString::number(revision)},
                   {"planId", planId},
                   {"accepted", accepted},
                   {"error", error}
               }
    ).toJson(QJsonDocument::Compact);
  }
  static std::optional<WorkspaceApplyResult> decode(const QByteArray &bytes)
  {
    if (bytes.size() > 2048)
      return {};
    const auto o = QJsonDocument::fromJson(bytes).object();
    WorkspaceApplyResult r;
    r.id = o["id"].toString();
    r.session = o["session"].toString();
    r.planId = o["planId"].toString();
    r.error = o["error"].toString();
    r.generation = auth::counter(o["generation"]);
    r.revision = auth::counter(o["revision"]);
    r.accepted = o["accepted"].toBool();
    if (o.size() != 7 || !auth::identifier(r.id) || !auth::identifier(r.session) || !r.generation || !r.revision ||
        r.planId.isEmpty() || auth::fingerprint(r.planId) != r.planId || !o["accepted"].isBool() ||
        !o["error"].isString() || r.error.size() > 160 || (r.accepted && !r.error.isEmpty()) ||
        (!r.accepted && r.error.isEmpty()))
      return {};
    return r;
  }
  bool matches(const WorkspaceApplyRequest &r) const
  {
    return id == r.id && session == r.session && generation == r.generation && revision == r.state.revision &&
           planId == workspacePlanId(r.state);
  }
};
// The same validator is called on the core event thread before any live route
// is replaced. A request cannot alter configuration: it must exactly match the
// already atomically saved workspace read from this core's own settings file.
inline QString validateWorkspaceApply(
    const WorkspaceApplyRequest &r, const WorkspaceSnapshot &live, const QByteArray &saved,
    bool explicitlySubmitted = true
)
{
  if (!explicitlySubmitted)
    return "draft-not-submitted";
  if (r.session != live.session || r.generation != live.generation)
    return "stale-connection-generation";
  if (live.externalConfig)
    return "external-configuration";
  const auto persisted = WorkspaceState::decode(saved);
  if (!persisted || persisted->encode() != r.state.encode())
    return "saved-plan-changed";
  if (r.state.revision < live.planRevision)
    return "stale-configuration-version";
  if (r.state.revision == live.planRevision && live.planId != workspacePlanId(r.state))
    return "configuration-version-conflict";
  for (const auto &p : r.state.layout.placements) {
    if (!live.onlineDevices.contains(p.device) || !live.topologyCapable.contains(p.device) ||
        !live.topologies.contains(p.device))
      return "unsupported-or-offline-endpoint";
    const auto identity = live.identityReferences.value(p.device);
    if (identity.isEmpty() || r.state.identityReferences.value(p.device) != identity)
      return "verified-device-identity-changed";
  }
  return r.state.layout.error(live.topologies);
}
} // namespace deskflow::display
