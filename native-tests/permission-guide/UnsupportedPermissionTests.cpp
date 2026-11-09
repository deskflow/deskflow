// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "gui/dialogs/CandidatePermissionDialog.h"
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QtTest>
class UnusedProvider final : public deskflow::gui::AccessibilityPermissionProvider
{
public:
  mutable int checks = 0;
  int requests = 0;
  bool isGranted() const override
  {
    ++checks;
    return true;
  }
  void request() override
  {
    ++requests;
  }
};
class UnsupportedPermissionTests final : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void unsupportedPlatformDoesNotRequestOrImplyMacSettings()
  {
    UnusedProvider provider;
    deskflow::gui::CandidatePermissionDialog dialog(provider, 27);
    dialog.show();
    QSignalSpy accepted(&dialog, &QDialog::accepted);
    QVERIFY(dialog.findChild<QLabel *>("permissionStatus")->text().contains("not available on this platform"));
    QVERIFY(!dialog.findChild<QLabel *>("permissionInstructions")->isVisible());
    auto *request = dialog.findChild<QPushButton *>("requestPermission");
    QVERIFY(!request->isVisible());
    request->setEnabled(true);
    request->click();
    dialog.accept();
    QCOMPARE(provider.requests, 0);
    QCOMPARE(provider.checks, 0);
    QCOMPARE(accepted.count(), 0);
    dialog.findChild<QPushButton *>("quitPermissionGuide")->click();
    QVERIFY(!dialog.isVisible());
  }
};
QTEST_MAIN(UnsupportedPermissionTests)
#include "UnsupportedPermissionTests.moc"
