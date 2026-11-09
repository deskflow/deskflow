// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "gui/core/WindowsStartupProvider.h"
#include <QDialog>
class QLabel;
class QCheckBox;
class QPushButton;
namespace deskflow::gui {
class WindowsStartupDialog final : public QDialog
{
  Q_OBJECT
public:
  WindowsStartupDialog(std::shared_ptr<winstartup::System>, QString backupFile, QWidget *parent = nullptr);
  void reject() override;

private:
  void refresh();
  void render(const winstartup::State &);
  void review(bool restore);
  std::shared_ptr<winstartup::System> m_system;
  QString m_backup;
  winstartup::State m_state;
  QLabel *m_status;
  QCheckBox *m_login, *m_recovery;
  QPushButton *m_apply, *m_restore, *m_refresh, *m_back;
  bool m_busy = false;
};
} // namespace deskflow::gui
