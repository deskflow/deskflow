// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/Settings.h"
#include "gui/config/ServerConfig.h"
#include "gui/dialogs/DesktopLayoutEditor.h"
#include "gui/dialogs/ServerConfigDialog.h"
#include "gui/widgets/SettingsDialogButtonBox.h"
#include <QApplication>
#include <QDialogButtonBox>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
#include <QTextStream>
#include <QTimer>
#include <cstdio>

static QByteArray bytes(const QString &path)
{
  QFile f(path);
  return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}
static int writer(const QString &mode)
{
  Settings::setValue(Settings::Core::ComputerName, "Mac");
  Settings::setValue(Settings::Client::LanguageSync, false);
  Settings::setValue(Settings::Security::TlsEnabled, true);
  Settings::setValue(Settings::Security::CheckPeers, true);
  Settings::setValue(Settings::Computer::Aliases.arg("Mac"), QStringList{"mac-alias"});
  auto &proxy = Settings::proxy();
  proxy.beginGroup("internalConfig");
  proxy.beginWriteArray("screens");
  for (int i = 0; i < 15; ++i) {
    proxy.setArrayIndex(i);
    proxy.setValue("name", i == 7 ? "Mac" : "");
  }
  proxy.endArray();
  proxy.endGroup();
  proxy.sync();
  // Seed a complete current-format profile, including per-computer defaults.
  // This is fixture setup, before the Save interaction under test.
  ServerConfig initial;
  initial.commit();
  Settings::save(false);
  const auto original = bytes(Settings::settingsFile());
  ServerConfig config;
  ServerConfigDialog dialog(nullptr, config);
  dialog.show();
  if (!dialog.addClient("Windows"))
    return 10;
  auto *editor = dialog.findChild<DesktopLayoutEditor *>("serverArrangement");
  auto *buttons = dialog.findChild<SettingsDialogButtonBox *>()->findChild<QDialogButtonBox *>();
  editor->moveDesktop("Windows", 4320, 1080.5, false);
  QApplication::processEvents();
  // Drafting must not write, including after the event loop runs.
  if (bytes(Settings::settingsFile()) != original)
    return 11;
  if (mode == "invalid")
    editor->moveDesktop("Windows", 2880, 900, false);
  if (mode == "cancel" || mode == "invalid") {
    if (mode == "invalid" && buttons->button(QDialogButtonBox::Save)->isEnabled())
      return 12;
    QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel), Qt::LeftButton);
    return dialog.result() == QDialog::Rejected && bytes(Settings::settingsFile()) == original ? 0 : 13;
  }
#ifndef Q_OS_WIN
  if (mode == "fail" || mode == "retry") {
    const auto directory = Settings::settingsPath();
    const auto permissions = QFile::permissions(directory);
    if (!QFile::setPermissions(directory, QFile::ReadOwner | QFile::ExeOwner))
      return 14;
    bool warned = false;
    QTimer dismiss;
    dismiss.setInterval(1);
    QObject::connect(&dismiss, &QTimer::timeout, &dialog, [&] {
      if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
        warned = true;
        box->accept();
      }
    });
    dismiss.start();
    QTest::mouseClick(buttons->button(QDialogButtonBox::Save), Qt::LeftButton);
    dismiss.stop();
    QFile::setPermissions(directory, permissions);
    if (!warned || !dialog.isVisible() || config.computerExists("Windows") ||
        bytes(Settings::settingsFile()) != original)
      return 15;
    if (mode == "fail") {
      QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel), Qt::LeftButton);
      QApplication::processEvents();
      return bytes(Settings::settingsFile()) == original ? 0 : 16;
    }
  }
#endif
  const auto image = qEnvironmentVariable("LAYOUT_TEST_SCREENSHOTS");
  if (!image.isEmpty())
    dialog.grab().save(image + "/layout-before-save.png");
  QTest::mouseClick(buttons->button(QDialogButtonBox::Save), Qt::LeftButton);
  if (dialog.result() != QDialog::Accepted)
    return 17;
  if (bytes(Settings::settingsFile()) == original)
    return 18;
  // No config.commit(), Settings::save(), manual sync or shutdown hook here.
  return 0;
}
static int reader()
{
  // The real application initializes Settings before constructing MainWindow.
  (void)Settings::instance();
  ServerConfig config;
  QString exported;
  QTextStream stream(&exported);
  stream << config;
  QJsonArray names;
  for (const auto &computer : std::as_const(config).computers())
    if (!computer.isNull())
      names.append(computer.name());
  const QJsonObject result{
      {"names", names},
      {"layout", QString::fromStdString(config.hasDesktopLayout() ? "custom" : "grid")},
      {"desktops", QString::fromUtf8(
                       QJsonDocument::fromJson(Settings::value(Settings::Server::DesktopLayout).toString().toUtf8())
                           .toJson(QJsonDocument::Compact)
                   )},
      {"export", exported},
      {"alias", Settings::value(Settings::Computer::Aliases.arg("Mac")).toStringList().join(",")},
      {"tls", Settings::value(Settings::Security::TlsEnabled).toBool()},
      {"peerChecks", Settings::value(Settings::Security::CheckPeers).toBool()},
      {"languageSync", Settings::value(Settings::Client::LanguageSync).toBool()}
  };
  const auto data = QJsonDocument(result).toJson(QJsonDocument::Compact);
  std::fwrite(data.constData(), 1, data.size(), stdout);
  return 0;
}
class PersistenceTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void saveCancelAndFailureAcrossFreshProcesses_data()
  {
    QTest::addColumn<QString>("mode");
    QTest::addColumn<bool>("saved");
    QTest::newRow("save-normal-exit-reopen") << QString("save") << true;
    QTest::newRow("cancel-normal-exit-reopen") << QString("cancel") << false;
    QTest::newRow("invalid-layout-no-write") << QString("invalid") << false;
#ifndef Q_OS_WIN
    QTest::newRow("atomic-write-fails-cancel") << QString("fail") << false;
    QTest::newRow("atomic-write-fails-retry") << QString("retry") << true;
#endif
  }
  void saveCancelAndFailureAcrossFreshProcesses()
  {
    QFETCH(QString, mode);
    QFETCH(bool, saved);
    QTemporaryDir profile;
    QVERIFY(profile.isValid());
    QProcess write;
    write.start(QCoreApplication::applicationFilePath(), {"--write", profile.path(), mode});
    QVERIFY(write.waitForFinished(20000));
    QCOMPARE(write.exitStatus(), QProcess::NormalExit);
    QVERIFY2(write.exitCode() == 0, write.readAllStandardError().constData());
    QProcess read;
    read.start(QCoreApplication::applicationFilePath(), {"--read", profile.path()});
    QVERIFY(read.waitForFinished(10000));
    QVERIFY2(
        read.exitStatus() == QProcess::NormalExit && read.exitCode() == 0, read.readAllStandardError().constData()
    );
    const auto result = QJsonDocument::fromJson(read.readAllStandardOutput()).object();
    QVERIFY(!result.isEmpty());
    QCOMPARE(result["names"].toArray().size(), saved ? 2 : 1);
    QCOMPARE(result["layout"].toString(), saved ? "custom" : "grid");
    QCOMPARE(result["alias"].toString(), QString("mac-alias"));
    QVERIFY(result["tls"].toBool());
    QVERIFY(result["peerChecks"].toBool());
    QVERIFY(!result["languageSync"].toBool());
    if (saved) {
      const auto layout =
          QJsonDocument::fromJson(result["desktops"].toString().toUtf8()).object()["desktops"].toArray();
      QCOMPARE(layout.size(), 2);
      QJsonObject windows;
      for (const auto &desktop : layout)
        if (desktop.toObject()["name"].toString() == "Windows")
          windows = desktop.toObject();
      QVERIFY(!windows.isEmpty());
      QCOMPARE(windows["x"].toDouble(), 4320.0);
      QCOMPARE(windows["y"].toDouble(), 1080.5);
      QCOMPARE(windows["width"].toDouble(), 1440.0);
      QCOMPARE(windows["height"].toDouble(), 900.0);
      QVERIFY(result["export"].toString().contains("right("));
      QVERIFY(result["export"].toString().contains("left("));
    }
  }
};
int main(int argc, char **argv)
{
  const bool child = argc >= 3 && (QByteArray(argv[1]) == "--write" || QByteArray(argv[1]) == "--read");
  if (child && QByteArray(argv[1]) == "--write" && argc < 4)
    return 2;
  if (child) {
    const QString profile = QString::fromLocal8Bit(argv[2]);
    if (!QDir::isAbsolutePath(profile) || !QDir(profile).exists())
      return 2;
    qputenv("XDG_CONFIG_HOME", profile.toUtf8());
    qputenv("XDG_STATE_HOME", (profile + "/state").toUtf8());
    QDir().mkpath(profile + "/Deskflow");
    QDir().mkpath(profile + "/state");
    if (QByteArray(argv[1]) == "--write") {
      QFile f(profile + "/Deskflow/Deskflow.conf");
      if (!f.open(QIODevice::WriteOnly))
        return 3;
    }
  }
  QApplication app(argc, argv);
  app.setStyle("Fusion");
  if (child)
    return QByteArray(argv[1]) == "--write" ? writer(QString::fromLocal8Bit(argv[3])) : reader();
  PersistenceTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "LayoutPersistenceProcessTests.moc"
