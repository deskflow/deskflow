// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "gui/core/AutomaticRecovery.h"
#include <QDialog>
class QCheckBox;
class QComboBox;
class QLabel;
class AutomaticRecoveryDialog : public QDialog
{
  Q_OBJECT
public:
  AutomaticRecoveryDialog(
      const QString &, const deskflow::gui::AutomaticRecoveryPolicy &, deskflow::gui::DiscoveryScopeSource,
      QWidget *parent = nullptr
  );
  deskflow::gui::AutomaticRecoveryPolicy policy() const;
  void showCredentialResult(bool accepted);
  void setReceivingRole(bool receiving);
  void setApplyHandler(std::function<QString(const deskflow::gui::AutomaticRecoveryPolicy &)> handler)
  {
    m_apply = std::move(handler);
  }
  void accept() override;
Q_SIGNALS:
  void preparedSessionRequested();

private:
  std::function<QString(const deskflow::gui::AutomaticRecoveryPolicy &)> m_apply;
  QLabel *m_roleHint = nullptr;
  QString m_fingerprint;
  QCheckBox *m_enabled;
  QComboBox *m_network;
  QLabel *m_status;
  QVector<deskflow::gui::DiscoveryScope> m_scopes;
};
