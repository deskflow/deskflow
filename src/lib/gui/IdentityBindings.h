// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "common/PeerAuthentication.h"
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QString>
#include <optional>

namespace deskflow::gui {
// Device metadata only. Never a replacement for the core's TLS trust database.
// A discovery event cannot call confirm() or authenticated(). The network
// integration must supply a fresh verified TLS result and matching attempt ID.
class IdentityBindings
{
public:
  struct Endpoint
  {
    QString host;
    int port = 0;
    bool operator==(const Endpoint &) const = default;
  };
  struct Device
  {
    QString sha256, label;
    Endpoint endpoint;
    bool operator==(const Device &) const = default;
  };
  enum class Review
  {
    Invalid,
    VerifyNew,
    KnownIdentity,
    ChangedIdentity
  };
  struct Attempt
  {
    quint64 generation = 0;
    Endpoint endpoint;
    QString expected, observed;
    Review review = Review::Invalid;
  };
  static QString normalizedFingerprint(const QString &value);
  static bool validEndpoint(const Endpoint &endpoint);
  static bool validLabel(const QString &label);
  bool load(const QByteArray &json); // atomic, bounded; invalid data does not replace records
  QByteArray encode() const;
  const QMap<QString, Device> &devices() const
  {
    return m_devices;
  }
  quint64 begin(const Endpoint &, const QString &expectedFingerprint = {});
  Review observe(quint64 generation, const QString &fingerprint);
  bool confirm(quint64 generation, const QString &label, bool comparedOnBothDevices);
  bool authenticated(quint64 generation, const QString &verifiedFingerprint, bool tls, bool peerChecks);
  void cancel();
  bool forget(const QString &fingerprint);
  bool rename(const QString &fingerprint, const QString &label);
  // Saved metadata selects an expected pin; it never grants TLS trust. If two
  // saved identities share an address, require an explicit saved-device choice.
  QList<Device> atEndpoint(const Endpoint &) const;
  std::optional<Device> selectedDevice(const Endpoint &, const QString &selectedFingerprint = {}) const;
  // Only call from the current core's verified peer-name signal. Replace the
  // address-based default once; preserve existing nicknames and schema 1.
  bool updateDefaultLabel(const QString &fingerprint, const QString &reportedName);
  bool beginAuthentication(const deskflow::auth::Context &context, const Endpoint &endpoint);
  bool
  beginReviewedAuthentication(const deskflow::auth::Context &context, const Endpoint &endpoint, const QString &label);
  bool consumeAuthentication(const deskflow::auth::Report &report);
  const Attempt &attempt() const
  {
    return m_attempt;
  }

private:
  QMap<QString, Device> m_devices;
  Attempt m_attempt;
  quint64 m_generation = 0;
  QString m_label;
  bool m_confirmed = false;
  deskflow::auth::Gate m_authGate;
};
} // namespace deskflow::gui
