// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "gui/core/DiscoveryScope.h"
#include <QDialog>
class QListWidget;
namespace deskflow::gui {
class NearbyDiscovery;
class IdentityBindings;
class DeviceRecovery;
class NearbyDevicesDialog : public QDialog
{
  Q_OBJECT
public:
  explicit NearbyDevicesDialog(
      QWidget *parent = nullptr, DiscoveryScopeSource source = DiscoveryScope::current,
      IdentityBindings *bindings = nullptr, const QString &expected = {}
  );
  DeviceRecovery *recovery() const
  {
    return m_recovery;
  }
  QString selectedHost() const;
  int selectedPort() const;
Q_SIGNALS:
  void searchingChanged(bool active);
  void manualEntryRequested();
  void preparedSessionRequested();
  void verifiedEndpoint(const QString &fingerprint, const QString &host, int port);

private:
  NearbyDiscovery *m_discovery;
  QListWidget *m_devices;
  DeviceRecovery *m_recovery = nullptr;
};
} // namespace deskflow::gui
