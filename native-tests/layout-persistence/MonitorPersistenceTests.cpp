// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/Settings.h"
#include "gui/config/ServerConfig.h"
#include "gui/dialogs/DesktopLayoutEditor.h"
#include "gui/dialogs/ServerConfigDialog.h"
#include "gui/widgets/SettingsDialogButtonBox.h"
#include <QApplication>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
#include <QTranslator>
#include <cstdio>
using namespace deskflow::display;
static Topologies fixtures()
{
  return {
      {"Windows", Topology{{{"w", {0, 0, 1920, 1080}, {0, 0, 3840, 2160}, {3840, 2160}, {}}}}},
      {"Mac", Topology{
                  {{"m-left", {-1600, 0, 1600, 900}, {-1600, 0, 1600, 900}, {3200, 1800}, {}},
                   {"m-right", {0, 0, 1920, 1080}, {0, 0, 1920, 1080}, {3840, 2160}, {}}}
              }}
  };
}
static QByteArray bytes(const QString &path)
{
  QFile f(path);
  return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}
static int writeProfile(const QString &mode)
{
  Settings::setValue(Settings::Core::ComputerName, "Windows");
  Settings::setValue(Settings::Security::TlsEnabled, true);
  Settings::setValue(Settings::Security::CheckPeers, true);
  auto &p = Settings::proxy();
  p.beginGroup("internalConfig");
  p.beginWriteArray("screens");
  for (int i = 0; i < 15; ++i) {
    p.setArrayIndex(i);
    p.setValue("name", i == 7 ? "Windows" : "");
  }
  p.endArray();
  p.endGroup();
  p.sync();
  ServerConfig original;
  original.addClient("Mac");
  original.commit();
  Settings::setValue(Settings::Server::MonitorTopologies, encodeTopologies(fixtures()));
  Settings::save(false);
  const auto before = bytes(Settings::settingsFile());
  ServerConfig config;
  ServerConfigDialog dialog(nullptr, config);
  dialog.show();
  QApplication::processEvents();
  const auto expected = qEnvironmentVariable("EXPECTED_DISPLAY_DPR").toDouble();
  if (expected > 0 && qAbs(dialog.devicePixelRatioF() - expected) > 0.01)
    return 16;
  auto *toggle = dialog.findChild<QPushButton *>("toggleMonitorLayout");
  if (!toggle || !toggle->isEnabled())
    return 10;
  QTest::mouseClick(toggle, Qt::LeftButton);
  auto *editor = dialog.findChild<DesktopLayoutEditor *>("serverArrangement");
  if (editor->desktops().size() != 3 || editor->findChild<QDoubleSpinBox *>("layoutWidth")->isEnabled())
    return 11;
  const auto key = deskflow::gui::MonitorLayoutDraft::token({"Mac", "m-left", {}});
  auto *canvas = editor->findChild<LayoutCanvas *>("layoutCanvas");
  QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, canvas->desktopRect(key.toStdString()).center().toPoint());
  QTest::keyClick(canvas, Qt::Key_Down);
  QApplication::processEvents();
  if (bytes(Settings::settingsFile()) != before)
    return 12;
  auto *buttons = dialog.findChild<SettingsDialogButtonBox *>()->findChild<QDialogButtonBox *>();
  if (mode == "cancel") {
    QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel), Qt::LeftButton);
    return bytes(Settings::settingsFile()) == before && dialog.result() == QDialog::Rejected ? 0 : 13;
  }
  if (!buttons->button(QDialogButtonBox::Save)->isEnabled())
    return 14;
  const auto screenshots = qEnvironmentVariable("LAYOUT_TEST_SCREENSHOTS");
  if (!screenshots.isEmpty())
    dialog.grab().save(screenshots + "/individual-displays-before-save.png");
  QTest::mouseClick(buttons->button(QDialogButtonBox::Save), Qt::LeftButton);
  // The only persistence operation under test is the real Save click above.
  return dialog.result() == QDialog::Accepted && bytes(Settings::settingsFile()) != before ? 0 : 15;
}
static int readProfile()
{
  (void)Settings::instance();
  ServerConfig config;
  const auto raw = Settings::value(Settings::Server::MonitorLayout).toByteArray();
  const auto layout = Layout::decode(raw);
  double y = -999;
  QPoint mapped;
  if (layout) {
    for (const auto &p : layout->placements)
      if (p.device == "Mac" && p.monitor == "m-left")
        y = p.position.y();
    if (const auto c = layout->cross(fixtures(), "Windows", "w", Side::Left, {0, 900}))
      mapped = c->input;
  }
  QJsonObject result{
      {"saved", config.hasMonitorLayout()},
      {"count", layout ? layout->placements.size() : 0},
      {"leftY", y},
      {"mappedX", mapped.x()},
      {"mappedY", mapped.y()},
      {"tls", Settings::value(Settings::Security::TlsEnabled).toBool()},
      {"checks", Settings::value(Settings::Security::CheckPeers).toBool()}
  };
  if (config.hasMonitorLayout())
    result["draftCount"] = int(config.monitorLayoutDraft().desktops().size());
  const auto encoded = QJsonDocument(result).toJson(QJsonDocument::Compact);
  std::fwrite(encoded.constData(), 1, encoded.size(), stdout);
  return 0;
}
class MonitorPersistenceTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void saveAndCancelAcrossProcesses_data()
  {
    QTest::addColumn<QString>("mode");
    QTest::newRow("save-exit-reopen") << QString("save");
    QTest::newRow("cancel-exit-reopen") << QString("cancel");
  }
  void saveAndCancelAcrossProcesses()
  {
    QFETCH(QString, mode);
    QTemporaryDir root;
    QVERIFY(root.isValid());
    QProcess writer;
    writer.start(QCoreApplication::applicationFilePath(), {"--write", root.path(), mode});
    QVERIFY(writer.waitForFinished(15000));
    QVERIFY2(
        writer.exitStatus() == QProcess::NormalExit && writer.exitCode() == 0,
        qPrintable(QString::number(writer.exitCode()) + writer.readAllStandardError())
    );
    QProcess reader;
    reader.start(QCoreApplication::applicationFilePath(), {"--read", root.path()});
    QVERIFY(reader.waitForFinished(10000));
    QVERIFY2(reader.exitCode() == 0, reader.readAllStandardError().constData());
    const auto result = QJsonDocument::fromJson(reader.readAllStandardOutput()).object();
    QCOMPARE(result["saved"].toBool(), mode == "save");
    QVERIFY(result["tls"].toBool());
    QVERIFY(result["checks"].toBool());
    if (mode == "save") {
      QCOMPARE(result["count"].toInt(), 3);
      QCOMPARE(result["draftCount"].toInt(), 3);
      QCOMPARE(result["leftY"].toDouble(), 10.0);
      QCOMPARE(result["mappedX"].toInt(), 1918);
      QCOMPARE(result["mappedY"].toInt(), 450);
    }
  }
  void changedTopologyKeepsSavedDraftButDoesNotRoute()
  {
    auto t = fixtures();
    deskflow::gui::MonitorLayoutDraft draft(t, {"Mac", "Windows"}, {});
    QVERIFY(draft.error.isEmpty());
    const auto saved = draft.layout.encode();
    t["Mac"].monitors[1].pixels = {5120, 2880};
    deskflow::gui::MonitorLayoutDraft changed(t, {"Mac", "Windows"}, saved);
    QVERIFY(!changed.error.isEmpty());
    QCOMPARE(changed.layout.encode(), saved);
    QVERIFY(!changed.layout.cross(t, "Windows", "w", Side::Left, {0, 100}));
    t.remove("Mac");
    deskflow::gui::MonitorLayoutDraft unavailable(t, {"Mac", "Windows"}, saved);
    QVERIFY(!unavailable.error.isEmpty());
  }
};
int main(int argc, char **argv)
{
  const bool child = argc >= 3 && (QByteArray(argv[1]) == "--write" || QByteArray(argv[1]) == "--read");
  if (child) {
    const QString root = QString::fromLocal8Bit(argv[2]);
    if (!QDir::isAbsolutePath(root) || !QDir(root).exists())
      return 2;
    qputenv("XDG_CONFIG_HOME", root.toUtf8());
    qputenv("XDG_STATE_HOME", (root + "/state").toUtf8());
    QDir().mkpath(root + "/Deskflow");
    QDir().mkpath(root + "/state");
    if (QByteArray(argv[1]) == "--write") {
      if (argc < 4)
        return 2;
      QFile f(root + "/Deskflow/Deskflow.conf");
      if (!f.open(QIODevice::WriteOnly))
        return 3;
    }
  }
  QApplication app(argc, argv);
  app.setStyle("Fusion");
  QTranslator qtTranslator;
  const auto qtTranslation = qEnvironmentVariable("DISPLAY_TEST_QT_TRANSLATION");
  if (!qtTranslation.isEmpty() && qtTranslator.load(qtTranslation))
    app.installTranslator(&qtTranslator);
  QTranslator translator;
  const auto translation = qEnvironmentVariable("DISPLAY_TEST_TRANSLATION");
  if (!translation.isEmpty() && translator.load(translation))
    app.installTranslator(&translator);
  if (child)
    return QByteArray(argv[1]) == "--write" ? writeProfile(QString::fromLocal8Bit(argv[3])) : readProfile();
  MonitorPersistenceTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "MonitorPersistenceTests.moc"
