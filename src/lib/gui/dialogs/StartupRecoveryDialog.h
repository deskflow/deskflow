// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "gui/core/MacStartupExecutor.h"
#include "gui/core/StartupRecovery.h"
#include <QDialog>
#include <functional>
class QLabel;
class QCheckBox;
class QPushButton;
class QPlainTextEdit;
namespace deskflow::gui {
class StartupRecoveryDialog final : public QDialog
{
  Q_OBJECT
public:
  using ChangeHandler = std::function<bool(const startup::Plan &, const QByteArray &)>;
  using RestoreHandler = std::function<bool(const startup::RecoveryRecord &, const QByteArray &)>;
  StartupRecoveryDialog(
      std::shared_ptr<startup::Reader>, startup::Preferences, QWidget *parent = nullptr, ChangeHandler = {},
      RestoreHandler = {}
  );
Q_SIGNALS:
  void requestStartupQuit();

private:
  void refresh();
  void showSnapshot(const startup::Snapshot &);
  void preview();
  void confirmChange(bool restoring = false);
  bool beginHandoff(const startup::Plan *, const startup::RecoveryRecord *, const QByteArray &);
  std::shared_ptr<startup::Reader> m_reader;
  startup::Preferences m_preferences;
  startup::Snapshot m_snapshot;
  startup::Plan m_plan;
  ChangeHandler m_change;
  RestoreHandler m_restoreChange;
  QLabel *m_status = nullptr, *m_services = nullptr, *m_policy = nullptr, *m_preview = nullptr, *m_history = nullptr;
  QCheckBox *m_login = nullptr, *m_crash = nullptr;
  QPushButton *m_refresh = nullptr, *m_review = nullptr, *m_apply = nullptr, *m_restore = nullptr;
  QPlainTextEdit *m_details = nullptr;
  bool m_reading = false;
};
} // namespace deskflow::gui
