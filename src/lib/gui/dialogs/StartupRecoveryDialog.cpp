// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
#include "gui/widgets/OptionControl.h"
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "StartupRecoveryDialog.h"
#include "gui/ExperienceStyle.h"
#include "gui/core/MacStartupExecutor.h"
#include "gui/core/StartupHandoff.h"
#include <QCheckBox>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScopeGuard>
#include <QScrollArea>
#include <QThread>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace deskflow::gui {
using namespace startup;
StartupRecoveryDialog::StartupRecoveryDialog(
    std::shared_ptr<Reader> reader, Preferences preferences, QWidget *parent, ChangeHandler change,
    RestoreHandler restore
)
    : QDialog(parent),
      m_reader(std::move(reader)),
      m_preferences(preferences),
      m_change(std::move(change)),
      m_restoreChange(std::move(restore))
{
  const auto instanceKey = qApp->property("candidateInstanceKey").toString();
  if (nativeStartupChangesAvailable() && !instanceKey.isEmpty()) {
    if (!m_change)
      m_change = [this](const Plan &p, const QByteArray &d) { return beginHandoff(&p, nullptr, d); };
    if (!m_restoreChange)
      m_restoreChange = [this](const RecoveryRecord &r, const QByteArray &d) { return beginHandoff(nullptr, &r, d); };
  }
  applyExperienceStyle(this);
  setWindowTitle(tr("Startup & recovery"));
  setMinimumSize(520, 440);
  resize(640, 600);
  auto *root = new QVBoxLayout(this);
  root->setContentsMargins(24, 20, 24, 20);
  root->setSpacing(14);
  auto *scroll = new QScrollArea(this);
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  auto *body = new QWidget;
  auto *layout = new QVBoxLayout(body);
  layout->setContentsMargins(0, 0, 14, 12);
  layout->setSpacing(16);
  auto label = [&](const char *name) {
    auto *w = new QLabel(body);
    w->setObjectName(name);
    w->setWordWrap(true);
    w->setTextFormat(Qt::PlainText);
    layout->addWidget(w);
    return w;
  };
  m_status = label("startupStatus");
  m_services = label("startupServices");
  m_policy = label("startupPolicy");
  auto *retry = label("startupConnectionRecovery");
  retry->setText(
      tr("Connection recovery while the app is open") + "\n" +
      tr("The receiving computer retries the current address after a connection drop. If the input process crashes, "
         "the app attempts to restart it up to three times. Pause, normal quit and identity errors stop these attempts."
      )
  );
  auto *address = label("startupAddressRecovery");
  address->setText(
      tr("When a saved computer’s address changes") + "\n" +
      tr("On the receiving computer, open Saved computers, select the controlling computer, then configure address "
         "recovery. It needs a selected network and a prepared identity; saving a preference alone does not make it "
         "ready.")
  );
  m_login = new deskflow::gui::OptionCheckBox(tr("Open Candidate at login"), body);
  m_login->setObjectName("startupLogin");
  layout->addWidget(m_login);
  m_crash = new deskflow::gui::OptionCheckBox(tr("Recover after an abnormal app exit"), body);
  m_crash->setObjectName("startupCrash");
  layout->addWidget(m_crash);
  auto *dependency = label("startupRecoveryDependency");
  dependency->setText(
      tr("App recovery uses the same login job. A normal quit is respected; it does not create another watchdog.")
  );
  m_preview = label("startupPreview");
  m_apply = new QPushButton(tr("Apply reviewed changes…"), body);
  m_apply->setObjectName("startupApply");
  m_apply->setAutoDefault(false);
  m_apply->setDefault(false);
  m_apply->setEnabled(false);
  layout->addWidget(m_apply);
  m_apply->ensurePolished();
  m_apply->setMinimumSize(
      m_apply->fontMetrics().horizontalAdvance(m_apply->text()) + 32, m_apply->fontMetrics().height() + 20
  );
  auto *availability = label("startupAvailability");
  layout->removeWidget(availability);
  layout->insertWidget(1, availability);
  availability->setText(
      m_change ? tr("A specific confirmation and a normal app exit are required before changing startup services.")
               : tr("System startup changes are unavailable in this build. Preview does not enable startup.")
  );
  if (!m_change) {
    m_login->hide();
    m_crash->hide();
    dependency->hide();
    m_apply->hide();
    availability->setText(
        tr("Login startup and app crash recovery are unavailable in this build. Existing system jobs are not changed. "
           "These are separate from connection recovery below.")
    );
  }
  connect(m_apply, &QPushButton::clicked, this, [this] { confirmChange(false); });
  m_history = label("startupHistory");
  m_restore = new QPushButton(tr("Restore previous startup state…"), body);
  m_restore->setObjectName("startupRestore");
  m_restore->setAutoDefault(false);
  m_restore->setDefault(false);
  m_restore->setEnabled(false);
  m_restore->ensurePolished();
  m_restore->setMinimumSize(
      m_restore->fontMetrics().horizontalAdvance(m_restore->text()) + 32, m_restore->fontMetrics().height() + 20
  );
  layout->addWidget(m_restore);
  connect(m_restore, &QPushButton::clicked, this, [this] { confirmChange(true); });
  auto *details = new QToolButton(body);
  details->setObjectName("startupDetailsToggle");
  details->setText(tr("Operation details"));
  details->setCheckable(true);
  details->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  details->setArrowType(Qt::RightArrow);
  layout->addWidget(details, 0, Qt::AlignLeft);
  m_details = new QPlainTextEdit(body);
  m_details->setObjectName("startupDetails");
  m_details->setReadOnly(true);
  m_details->setMinimumHeight(160);
  m_details->hide();
  layout->addWidget(m_details);
  connect(details, &QToolButton::toggled, this, [this, details](bool open) {
    details->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
    m_details->setVisible(open);
  });
  layout->addStretch();
  scroll->setWidget(body);
  root->addWidget(scroll, 1);
  auto *actions = new QHBoxLayout;
  actions->setSpacing(8);
  m_review = new QPushButton(tr("Preview changes"), this);
  m_review->setObjectName("startupReview");
  m_review->setProperty("primaryAction", true);
  m_refresh = new QPushButton(tr("Refresh status"), this);
  m_refresh->setObjectName("startupRefresh");
  auto *close = new QPushButton(tr("Back"), this);
  close->setObjectName("startupBack");
  for (auto *button : {m_review, m_refresh, close}) {
    button->setAutoDefault(false);
    button->setDefault(false);
    button->ensurePolished();
    button->setMinimumSize(
        button->fontMetrics().horizontalAdvance(button->text()) + 32, button->fontMetrics().height() + 20
    );
  }
  actions->addWidget(close);
  actions->addStretch();
  actions->addWidget(m_refresh);
  actions->addWidget(m_review);
  m_review->setVisible(bool(m_change));
  root->addLayout(actions);
  connect(close, &QPushButton::clicked, this, &QDialog::reject);
  connect(m_refresh, &QPushButton::clicked, this, &StartupRecoveryDialog::refresh);
  connect(m_review, &QPushButton::clicked, this, &StartupRecoveryDialog::preview);
  auto changed = [this] {
    m_crash->setEnabled(m_login->isEnabled() && m_login->isChecked());
    if (!m_login->isChecked())
      m_crash->setChecked(false);
    m_preview->setText(tr("Draft only. Preview and refresh do not change system services."));
    m_details->clear();
    m_plan = {};
    m_apply->setEnabled(false);
  };
  connect(m_login, &QCheckBox::toggled, this, changed);
  connect(m_crash, &QCheckBox::toggled, this, changed);
  QTimer::singleShot(0, this, &StartupRecoveryDialog::refresh);
}
void StartupRecoveryDialog::refresh()
{
  if (m_reading)
    return;
  m_reading = true;
  m_refresh->setEnabled(false);
  m_review->setEnabled(false);
  m_login->setEnabled(false);
  m_crash->setEnabled(false);
  m_plan = {};
  m_apply->setEnabled(false);
  m_restore->setEnabled(false);
  m_history->clear();
  m_status->setText(tr("Reading startup status…"));
  m_preview->clear();
  m_details->clear();
  // No launchctl wait runs on the GUI thread. Closing the dialog disconnects
  // this completion handler; the bounded read owns no widget or service.
  auto result = std::make_shared<Snapshot>();
  const auto reader = m_reader;
  auto *thread = QThread::create([result, reader] { *result = reader->read(); });
  connect(thread, &QThread::finished, this, [this, result] {
    m_reading = false;
    showSnapshot(*result);
  });
  connect(thread, &QThread::finished, thread, &QObject::deleteLater);
  thread->start();
}
void StartupRecoveryDialog::showSnapshot(const Snapshot &snapshot)
{
  m_snapshot = snapshot;
  m_refresh->setEnabled(true);
  const auto tidy = qScopeGuard([this] {
    for (auto *label : {m_services, m_policy, m_history, m_preview})
      label->setVisible(!label->text().isEmpty());
  });
  const auto history = snapshot.history;
  if (history.status == "handoff-failed")
    m_history->setText(tr("The last handoff failed before any service change. Refresh and review before trying again.")
    );
  else if (!history.error.isEmpty())
    m_history->setText(
        tr("The last operation could not be verified. Automatic retry and rollback are unavailable; review the local "
           "record first.")
    );
  else if (history.status == "applied")
    m_history->setText(tr("The last startup change was verified. Its startup-only backup is available for review."));
  else if (history.status == "restored")
    m_history->setText(tr("The previous startup state was restored and verified."));
  else if (history.status == "backup-failed")
    m_history->setText(tr("The last backup failed before any service change."));
  else
    m_history->clear();
  m_restore->setVisible(!history.directory.isEmpty());
  m_restore->setEnabled(history.restorable && bool(m_restoreChange));
  if (!snapshot.supported) {
    m_status->setText(
        snapshot.error == "isolated-review" ? tr("Isolated review — system startup is not queried.")
                                            : tr("Not supported on this platform yet.")
    );
    m_services->clear();
#ifdef Q_OS_WIN
    findChild<QLabel *>("startupAvailability")
        ->setText(
            tr("Windows login startup and whole-app crash recovery are not implemented in this Candidate. Opening the "
               "app or reconnecting does not enable system startup.")
        );
#endif
    m_policy->clear();
    findChild<QToolButton *>("startupDetailsToggle")->setVisible(!history.directory.isEmpty());
    m_login->hide();
    m_crash->hide();
    m_review->hide();
    m_apply->hide();
    if (snapshot.error == "unsupported-platform")
      m_refresh->hide();
    return;
  }
  bool readable = snapshot.error.isEmpty();
  for (const auto &label : {candidate, original, watchdog})
    readable = readable && snapshot.services.contains(label) && snapshot.services[label].known &&
               snapshot.services[label].error.isEmpty();
  const auto c = snapshot.services.value(candidate), old = snapshot.services.value(original),
             guard = snapshot.services.value(watchdog), awake = snapshot.services.value(keepawake);
  const bool enabled = c.loaded && !c.disabled && c.login;
  m_status->setText(
      !readable  ? tr("Status unavailable — refresh before planning a change.")
      : c.failed ? tr("Failed — the Candidate startup job did not stay running.")
      : enabled  ? tr("Candidate startup enabled")
                 : tr("Candidate startup not enabled")
  );
  QStringList current;
  if (old.running)
    current << tr("Original Deskflow is still running.");
  else if (old.loaded && !old.disabled)
    current << tr("Original Deskflow startup is still loaded.");
  if (guard.loaded && !guard.disabled)
    current << tr("Original watchdog is still loaded; idle between scheduled checks does not mean disabled.");
  if (!awake.known)
    current << tr("System sleep-protection status is unavailable.");
  else if (awake.running)
    current << tr("Existing system sleep protection is running and will be left unchanged.");
  if (c.definition.size() && !c.loaded)
    current << tr("A Candidate startup file exists but is not loaded.");
  m_services->setText(current.join('\n'));
  m_policy->setText(
      (m_preferences.paused ? tr("Current connection is paused; startup changes never clear that pause.")
       : m_preferences.resume
           ? tr("Existing resume preference is retained; it does not grant identity, trust or system permissions.")
           : tr("Automatic core start is not selected; opening the app does not enable it.")) +
      "\n" +
      (m_preferences.preventSleep
           ? tr("App sleep prevention is selected in General. It is separate from login startup.")
           : tr("App sleep prevention is off in General. It is separate from login startup."))
  );
  m_login->setChecked(enabled && c.login);
  m_crash->setChecked(enabled && c.crashRecovery);
  m_login->setEnabled(readable && bool(m_change));
  m_crash->setEnabled(readable && bool(m_change) && m_login->isChecked());
  m_review->setEnabled(readable && bool(m_change));
  if (m_change)
    m_preview->setText(tr("Draft only. Preview and refresh do not change system services."));
  else
    m_preview->clear();
  if (!history.directory.isEmpty())
    m_details->setPlainText(
        tr("Local operation record: %1")
            .arg(history.status == "handoff-failed" ? history.directory : history.directory + "/journal.json") +
        "\n" + history.error
    );
}
void StartupRecoveryDialog::preview()
{
  const auto plan = prepare(m_snapshot, {m_login->isChecked(), m_crash->isChecked()});
  m_plan = plan;
  m_apply->setEnabled(plan.valid() && bool(m_change));
  m_preview->show();
  if (!plan.valid()) {
    m_preview->setText(
        plan.error == "no-change"
            ? tr("No change requested.")
            : tr("Cannot prepare a safe change. Refresh the status and review the existing service files.")
    );
    m_details->clear();
    return;
  }
  m_status->setText(tr("Confirmation required — nothing has been changed."));
  m_preview->setText(
      m_change
          ? tr("Review the exact operations below. Applying them closes this app normally before the short-lived "
               "transaction runs. Failure is reported through its local journal; uncertain results require review.")
          : (m_login->isChecked()
                 ? tr("The plan backs up the current files, retires the original watchdog before its startup job, and "
                      "uses one Candidate login job. A coordinated single-instance handoff and explicit confirmation "
                      "are required. This build only prepares the plan.")
                 : tr("The plan disables the Candidate login job, preserving its file for rollback. It does not stop "
                      "the original protection. Explicit confirmation is required. This build only prepares the plan."))
  );
  m_details->setPlainText(
      plan.commands().join('\n') + "\n\n" +
      tr("Rollback restores only the reviewed startup jobs and their previous enabled/loaded states. Sleep protection, "
         "permissions, identity and trust are excluded.")
  );
}
bool StartupRecoveryDialog::beginHandoff(const Plan *plan, const RecoveryRecord *record, const QByteArray &digest)
{
  auto *handoff = new StartupHandoff(qApp->property("candidateInstanceKey").toString(), qApp);
  connect(this, &QObject::destroyed, handoff, [handoff] {
    if (!handoff->ready())
      handoff->deleteLater();
  });
  connect(handoff, &StartupHandoff::failed, this, [this, handoff](const QString &reason) {
    m_status->setText(tr("Handoff failed — no system change was requested. Refresh before trying again."));
    m_details->appendPlainText(reason);
    m_refresh->setEnabled(true);
    handoff->deleteLater();
  });
  connect(handoff, &StartupHandoff::readyToQuit, this, &StartupRecoveryDialog::requestStartupQuit);
  const bool started = plan ? handoff->begin(*plan, digest) : record && handoff->beginRestore(*record, digest);
  if (!started)
    handoff->deleteLater();
  return started;
}
void StartupRecoveryDialog::confirmChange(bool restoring)
{
  if (restoring ? !m_restoreChange : (!m_change || !m_plan.valid()))
    return;
  RecoveryRecord recovery;
  if (restoring) {
    recovery = inspectRecoveryBackup(m_snapshot, m_snapshot.history.directory);
    if (!recovery.history.restorable || recovery.history.restoreDigest != m_snapshot.history.restoreDigest) {
      m_history->setText(tr("The backup or current state changed. Refresh before reviewing a rollback."));
      m_restore->setEnabled(false);
      return;
    }
  }
  const auto reviewed = restoring ? recovery.plan : m_plan;
  const auto digest = restoring ? recovery.history.restoreDigest : reviewed.approvalDigest;
  QDialog confirmation(this);
  confirmation.setObjectName("startupConfirmation");
  confirmation.setWindowTitle(tr("Confirm startup changes"));
  confirmation.setMinimumWidth(480);
  auto *layout = new QVBoxLayout(&confirmation);
  layout->setContentsMargins(24, 24, 24, 24);
  layout->setSpacing(16);
  auto *text = new QLabel(
      (restoring
           ? tr("Restore only the reviewed startup jobs to their saved states. This app closes normally first; "
                "restoring the original jobs may resume the original app. Application configuration and certificates "
                "are not restored. Keep the separate old-version configuration backup for a software rollback.")
           : tr("This changes persistent login and crash-recovery services. The reviewed original watchdog/startup "
                "jobs may stop, and input sharing may pause while this app exits normally. Normal quit and the saved "
                "pause remain respected. No sleep service, permission, configuration or identity is migrated.")) +
          "\n\n" +
          tr("Application: %1\nVersion: %2\nReviewed plan: %3")
              .arg(reviewed.before.executable, reviewed.before.version, QString::fromLatin1(digest)),
      &confirmation
  );
  text->setTextFormat(Qt::PlainText);
  text->setWordWrap(true);
  layout->addWidget(text);
  auto *cancel = new QPushButton(tr("Cancel"), &confirmation);
  cancel->setObjectName("startupCancelConfirmation");
  auto *confirm = new QPushButton(tr("Confirm these changes"), &confirmation);
  confirm->setObjectName("startupConfirm");
  auto *buttons = new QHBoxLayout;
  buttons->addWidget(cancel);
  buttons->addStretch();
  buttons->addWidget(confirm);
  layout->addLayout(buttons);
  for (auto *b : {cancel, confirm}) {
    b->setAutoDefault(false);
    b->setDefault(false);
    b->ensurePolished();
    b->setMinimumSize(b->fontMetrics().horizontalAdvance(b->text()) + 32, b->fontMetrics().height() + 20);
  }
  connect(cancel, &QPushButton::clicked, &confirmation, &QDialog::reject);
  connect(confirm, &QPushButton::clicked, &confirmation, &QDialog::accept);
  if (confirmation.exec() != QDialog::Accepted)
    return;
  if (!restoring && reviewed.approvalDigest != m_plan.approvalDigest)
    return;
  m_apply->setEnabled(false);
  m_review->setEnabled(false);
  m_login->setEnabled(false);
  m_crash->setEnabled(false);
  m_refresh->setEnabled(false);
  m_restore->setEnabled(false);
  if (!(restoring ? m_restoreChange(recovery, digest) : m_change(reviewed, digest))) {
    m_status->setText(tr("Handoff unavailable — no system change was requested."));
    m_refresh->setEnabled(true);
    return;
  }
  m_status->setText(tr("Waiting for the confirmed handoff. Services have not been changed yet."));
}
} // namespace deskflow::gui
