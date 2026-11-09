// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "gui/dialogs/WindowsStartupDialog.h"
#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QFontInfo>
#include <QLabel>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTranslator>
using namespace deskflow::gui;
class WindowsStartupDialogTests : public QObject
{
  Q_OBJECT
  struct Fake : winstartup::System
  {
    winstartup::State state;
    int writes = 0;
    bool fail = false;
    Fake()
    {
      state.available = true;
      state.userSid = "S-1-5-21-1234";
      state.executable = qEnvironmentVariable("DESKFLOW_TEST_ROOT") + "/Candidate.exe";
      state.executableHash = QByteArray(64, 'a');
    }
    winstartup::State read() override
    {
      return state;
    }
    winstartup::Effect replace(const QByteArray &v, const QByteArray &expected) override
    {
      if (state.revision() != expected)
        return winstartup::Effect::Unchanged;
      ++writes;
      if (fail)
        return winstartup::Effect::Unchanged;
      state.definition = v;
      state.exists = !v.isEmpty();
      return winstartup::Effect::Applied;
    }
  };
  void answer(QMessageBox::StandardButton choice)
  {
    QTimer::singleShot(0, [choice] {
      auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
      if (box)
        box->button(choice)->click();
    });
  }
private Q_SLOTS:
  void initTestCase()
  {
    QVERIFY(QFontDatabase::addApplicationFont(qEnvironmentVariable("WINDIR") + "/Fonts/msyh.ttc") >= 0);
    QVERIFY(QFontDatabase::addApplicationFont(qEnvironmentVariable("WINDIR") + "/Fonts/msyhbd.ttc") >= 0);
    qApp->setFont(QFont("Microsoft YaHei UI", 9));
    qApp->setStyle("Fusion");
    QCOMPARE(QFontInfo(qApp->font()).family(), QString("Microsoft YaHei UI"));
    QVERIFY(QFontMetrics(qApp->font()).inFontUcs4(0x767b));
    QVERIFY(qAbs(qApp->devicePixelRatio() - qEnvironmentVariable("QT_SCALE_FACTOR").toDouble()) < .01);
  }

  void visualStates_data()
  {
    QTest::addColumn<bool>("zh");
    QTest::addColumn<int>("width");
    QTest::addColumn<bool>("conflict");
    for (bool zh : {false, true})
      for (int width : {520, 560, 640})
        for (bool conflict : {false, true})
          QTest::newRow(qPrintable(QString("%1-%2-%3").arg(zh).arg(width).arg(conflict))) << zh << width << conflict;
  }
  void visualStates()
  {
    QFETCH(bool, zh);
    QFETCH(int, width);
    QFETCH(bool, conflict);
    QTranslator app, qt;
    if (zh) {
      QVERIFY(app.load(":/translations/deskflow_zh_CN.qm"));
      qApp->installTranslator(&app);
      QVERIFY(qt.load("qtbase_zh_CN", QLibraryInfo::path(QLibraryInfo::TranslationsPath)));
      qApp->installTranslator(&qt);
    }
    const auto cleanup = qScopeGuard([&] {
      qApp->removeTranslator(&app);
      qApp->removeTranslator(&qt);
    });
    QTemporaryDir dir(qEnvironmentVariable("DESKFLOW_TEST_ROOT") + "/startup-XXXXXX");
    QVERIFY(dir.isValid());
    auto system = std::make_shared<Fake>();
    system->state.conflict = conflict;
    WindowsStartupDialog dialog(system, dir.path() + "/backup.json");
    dialog.resize(width, 480);
    dialog.show();
    QTest::qWait(150);
    QCOMPARE(dialog.width(), width);
    for (auto *button : dialog.findChildren<QPushButton *>())
      if (button->isVisible())
        QVERIFY(dialog.rect().contains(QRect(button->mapTo(&dialog, QPoint()), button->size())));
    const auto output = qEnvironmentVariable("DESKFLOW_UI_TEST_SCREENSHOTS");
    if (!output.isEmpty())
      QVERIFY(dialog.grab().save(output + QString("/windows-startup-%1-%2-%3.png").arg(zh).arg(width).arg(conflict)));
    QCOMPARE(system->writes, 0);
    dialog.reject();
  }
  void declinedConfirmationNeverWrites()
  {
    QTemporaryDir dir(qEnvironmentVariable("DESKFLOW_TEST_ROOT") + "/startup-XXXXXX");
    QVERIFY(dir.isValid());
    auto system = std::make_shared<Fake>();
    WindowsStartupDialog dialog(system, dir.path() + "/backup.json");
    dialog.show();
    auto *apply = dialog.findChild<QPushButton *>("windowsStartupApply");
    QTRY_VERIFY(apply->isEnabled());
    dialog.findChild<QCheckBox *>("windowsStartupLogin")->setChecked(true);
    answer(QMessageBox::Cancel);
    apply->click();
    QCOMPARE(system->writes, 0);
    QVERIFY(!QFile::exists(dir.path() + "/backup.json"));
    dialog.reject();
  }

  void openingAndCancellingNeverWrites()
  {
    QTemporaryDir dir(qEnvironmentVariable("DESKFLOW_TEST_ROOT") + "/startup-XXXXXX");
    QVERIFY(dir.isValid());
    auto system = std::make_shared<Fake>();
    WindowsStartupDialog dialog(system, dir.path() + "/backup.json");
    dialog.show();
    auto *apply = dialog.findChild<QPushButton *>("windowsStartupApply");
    QTRY_VERIFY(apply->isEnabled());
    dialog.findChild<QCheckBox *>("windowsStartupLogin")->setChecked(true);
    dialog.reject();
    QCOMPARE(system->writes, 0);
    QVERIFY(!QFile::exists(dir.path() + "/backup.json"));
  }
  void applyAndPersistentRestore()
  {
    QTemporaryDir dir(qEnvironmentVariable("DESKFLOW_TEST_ROOT") + "/startup-XXXXXX");
    QVERIFY(dir.isValid());
    auto system = std::make_shared<Fake>();
    WindowsStartupDialog dialog(system, dir.path() + "/backup.json");
    dialog.show();
    auto *apply = dialog.findChild<QPushButton *>("windowsStartupApply");
    QTRY_VERIFY(apply->isEnabled());
    dialog.findChild<QCheckBox *>("windowsStartupLogin")->setChecked(true);
    dialog.findChild<QCheckBox *>("windowsStartupRecovery")->setChecked(true);
    answer(QMessageBox::Yes);
    apply->click();
    QTRY_VERIFY(apply->isEnabled());
    QCOMPARE(system->writes, 1);
    QVERIFY(system->state.exists);
    QVERIFY(QFile::exists(dir.path() + "/backup.json"));
    auto *restore = dialog.findChild<QPushButton *>("windowsStartupRestore");
    QVERIFY(restore->isEnabled());
    answer(QMessageBox::Yes);
    restore->click();
    QTRY_VERIFY(apply->isEnabled());
    QCOMPARE(system->writes, 2);
    QVERIFY(!system->state.exists);
    dialog.reject();
  }
  void failureIsVisibleAndNotReportedEnabled()
  {
    QTemporaryDir dir(qEnvironmentVariable("DESKFLOW_TEST_ROOT") + "/startup-XXXXXX");
    QVERIFY(dir.isValid());
    auto system = std::make_shared<Fake>();
    system->fail = true;
    WindowsStartupDialog dialog(system, dir.path() + "/backup.json");
    dialog.show();
    auto *apply = dialog.findChild<QPushButton *>("windowsStartupApply");
    QTRY_VERIFY(apply->isEnabled());
    dialog.findChild<QCheckBox *>("windowsStartupLogin")->setChecked(true);
    answer(QMessageBox::Yes);
    apply->click();
    QTRY_VERIFY(apply->isEnabled());
    QVERIFY(!system->state.exists);
    QVERIFY(dialog.findChild<QLabel *>("windowsStartupStatus")->text().contains("not confirmed"));
    QVERIFY(!dialog.findChild<QCheckBox *>("windowsStartupLogin")->isChecked());
    dialog.reject();
  }
  void conflictBlocksApply()
  {
    QTemporaryDir dir(qEnvironmentVariable("DESKFLOW_TEST_ROOT") + "/startup-XXXXXX");
    QVERIFY(dir.isValid());
    auto system = std::make_shared<Fake>();
    system->state.conflict = true;
    WindowsStartupDialog dialog(system, dir.path() + "/backup.json");
    dialog.show();
    auto *status = dialog.findChild<QLabel *>("windowsStartupStatus");
    QTRY_VERIFY(status->text().contains("existing or unrecognized"));
    QVERIFY(!dialog.findChild<QPushButton *>("windowsStartupApply")->isEnabled());
    QCOMPARE(system->writes, 0);
    dialog.reject();
  }
};
QTEST_MAIN(WindowsStartupDialogTests)
#include "WindowsStartupDialogTests.moc"
