// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/Settings.h"
#include "gui/IdentityBindings.h"
#include "gui/MainWindow.h"
#include "gui/dialogs/PairingReviewDialog.h"
#include "gui/dialogs/SettingsDialog.h"
#include "gui/widgets/ConnectionHome.h"
#include <QApplication>
#include <QDialogButtonBox>
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <cstdio>

using Bindings = deskflow::gui::IdentityBindings;
using Home = deskflow::gui::ConnectionHome;
class FirstSetupFlowTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void cancelledPairingCanBeReviewedAfterExplicitRetry_data()
  {
    QTest::addColumn<int>("dismiss");
    QTest::newRow("cancel-button") << 0;
    QTest::newRow("escape") << 1;
    QTest::newRow("window-close") << 2;
  }
  void cancelledPairingCanBeReviewedAfterExplicitRetry()
  {
    QFETCH(int, dismiss);
    Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::Client);
    Settings::setValue(Settings::Client::RemoteHost, "192.168.50.20");
    Settings::setValue(Settings::Core::Port, 34800);
    Settings::setValue(Settings::Gui::AutoStartCore, false);
    Settings::setValue("gui/identityBindings", Bindings{}.encode());
    MainWindow window;
    window.show();
    QCOMPARE(window.m_coreProcess.mode(), Settings::CoreMode::Client);
    const auto saved = window.m_identityBindings.encode();
    int shown = 0;
    auto review = [&] {
      QTimer::singleShot(0, &window, [&] {
        auto *dialog = qobject_cast<PairingReviewDialog *>(QApplication::activeModalWidget());
        if (!dialog) {
          if (auto *other = qobject_cast<QDialog *>(QApplication::activeModalWidget()))
            other->reject();
          return;
        }
        ++shown;
        if (dismiss == 0)
          dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Cancel)->click();
        else if (dismiss == 1)
          QTest::keyClick(dialog, Qt::Key_Escape);
        else
          dialog->close();
      });
      window.handlePeerFingerprint(QString(64, 'c'));
    };
    review();
    QCOMPARE(shown, 1);
    // Duplicate notifications from the cancelled attempt must stay quiet.
    window.handlePeerFingerprint(QString(64, 'c'));
    QCOMPARE(shown, 1);
    window.labUserStart();
    review();
    QCOMPARE(shown, 2);
    QCOMPARE(window.m_identityBindings.encode(), saved);
    QVERIFY(!QFile::exists(Settings::tlsTrustedServersDb()));
    QVERIFY(!QFile::exists(Settings::tlsTrustedClientsDb()));
    QVERIFY(!window.m_coreProcess.isStarted());
  }
  void freshWindowUsesExistingIdentityPageAndCancelWritesNoIdentity()
  {
    Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::None);
    Settings::setValue(Settings::Client::RemoteHost, QString());
    Settings::setValue(Settings::Gui::AutoStartCore, false);
    Settings::setValue("gui/identityBindings", Bindings{}.encode());
    MainWindow window;
    window.show();
    auto *home = window.findChild<Home *>();
    QVERIFY(home);
    QVERIFY(!home->state().localName.isEmpty());
    QVERIFY(!home->state().identityReady);
    home->findChild<QPushButton *>("homeReceive")->click();
    bool visited = false;
    QTimer::singleShot(0, &window, [&] {
      auto *dialog = qobject_cast<SettingsDialog *>(QApplication::activeModalWidget());
      if (!dialog) {
        QFAIL("Expected the existing identity settings dialog");
        return;
      }
      const auto close = qScopeGuard([dialog] {
        if (dialog->isVisible())
          dialog->reject();
      });
      visited = true;
      QVERIFY(dialog->findChild<QLabel *>("identityHeading")->isVisible());
      dialog->reject();
    });
    home->findChild<QPushButton *>("homePrimary")->click();
    QVERIFY(visited);
    QVERIFY(!home->state().identityReady);
    QVERIFY(!window.m_coreProcess.connectionRequested());
    QDirIterator files(Settings::settingsPath(), QDir::Files, QDirIterator::Subdirectories);
    while (files.hasNext()) {
      const auto path = files.next();
      QVERIFY2(
          !path.endsWith(".pem") && !path.contains("TrustedServers") && !path.contains("TrustedClients"),
          qPrintable(path)
      );
    }
  }
  void savedNameAndExpectedPinSurviveIndependentProcessReopen()
  {
    QTemporaryDir profile(qEnvironmentVariable("DESKFLOW_TEST_ROOT", QDir::tempPath()) + "/first-setup-XXXXXX");
    QVERIFY(profile.isValid());
    auto child = [&](const QString &mode) {
      QProcess process;
      process.start(QCoreApplication::applicationFilePath(), {"--profile", profile.path(), mode});
      if (!process.waitForFinished(15000) || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return QJsonObject{};
      return QJsonDocument::fromJson(process.readAllStandardOutput()).object();
    };
    const auto first = child("seed");
    QVERIFY(!first.isEmpty());
    QCOMPARE(first["label"].toString(), QString("Work-PC"));
    const auto reopened = child("read");
    QVERIFY(!reopened.isEmpty());
    QCOMPARE(reopened["label"], first["label"]);
    QCOMPARE(reopened["pin"].toString(), QString(64, 'a'));
    QVERIFY(reopened["paused_on_open"].toBool());
    QVERIFY(!reopened["core_started"].toBool());
  }

public:
  static int runChild(const QString &mode)
  {
    const QString fp(64, 'a');
    if (mode == "seed") {
      Bindings model;
      auto g = model.begin({"192.168.50.20", 34800});
      model.observe(g, fp);
      if (!model.confirm(g, "192.168.50.20", true) || !model.authenticated(g, fp, true, true) ||
          !model.updateDefaultLabel(fp, "Work-PC"))
        return 4;
      Settings::setValue("gui/identityBindings", model.encode());
      Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::Client);
      Settings::setValue(Settings::Core::Port, 34800);
      Settings::setValue(Settings::Client::RemoteHost, "192.168.50.20");
      Settings::setValue(Settings::Gui::AutoStartCore, false);
      Settings::save(false);
    }
    MainWindow window;
    auto *home = window.findChild<Home *>();
    const bool paused = !window.m_coreProcess.connectionRequested();
    // Lab build disables core execution; call the actual controller selection
    // path to prove it restores the pin, without certificates or network input.
    window.labUserStart();
    const QJsonObject result{
        {"label", home->state().peerName},
        {"pin", window.m_coreProcess.expectedPeerFingerprint()},
        {"paused_on_open", paused},
        {"core_started", window.m_coreProcess.isStarted()}
    };
    const auto bytes = QJsonDocument(result).toJson(QJsonDocument::Compact);
    std::fwrite(bytes.constData(), 1, bytes.size(), stdout);
    std::fflush(stdout);
    return 0;
  }
};
int main(int argc, char **argv)
{
  QTemporaryDir isolated(qEnvironmentVariable("DESKFLOW_TEST_ROOT", QDir::tempPath()) + "/setup-main-XXXXXX");
  const bool child = argc == 4 && QByteArray(argv[1]) == "--profile";
  const QString root = child ? QString::fromLocal8Bit(argv[2]) : isolated.path();
  if (!QDir::isAbsolutePath(root) || !QDir(root).exists())
    return 2;
  qputenv("XDG_CONFIG_HOME", (root + "/config").toUtf8());
  qputenv("XDG_STATE_HOME", (root + "/state").toUtf8());
  QDir().mkpath(root + "/config/DeskflowLab");
  QDir().mkpath(root + "/state");
  QApplication app(argc, argv);
  app.setStyle("Fusion");
  if (child)
    return FirstSetupFlowTests::runChild(QString::fromLocal8Bit(argv[3]));
  FirstSetupFlowTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "FirstSetupFlowTests.moc"
