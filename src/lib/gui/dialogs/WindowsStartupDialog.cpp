// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "WindowsStartupDialog.h"
#include "gui/ExperienceStyle.h"
#include "gui/widgets/OptionControl.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>

namespace deskflow::gui {
WindowsStartupDialog::WindowsStartupDialog(std::shared_ptr<winstartup::System> system, QString backup, QWidget *parent)
    : QDialog(parent),
      m_system(std::move(system)),
      m_backup(std::move(backup))
{
  setWindowTitle(tr("Startup & recovery"));
  setMinimumSize(520, 440);
  resize(640, 540);
  auto *root = new QVBoxLayout(this);
  root->setContentsMargins(24, 20, 24, 20);
  root->setSpacing(16);
  auto *scroll = new QScrollArea(this);
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  auto *body = new QWidget;
  auto *layout = new QVBoxLayout(body);
  layout->setContentsMargins(0, 0, 12, 12);
  layout->setSpacing(14);
  m_status = new QLabel;
  m_status->setObjectName("windowsStartupStatus");
  m_status->setWordWrap(true);
  m_status->setTextFormat(Qt::PlainText);
  layout->addWidget(m_status);
  auto *description = new QLabel(
      tr("Open Candidate when you sign in. Optional app recovery restarts an abnormal exit up to three times, one "
         "minute apart. A normal quit or saved pause is respected. Changes take effect at the next sign-in; this "
         "connection stays unchanged.")
  );
  description->setWordWrap(true);
  layout->addWidget(description);
  m_login = new OptionCheckBox(tr("Open Candidate at login"));
  m_login->setObjectName("windowsStartupLogin");
  layout->addWidget(m_login);
  m_recovery = new OptionCheckBox(tr("Recover after an abnormal app exit"));
  m_recovery->setObjectName("windowsStartupRecovery");
  layout->addWidget(m_recovery);
  auto *scope = new QLabel(
      tr("This manages one task for your Windows account, without administrator privileges or a stored password. "
         "Recovery monitors task-launched sessions. Existing Deskflow services or startup hooks must be reviewed "
         "before enabling it.")
  );
  scope->setWordWrap(true);
  layout->addWidget(scope);
  m_restore = new QPushButton(tr("Restore previous startup settings…"));
  m_restore->setObjectName("windowsStartupRestore");
  layout->addWidget(m_restore);
  layout->addStretch();
  scroll->setWidget(body);
  root->addWidget(scroll, 1);
  auto *buttons = new QHBoxLayout;
  m_back = new QPushButton(tr("Back"));
  m_refresh = new QPushButton(tr("Refresh status"));
  m_apply = new QPushButton(tr("Review and apply…"));
  m_apply->setObjectName("windowsStartupApply");
  m_apply->setProperty("primaryAction", true);
  buttons->addWidget(m_back);
  buttons->addStretch();
  buttons->addWidget(m_refresh);
  buttons->addWidget(m_apply);
  root->addLayout(buttons);
  applyExperienceStyle(this);
  m_apply->setEnabled(false);
  m_restore->setEnabled(false);
  m_login->setEnabled(false);
  m_recovery->setEnabled(false);
  m_status->setText(tr("Reading startup status…"));
  for (auto *b : {m_restore, m_back, m_refresh, m_apply}) {
    b->setAutoDefault(false);
    b->ensurePolished();
    b->setMinimumSize(b->fontMetrics().horizontalAdvance(b->text()) + 32, b->fontMetrics().height() + 20);
  }
  connect(m_back, &QPushButton::clicked, this, &WindowsStartupDialog::reject);
  connect(m_refresh, &QPushButton::clicked, this, [this] { refresh(); });
  connect(m_apply, &QPushButton::clicked, this, [this] { review(false); });
  connect(m_restore, &QPushButton::clicked, this, [this] { review(true); });
  connect(m_login, &QCheckBox::toggled, this, [this](bool on) {
    m_recovery->setEnabled(on && !m_busy);
    if (!on)
      m_recovery->setChecked(false);
  });
  QTimer::singleShot(0, this, [this] { refresh(); });
}
void WindowsStartupDialog::reject()
{
  if (!m_busy)
    QDialog::reject();
}
void WindowsStartupDialog::render(const winstartup::State &state)
{
  m_state = state;
  m_busy = false;
  m_back->setEnabled(true);
  m_refresh->setEnabled(true);
  const bool ready = state.available && state.error.isEmpty() && !state.conflict;
  m_login->setEnabled(ready);
  m_apply->setEnabled(ready);
  const auto options = winstartup::observedOptions(state);
  m_login->setChecked(options.login);
  m_recovery->setChecked(options.recovery);
  m_recovery->setEnabled(ready && options.login);
  m_restore->setEnabled(ready && winstartup::prepareRestore(state, m_backup).error.isEmpty());
  m_status->setText(
      state.conflict  ? tr("An existing or unrecognized startup mechanism was found. Nothing was changed. Review that "
                            "mechanism before enabling Candidate startup.")
      : !ready        ? tr("Startup status could not be verified: %1. Nothing was changed.").arg(state.error)
      : options.login ? tr("Candidate login startup is enabled. App recovery: %1.")
                            .arg(options.recovery ? tr("enabled") : tr("disabled"))
                      : tr("Candidate login startup is disabled.")
  );
}
void WindowsStartupDialog::refresh()
{
  if (m_busy)
    return;
  m_busy = true;
  for (auto *b : {m_apply, m_restore, m_refresh, m_back})
    b->setEnabled(false);
  m_login->setEnabled(false);
  m_recovery->setEnabled(false);
  m_status->setText(tr("Reading startup status…"));
  const auto system = m_system;
  auto result = std::make_shared<winstartup::State>();
  auto *thread = QThread::create([system, result] { *result = system->read(); });
  connect(thread, &QThread::finished, this, [this, result] { render(*result); });
  connect(thread, &QThread::finished, thread, &QObject::deleteLater);
  thread->start();
}
void WindowsStartupDialog::review(bool restore)
{
  if (m_busy)
    return;
  const auto plan = restore ? winstartup::prepareRestore(m_state, m_backup)
                            : winstartup::prepare(m_state, {m_login->isChecked(), m_recovery->isChecked()});
  if (!plan.error.isEmpty()) {
    m_status->setText(tr("Cannot apply startup settings: %1").arg(plan.error));
    return;
  }
  const auto summary = tr("Change Candidate startup for this Windows account?\n\nLogin: %1\nAbnormal-exit recovery: "
                          "%2\nProgram: %3\n\nNo current connection, firewall rule or other service will be changed.")
                           .arg(
                               plan.options.login ? tr("enabled") : tr("disabled"),
                               plan.options.recovery ? tr("enabled") : tr("disabled"), plan.before.executable
                           );
  if (QMessageBox::question(
          this, tr("Confirm startup settings"), summary, QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel
      ) != QMessageBox::Yes)
    return;
  m_busy = true;
  for (auto *b : {m_apply, m_restore, m_refresh, m_back})
    b->setEnabled(false);
  m_login->setEnabled(false);
  m_recovery->setEnabled(false);
  const auto system = m_system;
  const auto backup = m_backup;
  auto result = std::make_shared<winstartup::Result>();
  auto state = std::make_shared<winstartup::State>();
  auto *thread = QThread::create([system, backup, plan, restore, result, state] {
    *result = restore ? winstartup::apply(plan, plan.digest, *system)
                      : winstartup::applyRecorded(plan, plan.digest, *system, backup);
    *state = system->read();
  });
  connect(thread, &QThread::finished, this, [this, result, state] {
    render(*state);
    if (!result->success)
      m_status->setText(
          tr("Startup settings were not confirmed: %1. Refresh status before trying again.").arg(result->error)
      );
  });
  connect(thread, &QThread::finished, thread, &QObject::deleteLater);
  thread->start();
}
} // namespace deskflow::gui
