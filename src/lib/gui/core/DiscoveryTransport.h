// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "DiscoveryScope.h"
#include <QObject>
namespace deskflow::gui {
class DiscoveryTransport : public QObject
{
  Q_OBJECT
public:
  explicit DiscoveryTransport(QObject *parent = nullptr, DiscoveryScopeSource source = DiscoveryScope::current)
      : QObject(parent),
        m_scopeSource(std::move(source))
  {
  }
  virtual void browse(const DiscoveryScope &scope) = 0;
  virtual void stop() = 0;
  virtual void advertise(const DiscoveryAdvertisement &advertisement) = 0;

protected:
  bool scopeAvailable(const DiscoveryScope &scope) const
  {
    return scope.valid() && m_scopeSource().contains(scope);
  }

private:
  DiscoveryScopeSource m_scopeSource;
Q_SIGNALS:
  void resolved(const QString &source, const QString &name, const QString &host, int port, int leaseMs);
  void removed(const QString &source);
  void status(const QString &text);
  void activeChanged(bool active);
  void sessionEnded();
};
DiscoveryTransport *createDiscoveryTransport(QObject *parent, DiscoveryScopeSource source = DiscoveryScope::current);
} // namespace deskflow::gui
