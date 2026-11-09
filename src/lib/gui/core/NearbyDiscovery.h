// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "DiscoveryScope.h"
#include <QObject>
#include <memory>
namespace deskflow::gui {
class DiscoveryTransport;
// Discovery supplies untrusted address hints. It never reads or writes trust.
class NearbyDiscovery : public QObject
{
  Q_OBJECT
public:
  explicit NearbyDiscovery(QObject *parent = nullptr, DiscoveryTransport *transport = nullptr);
  ~NearbyDiscovery() override;
  void start(const DiscoveryScope &scope);
  void stop();
  QStringList serviceNames(const QString &host, int port) const;
  static bool validEndpoint(const QString &host, int port);
Q_SIGNALS:
  void found(const QString &key, const QString &name, const QString &host, int port);
  void removed(const QString &key);
  void status(const QString &text);
  void searchingChanged(bool active);

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};
} // namespace deskflow::gui
