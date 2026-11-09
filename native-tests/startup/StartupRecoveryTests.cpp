// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "StartupFixture.h"
#include "common/CandidatePolicy.h"
#include "gui/dialogs/StartupRecoveryDialog.h"
#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScopeGuard>
#include <QScrollArea>
#include <QStyleOptionButton>
#include <QTest>
#include <QTextLayout>
#include <QTimer>
#include <QTranslator>
using namespace startup_fixture;
using deskflow::gui::StartupRecoveryDialog;
class StartupRecoveryTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void confirmationCancelEnterAndExplicitAction()
  {
    auto reader = std::make_shared<FakeReader>();
    int requests = 0;
    QByteArray confirmed;
    StartupRecoveryDialog dialog(reader, {true, false, true}, nullptr, [&](const Plan &p, const QByteArray &digest) {
      ++requests;
      confirmed = digest;
      Q_ASSERT(p.approvalDigest == digest);
      return false;
    });
    dialog.show();
    QTRY_VERIFY(reader->reads > 0 && dialog.findChild<QPushButton *>("startupReview")->isEnabled());
    dialog.findChild<QCheckBox *>("startupLogin")->setChecked(true);
    dialog.findChild<QPushButton *>("startupReview")->click();
    auto *apply = dialog.findChild<QPushButton *>("startupApply");
    QVERIFY(apply->isEnabled());
    QTimer::singleShot(0, [] {
      auto *modal = QApplication::activeModalWidget();
      QVERIFY(modal);
      QTest::keyClick(modal, Qt::Key_Return);
      QVERIFY(modal->isVisible());
      modal->findChild<QPushButton *>("startupCancelConfirmation")->click();
    });
    apply->click();
    QCOMPARE(requests, 0);
    QTimer::singleShot(0, [] {
      auto *modal = QApplication::activeModalWidget();
      QVERIFY(modal);
      modal->findChild<QPushButton *>("startupConfirm")->click();
    });
    apply->click();
    QCOMPARE(requests, 1);
    QVERIFY(!confirmed.isEmpty());
    QVERIFY(dialog.findChild<QLabel *>("startupStatus")->text().contains("no system change"));
    QVERIFY(!reader->state.services[candidate].loaded);
    QVERIFY(reader->state.services[original].loaded);
  }
  void productionGateIsExplicitAndUnavailable()
  {
    auto reader = std::make_shared<FakeReader>();
    StartupRecoveryDialog dialog(reader, {true, false, false});
    dialog.show();
    QTRY_VERIFY(reader->reads > 0 && dialog.findChild<QPushButton *>("startupReview")->isEnabled());
    dialog.findChild<QCheckBox *>("startupLogin")->setChecked(true);
    dialog.findChild<QPushButton *>("startupReview")->click();
    QVERIFY(!dialog.findChild<QPushButton *>("startupApply")->isEnabled());
    QVERIFY(dialog.findChild<QLabel *>("startupAvailability")->text().contains("unavailable"));
  }
  void planReusesOneJobAndRetiresWatchdogFirst()
  {
    const auto p = prepare(snapshot(), {true, true});
    QVERIFY2(p.valid(), qPrintable(p.error));
    QCOMPARE(p.operations.front().action, Action::Backup);
    QCOMPARE(p.operations[1].label, watchdog);
    QCOMPARE(p.operations[2].action, Action::Unload);
    QCOMPARE(p.operations[3].label, original);
    QCOMPARE(p.operations.back().action, Action::Load);
    for (const auto &op : p.operations)
      QVERIFY(op.label != keepawake);
    const auto bytes = p.after.services[candidate].definition;
    QVERIFY(bytes.contains("SuccessfulExit"));
    QVERIFY(!bytes.contains("StartInterval"));
    QVERIFY(!bytes.contains("caffeinate"));
    const auto loginOnly = prepare(snapshot(), {true, false});
    QVERIFY(loginOnly.valid());
    QVERIFY(!loginOnly.after.services[candidate].definition.contains("KeepAlive"));
    QVERIFY(loginOnly.approvalDigest != p.approvalDigest);
    QVERIFY(!prepare(snapshot(), {false, true}).valid());
    QVERIFY(!prepare(snapshot(), {false, false}).valid());
    QVERIFY(!deskflow::candidate::mayResume(true, true));
    QVERIFY(deskflow::candidate::mayResume(true, false));
  }
  void approvalHandoffAndFreshSnapshotAreRequired()
  {
    FakeExecutor e;
    const auto p = prepare(e.state, {true, true});
    QCOMPARE(apply(p, {}, true, e).outcome, Outcome::NeedsConfirmation);
    QCOMPARE(apply(p, p.approvalDigest, false, e).outcome, Outcome::NeedsConfirmation);
    QCOMPARE(e.calls, 0);
    e.permitted = false;
    QCOMPARE(apply(p, p.approvalDigest, true, e).outcome, Outcome::NeedsConfirmation);
    QCOMPARE(e.calls, 0);
    e.permitted = true;
    e.state.watchdogHash = "changed-after-preview";
    QCOMPARE(apply(p, p.approvalDigest, true, e).outcome, Outcome::Stale);
    QCOMPARE(e.calls, 0);
    e.state = p.before;
    auto changed = p;
    changed.operations[1].label = keepawake;
    QCOMPARE(apply(changed, p.approvalDigest, true, e).outcome, Outcome::Stale);
    QCOMPARE(e.calls, 0);
  }
  void verifiedApplyAndRollbackPreserveOtherServices()
  {
    FakeExecutor e;
    const auto before = e.state;
    const auto p = prepare(before, {true, true});
    const auto result = apply(p, p.approvalDigest, true, e);
    QCOMPARE(result.outcome, Outcome::Applied);
    QCOMPARE(e.state.revision(), p.after.revision());
    QVERIFY(!e.state.services[watchdog].loaded);
    e.state.services[keepawake].definition = "external-sleep-change";
    QCOMPARE(rollback(p, result, p.approvalDigest, e).outcome, Outcome::RolledBack);
    QCOMPARE(e.state.revision(), before.revision());
    QVERIFY(e.state.services[original].running);
    QCOMPARE(e.state.services[keepawake].definition, QByteArray("external-sleep-change"));
  }
  void everyKnownFailureRestoresPreviousState_data()
  {
    QTest::addColumn<int>("step");
    for (int n = 1; n <= 8; ++n)
      QTest::newRow(qPrintable(QString::number(n))) << n;
  }
  void everyKnownFailureRestoresPreviousState()
  {
    QFETCH(int, step);
    FakeExecutor e;
    e.failAt = step;
    const auto p = prepare(e.state, {true, true});
    QVERIFY2(p.valid(), qPrintable(p.error));
    QCOMPARE(p.operations.size(), 8);
    QCOMPARE(apply(p, p.approvalDigest, true, e).outcome, Outcome::FailedRolledBack);
    QCOMPARE(e.state.revision(), p.before.revision());
    QCOMPARE(e.calls, step);
    QCOMPARE(e.restores, step == 1 ? 0 : 1);
  }
  void unknownOutcomesAndFalseSuccessNeverBecomeEnabled()
  {
    FakeExecutor uncertain;
    uncertain.uncertainAt = 3;
    const auto p = prepare(uncertain.state, {true, true});
    QCOMPARE(apply(p, p.approvalDigest, true, uncertain).outcome, Outcome::RecoveryRequired);
    QCOMPARE(uncertain.calls, 3);
    QCOMPARE(uncertain.restores, 0);
    FakeExecutor noReceipt;
    noReceipt.emptyReceipt = true;
    QCOMPARE(apply(p, p.approvalDigest, true, noReceipt).outcome, Outcome::RecoveryRequired);
    QCOMPARE(noReceipt.calls, 1);
    FakeExecutor liar;
    liar.omitEffects = true;
    QCOMPARE(apply(p, p.approvalDigest, true, liar).outcome, Outcome::RecoveryRequired);
    FakeExecutor failedApp;
    failedApp.failToRun = true;
    QCOMPARE(apply(p, p.approvalDigest, true, failedApp).outcome, Outcome::FailedRolledBack);
    QVERIFY(failedApp.state.services[original].running);
    FakeExecutor failedRestore;
    failedRestore.failAt = 3;
    failedRestore.restoreFails = true;
    QCOMPARE(apply(p, p.approvalDigest, true, failedRestore).outcome, Outcome::RecoveryRequired);
  }
  void rollbackCannotOverwriteLaterChanges()
  {
    FakeExecutor e;
    const auto p = prepare(e.state, {true, true});
    const auto result = apply(p, p.approvalDigest, true, e);
    e.state.services[candidate].definition += "later-edit";
    QCOMPARE(rollback(p, result, p.approvalDigest, e).outcome, Outcome::Stale);
    QCOMPARE(e.restores, 0);
  }
  void concurrentChangeAndMismatchedReceiptCannotBeRolledBack()
  {
    FakeExecutor drift;
    drift.failAt = 3;
    drift.driftAt = 3;
    const auto p = prepare(drift.state, {true, true});
    QCOMPARE(apply(p, p.approvalDigest, true, drift).outcome, Outcome::RecoveryRequired);
    QCOMPARE(drift.restores, 0);
    QCOMPARE(drift.state.services[original].definition, QByteArray("concurrent-edit"));
    FakeExecutor e;
    auto result = apply(p, p.approvalDigest, true, e);
    QCOMPARE(result.outcome, Outcome::Applied);
    result.planDigest = "another-plan";
    QCOMPARE(rollback(p, result, p.approvalDigest, e).outcome, Outcome::NeedsConfirmation);
    QCOMPARE(e.restores, 0);
  }
  void disablingCandidateDoesNotRetireOriginalAgain()
  {
    auto s = snapshot();
    s.services[candidate].definition = "candidate-existing";
    s.services[candidate].loaded = true;
    const auto p = prepare(s, {false, false});
    QVERIFY(p.valid());
    for (const auto &op : p.operations)
      QVERIFY(op.label.isEmpty() || op.label == candidate);
    QCOMPARE(p.after.services[original].loaded, s.services[original].loaded);
    QCOMPARE(p.after.services[watchdog].loaded, s.services[watchdog].loaded);
  }
  void unknownStatusAndUnscopedPathsCannotProducePlan()
  {
    auto s = snapshot();
    s.services[original].known = false;
    QVERIFY(!prepare(s, {true, true}).valid());
    s = snapshot();
    s.services[candidate].path = "/Library/LaunchDaemons/unrelated.plist";
    QVERIFY(!prepare(s, {true, true}).valid());
    s = snapshot();
    s.domain = "system";
    QVERIFY(!prepare(s, {true, true}).valid());
    s = snapshot();
    s.supported = false;
    QVERIFY(!prepare(s, {true, true}).valid());
    QVERIFY(!makeReadOnlyReader()->read().supported); // Lab cannot inspect live jobs.
  }
  void readableUiAndTruthfulStates_data()
  {
    QTest::addColumn<bool>("zh");
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("state");
    for (bool zh : {false, true})
      for (int width : {520, 640})
        for (int state : {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10})
          QTest::newRow(qPrintable(QString("%1-%2-%3").arg(zh ? "zh" : "en").arg(width).arg(state)))
              << zh << width << state;
  }
  void readableUiAndTruthfulStates()
  {
    QFETCH(bool, zh);
    QFETCH(int, width);
    QFETCH(int, state);
    QTranslator translator;
    if (zh) {
      QVERIFY(translator.load(":/translations/deskflow_zh_CN.qm"));
      qApp->installTranslator(&translator);
    }
    const auto restore = qScopeGuard([&] { qApp->removeTranslator(&translator); });
    auto reader = std::make_shared<FakeReader>();
    if (state == 1 || state == 2) {
      reader->state = prepare(reader->state, {true, true}).after;
      reader->state.services[candidate].failed = state == 2;
      reader->state.services[candidate].running = state != 2;
    }
    if (state == 3)
      reader->state.services[original].known = false;
    if (state == 4)
      reader->state.supported = false;
    if (state == 5) {
      reader->state.services[candidate].loaded = true;
      reader->state.services[candidate].definition = "loaded-only-not-login";
    }
    if (state >= 6) {
      reader->state.history.directory = "fixture-backup";
      reader->state.history.status =
          QStringList{"applied", "restored", "uncertain", "handoff-failed", "backup-failed"}[state - 6];
      reader->state.history.restorable = state == 6;
      if (state == 8)
        reader->state.history.error = "operation-needs-review";
    }
    StartupRecoveryDialog dialog(reader, {true, true, true});
    dialog.resize(width, 500);
    dialog.show();
    auto *refresh = dialog.findChild<QPushButton *>("startupRefresh");
    QTRY_VERIFY(refresh->isEnabled() && reader->reads > 0);
    auto *review = dialog.findChild<QPushButton *>("startupReview");
    QCOMPARE(review->isEnabled(), state != 3 && state != 4);
    const auto status = dialog.findChild<QLabel *>("startupStatus")->text();
    if (state >= 6) {
      QVERIFY(!dialog.findChild<QLabel *>("startupHistory")->text().isEmpty());
      QVERIFY(!dialog.findChild<QPushButton *>("startupRestore")->isEnabled());
    }
    if (state == 2)
      QVERIFY(status.contains(zh ? QString::fromUtf8("失败") : "Failed"));
    if (state == 5) {
      QVERIFY(status.contains(zh ? QString::fromUtf8("未启用") : "not enabled"));
      QVERIFY(!dialog.findChild<QCheckBox *>("startupLogin")->isChecked());
    }
    if (state == 0) {
      QVERIFY(dialog.findChild<QLabel *>("startupServices")
                  ->text()
                  .contains(zh ? QString::fromUtf8("旧版 Deskflow 仍在运行") : "Original Deskflow is still running"));
      dialog.findChild<QCheckBox *>("startupLogin")->setChecked(true);
      dialog.findChild<QCheckBox *>("startupCrash")->setChecked(true);
      review->click();
      QVERIFY(dialog.findChild<QLabel *>("startupStatus")
                  ->text()
                  .contains(zh ? QString::fromUtf8("需确认") : "Confirmation required"));
      QVERIFY(!reader->state.services[candidate].loaded); // Preview does not install.
    }
    QTest::qWait(10);
    QCOMPARE(dialog.width(), width);
    for (auto *button : dialog.findChildren<QPushButton *>())
      if (button->isVisible()) {
        if (auto *scroll = dialog.findChild<QScrollArea *>(); scroll && scroll->widget()->isAncestorOf(button)) {
          scroll->ensureWidgetVisible(button);
          QTest::qWait(1);
        }
        QStyleOptionButton option;
        option.initFrom(button);
        option.rect = button->rect();
        option.text = button->text();
        const auto r = button->style()->subElementRect(QStyle::SE_PushButtonContents, &option, button);
        QFontMetricsF metrics(button->font(), button);
        QVERIFY2(r.width() + 0.5 >= metrics.horizontalAdvance(button->text()), qPrintable(button->text()));
        QVERIFY(r.height() + 0.5 >= metrics.height());
        QVERIFY(dialog.rect().contains(QRect(button->mapTo(&dialog, QPoint()), button->size())));
      }
    for (auto *box : dialog.findChildren<QCheckBox *>())
      if (box->isVisible()) {
        QStyleOptionButton option;
        option.initFrom(box);
        option.rect = box->rect();
        option.text = box->text();
        const auto r = box->style()->subElementRect(QStyle::SE_CheckBoxContents, &option, box);
        QVERIFY2(
            r.width() + 0.5 >= QFontMetricsF(box->font(), box).horizontalAdvance(box->text()), qPrintable(box->text())
        );
      }
    for (auto *label : dialog.findChildren<QLabel *>())
      if (label->isVisible() && label->wordWrap()) {
        qreal needed = 0;
        for (const auto &paragraph : label->text().split('\n')) {
          QTextLayout text(paragraph, label->font());
          QTextOption option;
          option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
          text.setTextOption(option);
          text.beginLayout();
          while (true) {
            auto line = text.createLine();
            if (!line.isValid())
              break;
            line.setLineWidth(label->contentsRect().width());
            needed += line.height();
          }
          text.endLayout();
        }
        QVERIFY2(label->height() + 1 >= needed, qPrintable(label->objectName() + ": " + label->text()));
      }
    const auto output = qEnvironmentVariable("STARTUP_TEST_SCREENSHOTS");
    if (zh && width == 520 && !output.isEmpty()) {
      QDir().mkpath(output);
      QVERIFY(dialog.grab().save(output + QString("/fixture-zh-state-%1.png").arg(state)));
    }
    dialog.findChild<QPushButton *>("startupBack")->click();
    QVERIFY(!dialog.isVisible());
  }
};
QTEST_MAIN(StartupRecoveryTests)
#include "StartupRecoveryTests.moc"
