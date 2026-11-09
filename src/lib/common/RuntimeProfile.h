// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <QByteArray>
#include <QJsonObject>
namespace deskflow::lab {
// A launch contract, not permission to run. Candidate execution remains a
// separate compiled gate; no argument or setting can grant it.
struct RuntimeProfile
{
  enum class Mode
  {
    Preview,
    NetworkTest,
    Candidate
  };
#ifdef DESKFLOW_CANDIDATE_BUILD
  Mode mode = Mode::Candidate;
  int port = 34800;
  bool tls = true, peerChecks = true, input = true;
#else
  Mode mode = Mode::NetworkTest;
  int port = 34800;
  bool tls = true, peerChecks = true, input = false;
#endif
  bool valid() const;
  bool permitsPort(int) const;
  bool executable() const;
  QByteArray encode() const;
  static RuntimeProfile decode(const QByteArray &, bool *ok = nullptr);
  QString name() const;
  QJsonObject preflight() const;
};
// Migration operates on an explicitly supplied PUBLIC settings snapshot only.
// It cannot read or write files, copy credentials, alter trust or install apps.
// The result is used by the real helper's read-only preflight endpoint.
struct ProfileMigration
{
  QByteArray before, after;
  QString error;
  bool valid() const
  {
    return error.isEmpty() && !after.isEmpty();
  }
  static ProfileMigration prepare(const QByteArray &publicSnapshot, const RuntimeProfile &);
  QByteArray rollback(const QByteArray &current, QString *error = nullptr) const;
  QJsonObject plan() const;
};
} // namespace deskflow::lab
