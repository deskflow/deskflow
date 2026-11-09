// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception

#include "gui/dialogs/CandidatePermissionDialog.h"

#include <QGuiApplication>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>

using deskflow::gui::AccessibilityPermissionProvider;
using deskflow::gui::CandidatePermissionDialog;

class FakePermissionProvider final : public AccessibilityPermissionProvider
{
public:
  enum class Outcome
  {
    Undetermined,
    Denied,
    Granted
  };
  Outcome outcome = Outcome::Undetermined;
  Outcome requestOutcome = Outcome::Undetermined;
  int requests = 0;
  mutable int checks = 0;

  bool isGranted() const override
  {
    ++checks;
    return outcome == Outcome::Granted;
  }
  void request() override
  {
    ++requests;
    outcome = requestOutcome;
  }
};

class CandidatePermissionDialogTests : public QObject
{
  Q_OBJECT

  static QPushButton *button(CandidatePermissionDialog &dialog, const char *name)
  {
    return dialog.findChild<QPushButton *>(name);
  }
  static void returnToApplication()
  {
    QMetaObject::invokeMethod(
        qGuiApp, "applicationStateChanged", Qt::DirectConnection, Q_ARG(Qt::ApplicationState, Qt::ApplicationActive)
    );
  }

private Q_SLOTS:
  void undeterminedDoesNotRequestOnOpenOrReturn()
  {
    FakePermissionProvider provider;
    CandidatePermissionDialog dialog(provider, 27);
    dialog.show();
    QSignalSpy accepted(&dialog, &QDialog::accepted);
    returnToApplication();
    button(dialog, "checkPermission")->click();
    QCOMPARE(provider.requests, 0);
    QVERIFY(dialog.isVisible());
    QVERIFY(!button(dialog, "continuePaused")->isEnabled());
    QCOMPARE(accepted.count(), 0);
  }

  void denialKeepsTheGuideOpen()
  {
    FakePermissionProvider provider;
    provider.requestOutcome = FakePermissionProvider::Outcome::Denied;
    CandidatePermissionDialog dialog(provider, 27);
    dialog.show();
    button(dialog, "requestPermission")->click();
    returnToApplication();
    button(dialog, "checkPermission")->click();
    QCOMPARE(provider.requests, 1);
    QVERIFY(dialog.isVisible());
    QVERIFY(!button(dialog, "continuePaused")->isEnabled());
    QVERIFY(dialog.findChild<QLabel *>("permissionStatus")
                ->text()
                .contains("does not currently trust this running instance"));
  }

  void approvalDoesNotContinueAutomatically()
  {
    FakePermissionProvider provider;
    provider.requestOutcome = FakePermissionProvider::Outcome::Granted;
    CandidatePermissionDialog dialog(provider, 27);
    dialog.show();
    QSignalSpy accepted(&dialog, &QDialog::accepted);
    button(dialog, "requestPermission")->click();
    QCOMPARE(provider.requests, 1);
    QVERIFY(button(dialog, "continuePaused")->isEnabled());
    QCOMPARE(accepted.count(), 0);
    QVERIFY(dialog.isVisible());
    button(dialog, "continuePaused")->click();
    QCOMPARE(accepted.count(), 1);
    QCOMPARE(dialog.result(), int(QDialog::Accepted));
  }

  void approvalOnReturnOnlyUpdatesState()
  {
    FakePermissionProvider provider;
    CandidatePermissionDialog dialog(provider, 27);
    dialog.show();
    QSignalSpy accepted(&dialog, &QDialog::accepted);
    provider.outcome = FakePermissionProvider::Outcome::Granted;
    returnToApplication();
    QVERIFY(button(dialog, "continuePaused")->isEnabled());
    QVERIFY(!button(dialog, "requestPermission")->isEnabled());
    QCOMPARE(provider.requests, 0);
    QCOMPARE(accepted.count(), 0);
  }

  void repeatedClicksDoNotDuplicateAnOutstandingRequest()
  {
    FakePermissionProvider provider;
    CandidatePermissionDialog dialog(provider, 27);
    auto *request = button(dialog, "requestPermission");
    request->click();
    request->click();
    request->click();
    QCOMPARE(provider.requests, 1);
    QVERIFY(!request->isEnabled());
    button(dialog, "checkPermission")->click();
    QCOMPARE(provider.requests, 1);
    QVERIFY(request->isEnabled());
    request->click(); // A new explicit request after checking is allowed.
    QCOMPARE(provider.requests, 2);
  }

  void alreadyGrantedStillNeedsExplicitContinue()
  {
    FakePermissionProvider provider;
    provider.outcome = FakePermissionProvider::Outcome::Granted;
    CandidatePermissionDialog dialog(provider, 27);
    QSignalSpy accepted(&dialog, &QDialog::accepted);
    returnToApplication();
    QVERIFY(button(dialog, "continuePaused")->isEnabled());
    QCOMPARE(provider.requests, 0);
    QCOMPARE(accepted.count(), 0);
  }

  void revocationBeforeContinueDoesNotAccept()
  {
    FakePermissionProvider provider;
    provider.outcome = FakePermissionProvider::Outcome::Granted;
    CandidatePermissionDialog dialog(provider, 27);
    dialog.show();
    QSignalSpy accepted(&dialog, &QDialog::accepted);
    provider.outcome = FakePermissionProvider::Outcome::Denied;
    button(dialog, "continuePaused")->click();
    QCOMPARE(accepted.count(), 0);
    QVERIFY(!button(dialog, "continuePaused")->isEnabled());
    QVERIFY(dialog.isVisible());
  }

  void quitStopsFurtherChecksAndRequests()
  {
    FakePermissionProvider provider;
    CandidatePermissionDialog dialog(provider, 27);
    dialog.show();
    QSignalSpy rejected(&dialog, &QDialog::rejected);
    button(dialog, "requestPermission")->click();
    button(dialog, "quitPermissionGuide")->click();
    const int checks = provider.checks;
    returnToApplication();
    button(dialog, "checkPermission")->click();
    QCOMPARE(provider.checks, checks);
    QCOMPARE(provider.requests, 1);
    QCOMPARE(rejected.count(), 1);
    QVERIFY(!dialog.isVisible());
  }

  void windowCloseDoesNotGrantOrRequest()
  {
    FakePermissionProvider provider;
    CandidatePermissionDialog dialog(provider, 27);
    dialog.show();
    QSignalSpy rejected(&dialog, &QDialog::rejected);
    dialog.close();
    QCOMPARE(rejected.count(), 1);
    QCOMPARE(provider.requests, 0);
  }

  void enterDoesNotRequestPermission()
  {
    FakePermissionProvider provider;
    CandidatePermissionDialog dialog(provider, 27);
    dialog.show();
    QTest::keyClick(&dialog, Qt::Key_Return);
    QCOMPARE(provider.requests, 0);
    QVERIFY(dialog.isVisible());
  }

  void systemSpecificName_data()
  {
    QTest::addColumn<int>("major");
    QTest::addColumn<QString>("name");
    QTest::newRow("macOS26") << 26 << QStringLiteral("Accessibility");
    QTest::newRow("macOS27") << 27 << QStringLiteral("Device Control and Data Access");
  }
  void systemSpecificName()
  {
    QFETCH(int, major);
    QFETCH(QString, name);
    FakePermissionProvider provider;
    CandidatePermissionDialog dialog(provider, major);
    QVERIFY(dialog.findChild<QLabel *>("permissionInstructions")->text().endsWith(name + '.'));
  }
};

QTEST_MAIN(CandidatePermissionDialogTests)
#include "CandidatePermissionDialogTests.moc"
