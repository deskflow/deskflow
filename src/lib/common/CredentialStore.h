// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <memory>
class QFile;

namespace deskflow::lab {
// Opaque bytes supplied by an explicitly authorized provisioner. This API
// cannot generate keys, import arbitrary paths or grant a peer trust.
struct CredentialMaterial
{
  QByteArray identity, servers, clients;
  bool bounded() const;
};
class CredentialLease
{
public:
  ~CredentialLease();
  const CredentialMaterial &material() const;

private:
  friend class CredentialStore;
  struct Impl;
  explicit CredentialLease(std::unique_ptr<Impl>);
  std::unique_ptr<Impl> d;
};
class CredentialStore
{
public:
  // Candidate-owned files only; no production import and no new key generation.
  static bool writeCandidateTrust(const QString &path, const QByteArray &bytes);
  static bool recoverCandidateSessions(const QString &settings);
  static QString candidateSessionsDirectory(const QString &settings);
  static bool ensurePrivateCandidateDirectory(const QString &path);
  static std::unique_ptr<CredentialLease>
  acquireCandidateProfile(const QString &settings, const QString &identity, const QString &expected, QString *error);
  // Exclusive new candidate file; no existing credential is read or replaced.
  static bool openPrivateCandidateFile(QFile &file);
  // Publication is atomic and exclusive. Returns only fixed error codes.
  static QString createSession(const QString &root, const QString &parent, const QJsonObject &manifest);
  static QString provision(const QString &root, const QString &parent, const CredentialMaterial &);
  static std::unique_ptr<CredentialLease> acquire(const QString &root, const QString &parent, QString *error);
  // A crashed process releases its OS lock. Remove only this session's two
  // fixed credential leaf names, never follow links or recursively traverse.
  static QString recover(const QString &root, const QString &parent);
  static bool activate(const CredentialLease &); // bootstrap once; memory snapshot
  static const CredentialMaterial *active();
};
} // namespace deskflow::lab
