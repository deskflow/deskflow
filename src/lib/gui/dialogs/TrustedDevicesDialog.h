// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "gui/IdentityBindings.h"
#include <QDialog>
class TrustedDevicesDialog : public QDialog
{
  Q_OBJECT
public:
  TrustedDevicesDialog(
      deskflow::gui::IdentityBindings &bindings, bool connectionAllowed, QWidget *parent = nullptr,
      const QString &unavailableReason = {}
  );
Q_SIGNALS:
  void metadataChanged();
  void connectRequested(const QString &fingerprint, const QString &host, int port);
  void automaticRecoveryRequested(const QString &fingerprint);
  void locateRequested(const QString &fingerprint);
};
