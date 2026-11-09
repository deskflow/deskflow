// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "StartupFixture.h"
#include "common/CandidatePolicy.h"
#include "common/Settings.h"
#include "gui/dialogs/SettingsDialog.h"
#include "gui/dialogs/StartupRecoveryDialog.h"
#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QLabel>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
using namespace startup_fixture;
class SettingsEntryTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void realSettingsEntryReturnsWithoutChangingPreferences_data()
  {
    QTest::addColumn<bool>("escape");
    QTest::newRow("back") << false;
    QTest::newRow("escape") << true;
  }
  void realSettingsEntryReturnsWithoutChangingPreferences()
  {
    QFETCH(bool, escape);
    Settings::setValue(deskflow::candidate::pausedKey, true);
    Settings::setValue(Settings::Gui::AutoStartCore, false);
    Settings::setValue(Settings::Core::PreventSleep, true);
    Settings::setValue(Settings::Core::ComputerName, "SyntheticStartupFixture");
    ServerConfig config(5, 3);
    auto reader = std::make_shared<FakeReader>();
    SettingsDialog settings(nullptr, config, reader);
    settings.show();
    if (escape)
      settings.findChild<QCheckBox *>("cbPreventSleep")
          ->setChecked(false); // An unsaved ordinary preference stays a draft.
    auto *entry = settings.findChild<QPushButton *>("startupRecoverySettings");
    QVERIFY(entry);
    QVERIFY(entry->isVisible());
    QTimer timer;
    timer.setInterval(10);
    bool reviewed = false;
    connect(&timer, &QTimer::timeout, &settings, [&] {
      auto *dialog = qobject_cast<deskflow::gui::StartupRecoveryDialog *>(QApplication::activeModalWidget());
      if (!dialog || reader->reads == 0 || !dialog->findChild<QPushButton *>("startupRefresh")->isEnabled())
        return;
      timer.stop();
      dialog->findChild<QCheckBox *>("startupLogin")->setChecked(true);
      if (escape)
        QVERIFY(dialog->findChild<QLabel *>("startupPolicy")
                    ->text()
                    .contains(
                        QCoreApplication::translate(
                            "deskflow::gui::StartupRecoveryDialog",
                            "App sleep prevention is off in General. It is separate from login startup."
                        )
                    ));
      dialog->findChild<QPushButton *>("startupReview")->click();
      reviewed = true;
      if (escape)
        QTest::keyClick(dialog, Qt::Key_Escape);
      else
        dialog->findChild<QPushButton *>("startupBack")->click();
    });
    timer.start();
    QTimer::singleShot(3000, &settings, [&] {
      if (auto *d = qobject_cast<QDialog *>(QApplication::activeModalWidget()))
        d->reject();
    });
    entry->click();
    QVERIFY(reviewed);
    QVERIFY(settings.isVisible());
    QVERIFY(Settings::value(deskflow::candidate::pausedKey).toBool());
    QVERIFY(!Settings::value(Settings::Gui::AutoStartCore).toBool());
    QVERIFY(Settings::value(Settings::Core::PreventSleep).toBool());
    QVERIFY(!reader->state.services[candidate].loaded);
    settings.reject();
  }
};
int main(int argc, char **argv)
{
  const auto root = qEnvironmentVariable("DESKFLOW_TEST_ROOT");
  if (root.isEmpty() || !QDir(root).isAbsolute() || !QDir(root).exists())
    return 2;
  QTemporaryDir fixture(root + "/startup-entry-XXXXXX");
  if (!fixture.isValid())
    return 2;
  qputenv("XDG_CONFIG_HOME", (fixture.path() + "/config").toUtf8());
  qputenv("XDG_STATE_HOME", (fixture.path() + "/state").toUtf8());
  QDir().mkpath(fixture.path() + "/config/DeskflowLab");
  QDir().mkpath(fixture.path() + "/state");
  QApplication app(argc, argv);
  SettingsEntryTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "SettingsEntryTests.moc"
