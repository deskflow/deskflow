// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
#include "../WindowsOffscreenFontSupport.h"
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/I18N.h"
#include "common/Settings.h"
#include "gui/ExperienceStyle.h"
#include "gui/MainWindow.h"
#include "gui/dialogs/AboutDialog.h"
#include "gui/dialogs/ActionDialog.h"
#include "gui/dialogs/AutomaticRecoveryDialog.h"
#include "gui/dialogs/ClientConfigDialog.h"
#include "gui/dialogs/ComputerSettingsDialog.h"
#include "gui/dialogs/DesktopLayoutEditor.h"
#include "gui/dialogs/HotkeyDialog.h"
#include "gui/dialogs/ServerConfigDialog.h"
#include "gui/dialogs/SettingsDialog.h"
#include "gui/dialogs/StartupRecoveryDialog.h"
#include "gui/dialogs/TrustedDevicesDialog.h"
#include "gui/ipc/CoreIpcClient.h"
#include "gui/widgets/ConnectionDiagnostics.h"
#include "gui/widgets/ConnectionHome.h"
#include "gui/widgets/DesktopWorkspaceView.h"
#include "gui/widgets/KeySequenceWidget.h"
#include "gui/widgets/LogDock.h"
#include "gui/widgets/OptionControl.h"
#include "gui/widgets/SearchWidget.h"
#include "gui/widgets/SettingsDialogButtonBox.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGroupBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLayout>
#include <QLibraryInfo>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QScopeGuard>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QStyleOptionButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <QTranslator>
#include <QUuid>
#include <cstdio>
using namespace deskflow::display;
using namespace deskflow::gui;
namespace {
class MemoryCoreIpc final : public deskflow::gui::ipc::CoreIpcClient
{
public:
  using CoreIpcClient::CoreIpcClient;
  QByteArray last;
  bool sendWorkspaceApply(const QByteArray &bytes) override
  {
    last = bytes;
    return true;
  }
  void sendStop() override
  {
  }
};
Topology localScreens()
{
  return {
      {{"a", {0, 0, 2560, 1440}, {0, 0, 3840, 2160}, {3840, 2160}, {}},
       {"b", {3840, 132, 1463, 914}, {3840, 132, 2560, 1600}, {2560, 1600}, {}}}
  };
}
Topology remoteScreen()
{
  return {{{"r", {0, 0, 1920, 1080}, {0, 0, 1920, 1080}, {3840, 2160}, {}}}};
}
QByteArray bytes(const QString &path)
{
  QFile f(path);
  return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}
} // namespace
class DesktopProductFlowTests : public QObject
{
  Q_OBJECT
  QString m_root = qEnvironmentVariable("DESKFLOW_TEST_ROOT");
  static void tabsForIdentity(SettingsDialog *dialog)
  {
    dialog->findChild<QTabWidget *>("tabWidget")->setCurrentIndex(3);
    auto *details = dialog->findChild<QToolButton *>("connectionDetailsToggle");
    if (!details->isChecked())
      details->click();
    QApplication::processEvents();
  }
  void seed(MainWindow &window, bool old = false)
  {
    window.m_serverConfig.addClient("Receiver");
    window.m_localWorkspaceTopology = localScreens();
    WorkspaceSnapshot snapshot;
    snapshot.devices = {"Controller", "Receiver"};
    snapshot.onlineDevices = snapshot.devices;
    snapshot.topologyCapable = old ? QStringList{"Controller"} : snapshot.devices;
    snapshot.topologies["Controller"] = localScreens();
    snapshot.session = QString(32, 'a');
    snapshot.generation = 1;
    snapshot.identityReferences = {{"Controller", QString(64, 'a')}, {"Receiver", QString(64, 'b')}};
    if (!old)
      snapshot.topologies["Receiver"] = remoteScreen();
    window.m_workspaceSnapshot = snapshot;
    window.m_workspaceLive = true;
    Settings::setValue(Settings::Server::MonitorTopologies, encodeTopologies(snapshot.topologies));
    window.refreshDesktopWorkspace();
    window.resize(860, 820);
    window.show();
    QApplication::processEvents();
  }
  void capture(MainWindow &window, const QString &name)
  {
    const auto output = qEnvironmentVariable("DESKFLOW_UI_TEST_SCREENSHOTS");
    if (output.isEmpty())
      return;
    QDir().mkpath(output);
    QApplication::processEvents();
    QVERIFY(window.grab().save(output + '/' + name + ".png"));
  }
private Q_SLOTS:

  void stoppedCoreDoesNotPresentCachedAcknowledgementAsLive()
  {
    Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::Server);
    MainWindow window;
    seed(window);
    const auto saved = bytes(Settings::settingsFile());
    window.m_workspaceSaveStatus = "The current sharing core is using the last confirmed arrangement.";
    window.m_workspaceLive = false;
    window.refreshDesktopWorkspace();
    auto *routing = window.m_desktopWorkspace->findChild<QLabel *>("workspaceActiveRouting");
    QVERIFY(routing->text().contains("Sharing is paused"));
    QVERIFY(!routing->text().contains("is using"));
    QCOMPARE(bytes(Settings::settingsFile()), saved);
    QVERIFY(!window.m_coreProcess.isStarted());
    capture(window, "stopped-core-retains-layout-without-live-claim");
  }

  void foundationSettingsOptionsAndPopupsCancelCleanly()
  {
    QTranslator translator, qtTranslator;
    QVERIFY(translator.load(":/translations/deskflow_zh_CN.qm"));
    qApp->installTranslator(&translator);
    QVERIFY(qtTranslator.load("qtbase_zh_CN", QLibraryInfo::path(QLibraryInfo::TranslationsPath)));
    qApp->installTranslator(&qtTranslator);
    const auto restore = qScopeGuard([&] {
      qApp->removeTranslator(&translator);
      qApp->removeTranslator(&qtTranslator);
    });
    MainWindow window;
    seed(window);
    const auto saved = bytes(Settings::settingsFile());
    bool visited = false;
    QTimer::singleShot(0, &window, [&] {
      auto *active = qobject_cast<SettingsDialog *>(QApplication::activeModalWidget());
      QVERIFY(active);
      visited = true;
      const auto close = qScopeGuard([active] {
        if (active->isVisible())
          active->reject();
      });
      auto &dialog = *active;
      dialog.resize(760, 600);
      dialog.activateWindow();
      auto *tabs = dialog.findChild<QTabWidget *>("tabWidget");
      QVERIFY(tabs);
      const auto output = qEnvironmentVariable("DESKFLOW_UI_TEST_SCREENSHOTS");
      int popups = 0;
      for (int page : {0, 1, 2, 3, 4}) {
        tabs->setCurrentIndex(page);
        if (page == 3)
          tabsForIdentity(&dialog);
        QTest::qWait(20);
        if (!output.isEmpty())
          QVERIFY(dialog.grab().save(output + QString("/foundation-settings-%1.png").arg(page)));
        for (auto *button : dialog.findChild<SettingsDialogButtonBox *>()->findChildren<QPushButton *>())
          if (button->isVisible())
            QVERIFY(button->visibleRegion().contains(button->rect()));
        for (auto *combo : tabs->currentWidget()->findChildren<QComboBox *>()) {
          if (!combo->isVisible() || !combo->isEnabled() || combo->count() < 2)
            continue;
          const int original = combo->currentIndex();
          combo->setFocus();
          combo->showPopup();
          QTest::qWait(20);
          QVERIFY(combo->view()->isVisible());
          ++popups;
          if (!output.isEmpty())
            QVERIFY(combo->view()->window()->grab().save(output + "/foundation-popup-" + combo->objectName() + ".png"));
          QTest::keyClick(combo->view(), Qt::Key_Down);
          QTest::keyClick(combo->view(), Qt::Key_Escape);
          QVERIFY(!combo->view()->isVisible());
          QCOMPARE(combo->currentIndex(), original);
        }
      }
      QVERIFY(popups >= 2);
      dialog.reject();
    });
    window.findChild<QPushButton *>("homeSettings")->click();
    QVERIFY(visited);
    QCOMPARE(bytes(Settings::settingsFile()), saved);
    const auto output = qEnvironmentVariable("DESKFLOW_UI_TEST_SCREENSHOTS");
    int menuIndex = 0;
    for (auto *action : window.menuBar()->actions())
      if (auto *menu = action->menu()) {
        menu->popup(window.menuBar()->mapToGlobal(window.menuBar()->actionGeometry(action).bottomLeft()));
        QTest::qWait(20);
        if (!output.isEmpty())
          QVERIFY(menu->grab().save(output + QString("/foundation-menu-%1.png").arg(menuIndex++)));
        QTest::keyClick(menu, Qt::Key_Escape);
        QVERIFY(!menu->isVisible());
      }
    QVERIFY(!window.m_coreProcess.isStarted());
  }

  void foundationSessionStatesAndRoleActions_data()
  {
    QTest::addColumn<bool>("share");
    QTest::addColumn<int>("width");
    QTest::addColumn<bool>("chinese");
    for (bool share : {false, true})
      for (int width : {620, 900})
        for (bool zh : {false, true})
          QTest::newRow(
              qPrintable(QString("%1-%2-%3").arg(share ? "controller" : "receiver").arg(width).arg(zh ? "zh" : "en"))
          ) << share
            << width << zh;
  }
  void foundationSessionStatesAndRoleActions()
  {
    QFETCH(bool, share);
    QFETCH(int, width);
    QFETCH(bool, chinese);
    QTranslator translator;
    if (chinese) {
      QVERIFY(translator.load(":/translations/deskflow_zh_CN.qm"));
      qApp->installTranslator(&translator);
    }
    const auto restore = qScopeGuard([&] { qApp->removeTranslator(&translator); });
    Settings::setValue(Settings::Core::CoreMode, share ? Settings::CoreMode::Server : Settings::CoreMode::Client);
    Settings::setValue(Settings::Client::RemoteHost, "controller.example");
    MainWindow window;
    seed(window);
    window.resize(width, 760);
    const auto original = bytes(Settings::settingsFile());
    using H = ConnectionHome;
    using S = H::Stage;
    H::State state;
    state.role = share ? H::Role::Share : H::Role::Receive;
    state.localName = "Windows 工作电脑";
    state.peerName = "Mac 书桌";
    state.identityReady = true;
    state.canStart = true;
    state.hasAddress = !share;
    state.hasSavedDevices = true;
    state.peerKnown = true;
    const QList<S> phases = {S::Idle,        S::Starting, S::Connecting, S::Verifying, S::Listening, S::Connected,
                             S::Discovering, S::Retrying, S::Stopping,   S::Paused,    S::Failed,    S::NeedsAction};
    for (const auto phase : phases) {
      if (share && (phase == S::Connecting || phase == S::Verifying || phase == S::Discovering || phase == S::Retrying))
        continue;
      if (!share && phase == S::Listening)
        continue;
      state.stage = phase;
      state.retrySeconds = 3;
      state.canRetry = phase == S::Failed;
      state.explanation = phase == S::Failed        ? "The selected address did not respond."
                          : phase == S::NeedsAction ? "The computer identity changed. Review it before connecting."
                                                    : "Generic diagnostic text must not replace the relationship.";
      window.m_workspaceLive = phase == S::Connected || phase == S::Listening;
      window.refreshDesktopWorkspace();
      window.m_home->setState(state);
      QTest::qWait(30);
      auto *home = window.m_home;
      auto *primary = home->findChild<QPushButton *>("homePrimary");
      const bool inactive = phase == S::Idle || phase == S::Paused || phase == S::Failed || phase == S::NeedsAction;
      QCOMPARE(home->findChild<QPushButton *>("homeChangeDirection")->isVisible(), inactive);
      QCOMPARE(home->findChild<QPushButton *>("homeOptions")->isVisible(), inactive);
      QCOMPARE(home->findChild<QPushButton *>("homeSaved")->isVisible(), !share);
      QVERIFY(home->findChild<QWidget *>("homeLocalDevice")->isVisible());
      QVERIFY(home->findChild<QWidget *>("homePeerDevice")->isVisible());
      QVERIFY(!home->findChild<QLabel *>("homeStatus")->text().isEmpty());
      if (phase != S::Failed && phase != S::NeedsAction)
        QVERIFY(!home->findChild<QLabel *>("homeReason")->text().contains("Generic diagnostic"));
      for (auto *button : home->findChildren<QPushButton *>())
        if (button->isVisible()) {
          QVERIFY2(
              window.rect().contains(QRect(button->mapTo(&window, QPoint()), button->size())),
              qPrintable(button->objectName())
          );
          QStyleOptionButton option;
          option.initFrom(button);
          option.text = button->text();
          option.rect = button->rect();
          const auto contents = button->style()->subElementRect(QStyle::SE_PushButtonContents, &option, button);
          QVERIFY2(
              contents.width() + .5 >= QFontMetricsF(button->font()).horizontalAdvance(button->text()),
              qPrintable(button->text())
          );
        }
      QCOMPARE(primary->isEnabled(), phase != S::Stopping);
      const auto name = QString("foundation-%1-%2-%3-%4")
                            .arg(share ? "controller" : "receiver")
                            .arg(width)
                            .arg(chinese ? "zh" : "en")
                            .arg(int(phase));
      capture(window, name);
    }
    QCOMPARE(bytes(Settings::settingsFile()), original);
    QVERIFY(!window.m_coreProcess.isStarted());
  }

  void foundationPrimaryActionsMatchTheVisiblePhase()
  {
    using H = ConnectionHome;
    using S = H::Stage;
    H home;
    home.resize(760, 600);
    home.show();
    QSignalSpy start(&home, &H::startStopRequested), retry(&home, &H::retryRequested),
        review(&home, &H::diagnosticsRequested);
    QSignalSpy prepare(&home, &H::identitySetupRequested), saved(&home, &H::savedDevicesRequested),
        find(&home, &H::findDeviceRequested), add(&home, &H::addComputerRequested);
    H::State state;
    state.role = H::Role::Receive;
    home.setState(state);
    auto *primary = home.findChild<QPushButton *>("homePrimary");
    primary->click();
    QCOMPARE(prepare.count(), 1);
    QCOMPARE(start.count(), 0);
    state.identityReady = true;
    home.setState(state);
    primary->click();
    QCOMPARE(find.count(), 1);
    state.hasSavedDevices = true;
    home.setState(state);
    primary->click();
    QCOMPARE(saved.count(), 1);
    state.hasAddress = true;
    state.canStart = true;
    for (const auto phase :
         {S::Idle, S::Paused, S::Starting, S::Connecting, S::Verifying, S::Connected, S::Retrying, S::Discovering}) {
      state.stage = phase;
      home.setState(state);
      const int before = start.count();
      primary->click();
      primary->click();
      QCOMPARE(start.count(), before + 1);
    }
    state.stage = S::Stopping;
    home.setState(state);
    const int before = start.count();
    QVERIFY(!primary->isEnabled());
    primary->click();
    QCOMPARE(start.count(), before);
    state.stage = S::Failed;
    state.canRetry = true;
    home.setState(state);
    primary->click();
    primary->click();
    QCOMPARE(retry.count(), 1);
    QCOMPARE(review.count(), 0);
    state.canRetry = false;
    home.setState(state);
    primary->click();
    QCOMPARE(review.count(), 1);
    state.stage = S::NeedsAction;
    home.setState(state);
    primary->click();
    QCOMPARE(review.count(), 2);
    state.role = H::Role::Share;
    state.stage = S::Listening;
    home.setState(state);
    home.findChild<QPushButton *>("homeFind")->click();
    QCOMPARE(add.count(), 1);
    QCOMPARE(start.count(), before);
  }

  void foundationCompactPhaseLayouts()
  {
    QTranslator translator;
    QVERIFY(translator.load(":/translations/deskflow_zh_CN.qm"));
    qApp->installTranslator(&translator);
    const auto restore = qScopeGuard([&] { qApp->removeTranslator(&translator); });
    using H = ConnectionHome;
    using S = H::Stage;
    for (bool share : {false, true}) {
      Settings::setValue(Settings::Core::CoreMode, share ? Settings::CoreMode::Server : Settings::CoreMode::Client);
      MainWindow window;
      seed(window);
      window.resize(620, 480);
      H::State state;
      state.role = share ? H::Role::Share : H::Role::Receive;
      state.localName = "Windows";
      state.peerName = "Mac";
      state.identityReady = true;
      state.canStart = true;
      state.hasAddress = !share;
      state.hasSavedDevices = true;
      state.peerKnown = true;
      for (auto phase : {S::Idle, S::Starting, S::Connected, S::Listening, S::Paused, S::Failed, S::NeedsAction}) {
        if (!share && phase == S::Listening)
          continue;
        state.stage = phase;
        state.canRetry = phase == S::Failed;
        state.explanation = "Review the selected computer before connecting.";
        window.m_workspaceLive = phase == S::Connected || phase == S::Listening;
        window.refreshDesktopWorkspace();
        window.m_home->setState(state);
        QTest::qWait(30);
        window.resize(620, 480);
        QTest::qWait(10);
        capture(window, QString("foundation-compact-%1-%2").arg(share ? "controller" : "receiver").arg(int(phase)));
        QCOMPARE(window.size(), QSize(620, 480));
        for (auto *button : window.m_home->findChildren<QPushButton *>())
          if (button->isVisible())
            QVERIFY2(
                window.rect().contains(QRect(button->mapTo(&window, QPoint()), button->size())),
                qPrintable(button->objectName())
            );
        if (share)
          QVERIFY2(
              window.m_desktopWorkspace->canvas()->visibleRegion().boundingRect().height() >= 160,
              "Compact phase must retain an editable canvas"
          );
      }
      QVERIFY(!window.m_coreProcess.isStarted());
    }
  }
  void foundationRecoveryFailureStaysOpenWithoutAccepting()
  {
    const QString fp(64, 'b');
    DiscoveryScope network{3, "Test network", QHostAddress("192.168.50.2"), 24};
    AutomaticRecoveryDialog dialog(fp, {}, [&] { return QVector<DiscoveryScope>{network}; });
    dialog.setReceivingRole(true);
    auto *enable = dialog.findChild<QCheckBox *>("automaticRecoveryEnabled");
    auto *picker = dialog.findChild<QComboBox *>("automaticRecoveryNetwork");
    enable->setChecked(true);
    picker->setCurrentIndex(1);
    int attempts = 0;
    dialog.setApplyHandler([&](const AutomaticRecoveryPolicy &) {
      ++attempts;
      return QString("Identity preparation failed. Nothing was saved.");
    });
    QSignalSpy accepted(&dialog, &QDialog::accepted);
    dialog.show();
    dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click();
    QCOMPARE(attempts, 1);
    QCOMPARE(accepted.count(), 0);
    QVERIFY(dialog.isVisible());
    QVERIFY(dialog.findChild<QLabel *>("automaticRecoveryStatus")->text().contains("Nothing was saved"));
    dialog.resize(600, 440);
    QTest::qWait(20);
    const auto output = qEnvironmentVariable("DESKFLOW_UI_TEST_SCREENSHOTS");
    if (!output.isEmpty())
      QVERIFY(dialog.grab().save(output + "/foundation-recovery-preparation-failed.png"));
    dialog.setApplyHandler([&](const AutomaticRecoveryPolicy &) {
      ++attempts;
      return QString();
    });
    dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click();
    QCOMPARE(attempts, 2);
    QCOMPARE(accepted.count(), 1);
    dialog.reject();
  }
  void foundationOptionsWrapAndKeepNativeKeyboardBehavior()
  {
    QWidget page;
    applyExperienceStyle(&page);
    auto *layout = new QVBoxLayout(&page);
    auto *check = new OptionCheckBox(
        QString::fromUtf8("连接中保持清醒；这是一条较长的中文选项，第二行应从文本起始处对齐。"), &page
    );
    auto *radio = new OptionRadioButton(QString::fromUtf8("使用这台电脑的键盘和鼠标，控制另一台电脑。"), &page);
    layout->addWidget(check);
    layout->addWidget(radio);
    page.resize(320, 220);
    page.show();
    page.activateWindow();
    QTest::qWait(20);
    check->setFocus();
    QTRY_VERIFY(check->hasFocus());
    QTest::keyClick(check, Qt::Key_Space);
    QVERIFY(check->isChecked());
    QTest::keyClick(check, Qt::Key_Tab);
    QVERIFY(radio->hasFocus());
    QTest::keyClick(radio, Qt::Key_Space);
    QVERIFY(radio->isChecked());
    QVERIFY(check->height() >= check->heightForWidth(check->width()));
    const auto output = qEnvironmentVariable("DESKFLOW_UI_TEST_SCREENSHOTS");
    if (!output.isEmpty())
      QVERIFY(page.grab().save(output + "/foundation-options-wrap.png"));
  }
  void settingsFooterPaintingIsNotClipped_data()
  {
    QTest::addColumn<QSize>("size");
    QTest::newRow("700x500") << QSize(700, 500);
    QTest::newRow("640x480") << QSize(640, 480);
    QTest::newRow("900x720") << QSize(900, 720);
  }
  void settingsFooterPaintingIsNotClipped()
  {
    QFETCH(QSize, size);
    QTranslator translator;
    QVERIFY(translator.load(":/translations/deskflow_zh_CN.qm"));
    qApp->installTranslator(&translator);
    const auto restore = qScopeGuard([&] { qApp->removeTranslator(&translator); });
    MainWindow window;
    seed(window);
    const auto stored = bytes(Settings::settingsFile());
    bool visited = false;
    QTimer::singleShot(0, &window, [&] {
      auto *active = qobject_cast<SettingsDialog *>(QApplication::activeModalWidget());
      QVERIFY(active);
      visited = true;
      const auto close = qScopeGuard([active] {
        if (active->isVisible())
          active->reject();
      });
      auto &dialog = *active;
      dialog.resize(size);
      dialog.activateWindow();
      auto *footer = dialog.findChild<SettingsDialogButtonBox *>("settingsFooter");
      QVERIFY(footer);
      auto *box = footer->findChild<QDialogButtonBox *>();
      QVERIFY(box);
      auto *cancel = box->button(QDialogButtonBox::Cancel);
      cancel->setFocus(Qt::TabFocusReason);
      QTest::qWait(20);
      QCOMPARE(dialog.size(), size);
      QVERIFY(cancel->hasFocus());
      const auto output = qEnvironmentVariable("DESKFLOW_UI_TEST_SCREENSHOTS");
      if (!output.isEmpty())
        QVERIFY(dialog.grab().save(output + QString("/review-footer-%1.png").arg(QTest::currentDataTag())));
      for (auto *button : {box->button(QDialogButtonBox::Save), cancel}) {
        qInfo() << "footer paint region" << button->text() << button->geometry()
                << button->visibleRegion().boundingRect();
        for (auto *ancestor = button->parentWidget(); ancestor; ancestor = ancestor->parentWidget()) {
          qInfo() << "ancestor" << ancestor->metaObject()->className() << ancestor->objectName() << ancestor->geometry()
                  << ancestor->minimumSizeHint() << ancestor->sizeHint();
          if (ancestor == &dialog)
            break;
        }
        QVERIFY2(
            button->visibleRegion().contains(button->rect()), qPrintable(button->text() + " is clipped by an ancestor")
        );
        const auto inDialog = QRect(button->mapTo(&dialog, QPoint()), button->size());
        QVERIFY(dialog.rect().contains(inDialog.adjusted(0, 0, 0, 2)));
      }
      QSignalSpy rejected(&dialog, &QDialog::rejected);
      QTest::keyClick(cancel, Qt::Key_Space);
      QCOMPARE(rejected.count(), 1);
    });
    window.findChild<QPushButton *>("homeSettings")->click();
    QVERIFY(visited);
    QCOMPARE(bytes(Settings::settingsFile()), stored);
  }
  void emptyShortcutRequiresRecordingBeforeAccept()
  {
    QTranslator translator, qt;
    QVERIFY(translator.load(":/translations/deskflow_zh_CN.qm"));
    qApp->installTranslator(&translator);
    QVERIFY(qt.load("qtbase_zh_CN", QLibraryInfo::path(QLibraryInfo::TranslationsPath)));
    qApp->installTranslator(&qt);
    const auto restore = qScopeGuard([&] {
      qApp->removeTranslator(&qt);
      qApp->removeTranslator(&translator);
    });
    Hotkey key;
    const auto original = key.text();
    HotkeyDialog dialog(nullptr, key);
    dialog.resize(560, 480);
    dialog.show();
    dialog.activateWindow();
    QTest::qWait(20);
    auto *box = dialog.findChild<QDialogButtonBox *>();
    QVERIFY(box);
    auto *ok = box->button(QDialogButtonBox::Ok);
    auto *record = dialog.findChild<KeySequenceWidget *>();
    QVERIFY(record);
    const auto output = qEnvironmentVariable("DESKFLOW_UI_TEST_SCREENSHOTS");
    if (!output.isEmpty())
      QVERIFY(dialog.grab().save(output + "/review-hotkey-empty.png"));
    QVERIFY(!ok->isEnabled());
    QSignalSpy accepted(&dialog, &QDialog::accepted);
    ok->click();
    QCOMPARE(accepted.count(), 0);
    record->setFocus();
    QTest::keyClick(record, Qt::Key_Space);
    QTest::keyClick(record, Qt::Key_F8, Qt::ControlModifier);
    QVERIFY(record->valid());
    QVERIFY(ok->isEnabled());
    QTest::qWait(20);
    if (!output.isEmpty())
      QVERIFY(dialog.grab().save(output + "/review-hotkey-valid.png"));
    const auto valid = record->keySequence();
    dialog.reject();
    QCOMPARE(key.text(), original);
    key.setKeySequence(valid);
    HotkeyDialog reopened(nullptr, key);
    reopened.show();
    reopened.activateWindow();
    QTest::qWait(10);
    auto *reopenedOk = reopened.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
    QVERIFY(reopenedOk->isEnabled());
    auto *reRecord = reopened.findChild<KeySequenceWidget *>();
    reRecord->setFocus();
    QTest::keyClick(reRecord, Qt::Key_Space);
    QVERIFY(!reopenedOk->isEnabled());
    QTest::keyClick(reRecord, Qt::Key_F9, Qt::ControlModifier);
    QVERIFY(reopenedOk->isEnabled());
    QSignalSpy saved(&reopened, &QDialog::accepted);
    QTest::mouseClick(reopenedOk, Qt::LeftButton);
    QCOMPARE(saved.count(), 1);
    QCOMPARE(key.keySequence(), reRecord->keySequence());
  }
  void idleDiagnosticsNamesTheActualPreparationPath()
  {
    QTranslator translator;
    QVERIFY(translator.load(":/translations/deskflow_zh_CN.qm"));
    qApp->installTranslator(&translator);
    const auto restore = qScopeGuard([&] { qApp->removeTranslator(&translator); });
    MainWindow window;
    seed(window);
    window.resize(760, 560);
    const auto stored = bytes(Settings::settingsFile());
    auto *advertise = window.findChild<QCheckBox *>("advertiseNearbyServer");
    QVERIFY(advertise);
    const bool exposure = advertise->isChecked();
    window.findChild<QPushButton *>("homeDiagnostics")->click();
    QTest::qWait(20);
    capture(window, "review-idle-diagnostics");
    auto *diagnostics = window.findChild<ConnectionDiagnostics *>();
    QVERIFY(diagnostics);
    QVERIFY(diagnostics->explanation().contains(QString::fromUtf8("我的桌面")));
    QVERIFY(diagnostics->explanation().contains(QString::fromUtf8("添加电脑")));
    window.findChild<QPushButton *>("backToComputers")->click();
    window.findChild<QPushButton *>("workspaceAdd")->click();
    QTest::qWait(20);
    QVERIFY(window.findChild<QWidget *>("serverDiscoveryControls")->isVisible());
    QCOMPARE(advertise->isChecked(), exposure);
    QCOMPARE(bytes(Settings::settingsFile()), stored);
    QVERIFY(!window.m_coreProcess.isStarted());
  }
  void formsKeepFooterAndCancelDraftsAtBothWidths_data()
  {
    QTest::addColumn<bool>("dark");
    QTest::addColumn<bool>("chinese");
    for (bool dark : {false, true})
      for (bool chinese : {false, true})
        QTest::newRow(qPrintable(QString("dark%1-zh%2").arg(dark).arg(chinese))) << dark << chinese;
  }
  void formsKeepFooterAndCancelDraftsAtBothWidths()
  {
    QFETCH(bool, dark);
    QFETCH(bool, chinese);
    const auto oldPalette = qApp->palette();
    QPalette palette = oldPalette;
    for (auto role : {QPalette::Window, QPalette::Button, QPalette::Base})
      palette.setColor(role, dark ? QColor("#202630") : QColor("#fafafa"));
    for (auto role : {QPalette::WindowText, QPalette::ButtonText, QPalette::Text})
      palette.setColor(role, dark ? QColor("#e7edf5") : QColor("#202124"));
    qApp->setPalette(palette);
    QTranslator language, qt;
    if (chinese) {
      QVERIFY(language.load(":/translations/deskflow_zh_CN.qm"));
      qApp->installTranslator(&language);
      QVERIFY(qt.load("qtbase_zh_CN", QLibraryInfo::path(QLibraryInfo::TranslationsPath)));
      qApp->installTranslator(&qt);
    }
    const auto restore = qScopeGuard([&] {
      qApp->removeTranslator(&qt);
      qApp->removeTranslator(&language);
      qApp->setPalette(oldPalette);
    });
    MainWindow window;
    seed(window);
    const auto stored = bytes(Settings::settingsFile());
    auto inspect = [&](QDialog &dialog) {
      auto *scroll = dialog.findChild<QScrollArea *>("formScroll");
      QVERIFY(scroll);
      for (const auto size : {QSize(560, 480), QSize(760, 640)}) {
        dialog.resize(size);
        dialog.show();
        QTest::qWait(10);
        QCOMPARE(dialog.size(), size);
        scroll->verticalScrollBar()->setValue(0);
        const auto output = qEnvironmentVariable("DESKFLOW_UI_TEST_SCREENSHOTS");
        if (!output.isEmpty())
          QVERIFY(dialog.grab().save(
              output + QString("/forms-%1-%2-%3x%4.png")
                           .arg(QTest::currentDataTag(), dialog.objectName())
                           .arg(size.width())
                           .arg(size.height())
          ));
        QCOMPARE(scroll->horizontalScrollBar()->maximum(), 0);
        auto *footer = dialog.findChild<QDialogButtonBox *>();
        QVERIFY(footer);
        QVERIFY(dialog.rect().contains(QRect(footer->mapTo(&dialog, QPoint()), footer->size())));
        auto *actionRow = dialog.layout()->itemAt(1)->widget();
        QVERIFY(actionRow);
        for (auto *button : dialog.findChildren<QAbstractButton *>())
          if (button->isVisible())
            QVERIFY(scroll->widget()->isAncestorOf(button) || actionRow == button || actionRow->isAncestorOf(button));
        for (auto *button : footer->findChildren<QPushButton *>())
          if (button->isVisible()) {
            QStyleOptionButton option;
            option.initFrom(button);
            option.text = button->text();
            option.rect = button->rect();
            const auto content = button->style()->subElementRect(QStyle::SE_PushButtonContents, &option, button);
            QVERIFY2(
                content.width() + 0.5 >= QFontMetricsF(button->font(), button).horizontalAdvance(button->text()),
                qPrintable(button->text())
            );
          }
        if (dialog.objectName() == "ComputerSettingsDialog") {
          auto *modifiers = dialog.findChild<QGroupBox *>("groupModifers");
          auto *fixes = dialog.findChild<QGroupBox *>("groupFixes");
          QVERIFY(modifiers);
          QVERIFY(fixes);
          const int modifierTop = modifiers->mapTo(&dialog, QPoint()).y();
          const int fixesTop = fixes->mapTo(&dialog, QPoint()).y();
          if (size.width() >= 700)
            QCOMPARE(modifierTop, fixesTop);
          else
            QVERIFY(fixesTop >= modifierTop + modifiers->height());
        }
        scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());
        if (!output.isEmpty() && dialog.objectName() == "ComputerSettingsDialog")
          QVERIFY(dialog.grab().save(
              output +
              QString("/forms-bottom-%1-%2x%3.png").arg(QTest::currentDataTag()).arg(size.width()).arg(size.height())
          ));
        QVERIFY(dialog.rect().contains(QRect(footer->mapTo(&dialog, QPoint()), footer->size())));
      }
      dialog.activateWindow();
      QTest::qWait(1);
      QSignalSpy cancel(&dialog, &QDialog::rejected);
      auto *cancelButton = dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Cancel);
      cancelButton->setFocus();
      QTest::keyClick(cancelButton, Qt::Key_Space);
      QCOMPARE(cancel.count(), 1);
    };
    ClientConfigDialog client(&window);
    client.findChild<QCheckBox *>("cbLanguageSync")->toggle();
    inspect(client);
    Computer computer("Receiver");
    const auto original = computer;
    ComputerSettingsDialog options(&window, &computer);
    options.findChild<QLineEdit *>("lineNameEdit")->setText("Unsaved");
    inspect(options);
    QCOMPARE(computer, original);
    Hotkey key;
    HotkeyDialog keys(&window, key);
    inspect(keys);
    Action action;
    ActionDialog actions(&window, window.m_serverConfig, key, action);
    inspect(actions);
    QCOMPARE(bytes(Settings::settingsFile()), stored);
  }
  void settingsAndAdvancedPagesKeepFooterAtMinimumSize()
  {
    MainWindow window;
    seed(window);
    const auto stored = bytes(Settings::settingsFile());
    SettingsDialog settings(&window, window.m_serverConfig);
    for (const auto size : {QSize(640, 480), QSize(900, 720)})
      for (int page = 0; page < 5; ++page) {
        settings.resize(size);
        settings.findChild<QTabWidget *>("tabWidget")->setCurrentIndex(page);
        settings.show();
        QTest::qWait(10);
        QCOMPARE(settings.size(), size);
        auto *footer = settings.findChild<QWidget *>("settingsFooter");
        QVERIFY(settings.rect().contains(QRect(footer->mapTo(&settings, QPoint()), footer->size())));
        if (page == 4)
          for (auto *part : settings.findChildren<QWidget *>())
            if (part->objectName() == "cbElevateDaemon" || part->objectName() == "elevationHelp" ||
                part->objectName() == "groupService")
              qInfo() << "service geometry" << part->objectName() << part->geometry() << part->isVisible()
                      << part->sizeHint() << part->minimumSizeHint();
        const auto output = qEnvironmentVariable("DESKFLOW_UI_TEST_SCREENSHOTS");
        if (!output.isEmpty())
          QVERIFY(settings.grab().save(
              output + QString("/settings-min-%1x%2-page%3.png").arg(size.width()).arg(size.height()).arg(page)
          ));
        for (auto *scroll : settings.findChildren<QScrollArea *>())
          if (scroll->isVisible()) {
            if (scroll->horizontalScrollBar()->maximum() > 0) {
              qInfo() << "overflow" << page << size << scroll->viewport()->size()
                      << scroll->widget()->minimumSizeHint();
              for (auto *child : scroll->widget()->findChildren<QWidget *>())
                if (child->isVisible() && child->minimumSizeHint().width() > scroll->viewport()->width() - 40)
                  qInfo() << child->objectName() << child->metaObject()->className() << child->size()
                          << child->minimumSizeHint();
            }
            QCOMPARE(scroll->horizontalScrollBar()->maximum(), 0);
          }
        for (auto *button : footer->findChildren<QPushButton *>())
          if (button->isVisible()) {
            QStyleOptionButton option;
            option.initFrom(button);
            option.text = button->text();
            option.rect = button->rect();
            QVERIFY(
                button->style()->subElementRect(QStyle::SE_PushButtonContents, &option, button).width() + 0.5 >=
                QFontMetricsF(button->font(), button).horizontalAdvance(button->text())
            );
          }
      }
    settings.reject();
    ServerConfigDialog advanced(&window, window.m_serverConfig);
    advanced.showAdvancedOnly();
    for (const auto size : {QSize(620, 480), QSize(900, 720)})
      for (int page = 0; page < 3; ++page) {
        advanced.resize(size);
        advanced.findChild<QTabWidget *>("tabWidget")->setCurrentIndex(page);
        advanced.show();
        QTest::qWait(10);
        QCOMPARE(advanced.size(), size);
        auto *footer = advanced.findChild<SettingsDialogButtonBox *>();
        QVERIFY(advanced.rect().contains(QRect(footer->mapTo(&advanced, QPoint()), footer->size())));
        for (auto *scroll : advanced.findChildren<QScrollArea *>())
          if (scroll->isVisible())
            QCOMPARE(scroll->horizontalScrollBar()->maximum(), 0);
      }
    advanced.reject();
    QCOMPARE(bytes(Settings::settingsFile()), stored);
  }
  void shortcutCanBeRecordedFromKeyboardAndDiscarded()
  {
    Hotkey key;
    const auto original = key.text();
    HotkeyDialog dialog(nullptr, key);
    dialog.show();
    dialog.activateWindow();
    QTest::qWait(1);
    auto *record = dialog.findChild<KeySequenceWidget *>();
    QVERIFY(record);
    QVERIFY(record->focusPolicy() != Qt::NoFocus);
    QCOMPARE(record->text(), QString("Record shortcut…"));
    record->setFocus();
    QTest::keyClick(record, Qt::Key_Space);
    QCOMPARE(record->text(), QString("Press a shortcut…"));
    QTest::keyClick(record, Qt::Key_F8, Qt::ControlModifier);
    QVERIFY(record->valid());
    QVERIFY(record->text() != QString("Press a shortcut…"));
    dialog.reject();
    QCOMPARE(key.text(), original);
  }
  void advancedComputerOptionsKeepWorkspaceAndOuterCancel()
  {
    MainWindow window;
    seed(window);
    const auto original = window.m_serverConfig;
    const auto stored = bytes(Settings::settingsFile());
    ServerConfigDialog advanced(&window, window.m_serverConfig);
    advanced.showAdvancedOnly();
    advanced.show();
    auto *picker = advanced.findChild<QComboBox *>("computerOptionsPicker");
    QVERIFY(picker);
    QCOMPARE(picker->count(), 2);
    QVERIFY(!advanced.findChild<QWidget *>("computerSetupView")->isVisible());
    picker->setCurrentIndex(picker->findText("Receiver"));
    bool opened = false;
    QTimer::singleShot(0, &advanced, [&] {
      auto *options = qobject_cast<ComputerSettingsDialog *>(QApplication::activeModalWidget());
      QVERIFY(options);
      const auto close = qScopeGuard([options] {
        if (options->isVisible())
          options->reject();
      });
      opened = true;
      QVERIFY(options->findChild<QLineEdit *>("lineNameEdit")->isReadOnly());
      options->findChild<QCheckBox *>("chkWeakX11Focus")->toggle();
      options->accept();
    });
    advanced.findChild<QPushButton *>("editComputerOptions")->click();
    QVERIFY(opened);
    advanced.reject();
    QCOMPARE(window.m_serverConfig, original);
    QCOMPARE(bytes(Settings::settingsFile()), stored);
  }
  void logSearchButtonsHaveVisibleIconsAndKeyboardFeedback()
  {
    MainWindow window;
    seed(window);
    // This test never starts a core. Keep the synthetic workspace offline too,
    // so the log-control screenshot does not imply a live routing session.
    window.m_workspaceLive = false;
    window.refreshDesktopWorkspace();
    window.m_logDock->show();
    window.resize(900, 720);
    QTest::qWait(10);
    auto *search = window.findChild<SearchWidget *>();
    QVERIFY(search);
    auto *toggle = search->findChild<QPushButton *>("logSearchToggle");
    QVERIFY(toggle);
    QVERIFY(!toggle->icon().isNull());
    toggle->setFocus();
    QTest::keyClick(toggle, Qt::Key_Space);
    QVERIFY(search->isExpanded());
    auto *text = search->findChild<QLineEdit *>("logSearchText");
    text->setText("isolated search");
    QSignalSpy next(search, &SearchWidget::findNext);
    auto *button = search->findChild<QPushButton *>("logSearchNext");
    QVERIFY(!button->icon().isNull());
    button->setFocus();
    QTest::keyClick(button, Qt::Key_Space);
    QCOMPARE(next.count(), 1);
    QCOMPARE(next.first().first().toString(), QString("isolated search"));
    capture(window, "log-search-expanded");
    toggle->click();
    QVERIFY(!search->isExpanded());
    auto *menu = window.menuBar()->actions().first()->menu();
    QVERIFY(menu);
    menu->popup(window.menuBar()->mapToGlobal(QPoint(0, window.menuBar()->height())));
    QTest::qWait(10);
    const auto output = qEnvironmentVariable("DESKFLOW_UI_TEST_SCREENSHOTS");
    if (!output.isEmpty())
      QVERIFY(menu->grab().save(output + "/menu-popup.png"));
    menu->hide();
  }
  void captureLegacyDialogsForReview()
  {
    QTranslator translator, qt;
    QVERIFY(translator.load(":/translations/deskflow_zh_CN.qm"));
    qApp->installTranslator(&translator);
    QVERIFY(qt.load("qtbase_zh_CN", QLibraryInfo::path(QLibraryInfo::TranslationsPath)));
    qApp->installTranslator(&qt);
    const auto restore = qScopeGuard([&] {
      qApp->removeTranslator(&qt);
      qApp->removeTranslator(&translator);
    });
    MainWindow window;
    seed(window);
    const auto before = bytes(Settings::settingsFile());
    auto render = [&](QDialog &dialog, const QString &name) {
      for (const auto size : {QSize(560, 480), QSize(760, 640)}) {
        dialog.resize(size);
        dialog.show();
        QTest::qWait(10);
        if (name == "startup")
          QTRY_VERIFY_WITH_TIMEOUT(dialog.findChild<QPushButton *>("startupRefresh")->isEnabled(), 3000);
        const auto output = qEnvironmentVariable("DESKFLOW_UI_TEST_SCREENSHOTS");
        if (!output.isEmpty())
          QVERIFY(dialog.grab().save(
              output + QString("/legacy-%1-%2x%3.png").arg(name).arg(size.width()).arg(size.height())
          ));
      }
      dialog.reject();
    };
    ClientConfigDialog client(&window);
    render(client, "client");
    Computer computer("Receiver");
    const auto original = computer;
    ComputerSettingsDialog options(&window, &computer);
    render(options, "computer");
    QCOMPARE(computer, original);
    Hotkey hotkey;
    HotkeyDialog keys(&window, hotkey);
    render(keys, "hotkey");
    Action action;
    ActionDialog actions(&window, window.m_serverConfig, hotkey, action);
    auto *type = actions.findChild<QComboBox *>("comboActionType");
    for (int i = 0; i < type->count(); ++i) {
      type->setCurrentIndex(i);
      render(actions, QString("action-%1").arg(i));
    }
    AboutDialog about(&window);
    render(about, "about");
    StartupRecoveryDialog startupDialog(startup::makeReadOnlyReader(), {true, false, false}, &window);
    render(startupDialog, "startup");
    ServerConfigDialog advanced(&window, window.m_serverConfig);
    advanced.showAdvancedOnly();
    auto *tabs = advanced.findChild<QTabWidget *>("tabWidget");
    for (int i = 0; i < tabs->count(); ++i) {
      tabs->setCurrentIndex(i);
      render(advanced, QString("server-%1").arg(i));
    }
    QCOMPARE(bytes(Settings::settingsFile()), before);
    QVERIFY(!window.m_coreProcess.isStarted());
  }
  void captureSecondaryPagesForReview()
  {
    QTranslator translator;
    QVERIFY(translator.load(":/translations/deskflow_zh_CN.qm"));
    qApp->installTranslator(&translator);
    const auto restore = qScopeGuard([&] { qApp->removeTranslator(&translator); });
    MainWindow window;
    seed(window);
    const auto before = bytes(Settings::settingsFile());
    auto save = [&](QWidget *widget, const QString &name) {
      const auto output = qEnvironmentVariable("DESKFLOW_UI_TEST_SCREENSHOTS");
      if (output.isEmpty())
        return;
      QDir().mkpath(output);
      QApplication::processEvents();
      QVERIFY(widget->grab().save(output + '/' + name + ".png"));
      if (name == "settings-700x500-page0") {
        auto *footer = widget->findChild<SettingsDialogButtonBox *>("settingsFooter");
        QVERIFY(footer);
        auto *box = footer->findChild<QDialogButtonBox *>();
        QVERIFY(box);
        for (auto *button : {box->button(QDialogButtonBox::Save), box->button(QDialogButtonBox::Cancel)}) {
          qInfo() << "captured footer" << button->text() << button->geometry() << button->visibleRegion().boundingRect()
                  << "box" << box->geometry() << box->sizeHint() << box->minimumSizeHint() << "footer"
                  << footer->geometry();
          QVERIFY2(
              button->visibleRegion().contains(button->rect()),
              qPrintable(name + ": " + button->text() + " painted outside action container")
          );
        }
      }
    };
    bool seen = false;
    QTimer::singleShot(0, &window, [&] {
      auto *dialog = qobject_cast<SettingsDialog *>(QApplication::activeModalWidget());
      QVERIFY(dialog);
      seen = true;
      const auto close = qScopeGuard([dialog] { dialog->reject(); });
      for (const auto size : {QSize(700, 500), QSize(900, 720)}) {
        dialog->resize(size);
        auto *tabs = dialog->findChild<QTabWidget *>("tabWidget");
        for (int page = 0; page < tabs->count(); ++page) {
          tabs->setCurrentIndex(page);
          QTest::qWait(10);
          save(dialog, QString("settings-%1x%2-page%3").arg(size.width()).arg(size.height()).arg(page));
        }
      }
      tabsForIdentity(dialog);
      save(dialog, "identity-details");
    });
    window.openSettings();
    QVERIFY(seen);
    window.showConnectionOptions();
    window.resize(760, 560);
    QTest::qWait(10);
    save(&window, "connection-setup");
    window.showConnectionOptions(true);
    window.resize(760, 560);
    QTest::qWait(10);
    save(&window, "diagnostics-page");
    window.m_homePages->setCurrentWidget(window.m_home);
    bool popup = false;
    QTimer::singleShot(0, &window, [&] {
      if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
        popup = true;
        save(box, "add-computer");
        box->reject();
      }
    });
    auto *add = window.m_desktopWorkspace->findChild<QPushButton *>("workspaceAdd");
    QVERIFY(add);
    add->click();
    QTest::qWait(10);
    if (!popup)
      save(&window, "add-computer");
    QCOMPARE(bytes(Settings::settingsFile()), before);
    QVERIFY(!window.m_coreProcess.isStarted());
  }
  void init()
  {
    const auto path = m_root + '/' + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".conf";
    Settings::setSettingsFile(path);
    Settings::setValue(Settings::Core::Language, "en");
    I18N::setLanguage("en");
    Settings::setValue(Settings::Core::ComputerName, "Controller");
    Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::Server);
    Settings::setValue(Settings::Core::ProcessMode, Settings::ProcessMode::Desktop);
    Settings::setValue(Settings::Gui::AutoStartCore, false);
    Settings::setValue(Settings::Gui::AutoUpdateCheck, false);
    Settings::setValue(Settings::Gui::LogExpanded, false);
    Settings::setValue(Settings::Security::TlsEnabled, true);
    Settings::setValue(Settings::Security::CheckPeers, true);
    Settings::setValue(Settings::Security::Certificate, m_root + "/unused-test-certificate.pem");
    Settings::setValue(Settings::Server::ExternalConfig, false);
    Settings::setValue(Settings::Log::ToFile, false);
    Settings::save(false);
  }
  void actualWindowCanvasAndLogStayUsable_data()
  {
    QTest::addColumn<QSize>("viewport");
    QTest::addColumn<bool>("log");
    QTest::addColumn<bool>("dark");
    for (const auto size : {QSize(620, 480), QSize(900, 720), QSize(1280, 960)})
      for (bool log : {false, true})
        for (bool dark : {false, true})
          QTest::newRow(qPrintable(QString("%1x%2-log%3-dark%4").arg(size.width()).arg(size.height()).arg(log).arg(dark)
          )) << size
             << log << dark;
  }
  void actualWindowCanvasAndLogStayUsable()
  {
    QFETCH(QSize, viewport);
    QFETCH(bool, log);
    QFETCH(bool, dark);
    const auto previous = qApp->palette();
    QPalette palette = previous;
    for (auto role : {QPalette::Window, QPalette::Button, QPalette::Base})
      palette.setColor(role, dark ? QColor("#202630") : QColor("#fafafa"));
    for (auto role : {QPalette::WindowText, QPalette::ButtonText, QPalette::Text})
      palette.setColor(role, dark ? QColor("#e7edf5") : QColor("#202124"));
    qApp->setPalette(palette);
    QTranslator translator;
    QVERIFY(translator.load(":/translations/deskflow_zh_CN.qm"));
    qApp->installTranslator(&translator);
    const auto restore = qScopeGuard([&] {
      qApp->removeTranslator(&translator);
      qApp->setPalette(previous);
    });
    MainWindow window;
    seed(window);
    auto *view = window.m_desktopWorkspace;
    // The production view and real LogDock compose this fixture. The Lab core
    // never executes, and all settings belong to DESKFLOW_TEST_ROOT.
    auto s = window.m_home->state();
    s.stage = ConnectionHome::Stage::Connected;
    s.identityReady = s.canStart = true;
    s.peerName = "Receiver";
    s.explanation.clear();
    window.m_home->setState(s);
    view->setActiveRouting(QString::fromUtf8("当前核心已应用此排列。"));
    window.m_logDock->setVisible(log);
    if (log)
      window.m_logDock->appendLine("Isolated UI review: no input core or network is running.");
    window.resize(viewport);
    QTest::qWait(30);
    // Match the user's compact log panel instead of an unrelated floating log.
    if (log)
      window.resizeDocks({window.m_logDock}, {100}, Qt::Vertical);
    QTest::qWait(30);
    // Showing the real LogDock changes its saved preference and refreshes the
    // controller. Reapply the explicit review state after that delivery; this
    // is a simulated connected UI, never evidence of a real input connection.
    window.m_home->setState(s);
    view->setActiveRouting(QString::fromUtf8("当前核心已应用此排列。"));
    QTest::qWait(20);
    QCOMPARE(window.m_home->state().stage, ConnectionHome::Stage::Connected);
    const auto name = QString("composition-%1").arg(QString::fromLatin1(QTest::currentDataTag()));
    capture(window, name);
    auto *primary = window.findChild<QPushButton *>("homePrimary");
    auto *canvas = view->canvas();
    const QRect canvasInWindow(canvas->mapTo(&window, QPoint()), canvas->size());
    const QRect primaryInWindow(primary->mapTo(&window, QPoint()), primary->size());
    const auto output = qEnvironmentVariable("DESKFLOW_UI_TEST_SCREENSHOTS");
    QJsonObject geometry{
        {"requested_width", viewport.width()},
        {"requested_height", viewport.height()},
        {"window_width", window.width()},
        {"window_height", window.height()},
        {"canvas_width", canvas->width()},
        {"canvas_height", canvas->height()},
        {"canvas_visible_height", canvas->visibleRegion().boundingRect().height()},
        {"primary_height", primary->height()},
        {"primary_visible_height", primary->visibleRegion().boundingRect().height()},
        {"home_summary_height", window.findChild<QWidget *>("homeColumn")->height()},
        {"log_height", log ? window.m_logDock->height() : 0}
    };
    if (!output.isEmpty()) {
      QFile f(output + '/' + name + ".json");
      QVERIFY(f.open(QIODevice::WriteOnly));
      f.write(QJsonDocument(geometry).toJson());
    }
    QCOMPARE(window.size(), viewport);
    QVERIFY2(
        primary->visibleRegion().boundingRect().contains(primary->rect()),
        "Primary action clipped by its actual parent hierarchy"
    );
    QVERIFY(window.rect().contains(primaryInWindow));
    QVERIFY2(
        canvas->visibleRegion().boundingRect().height() >= 160,
        "Visible canvas must remain usable alongside the real log dock"
    );
    QVERIFY2(
        canvas->visibleRegion().boundingRect().contains(canvas->rect()),
        "The canvas itself must not scroll out behind its inspector"
    );
    auto *selection = view->findChild<QComboBox *>("workspaceSelection");
    QVERIFY2(
        selection->visibleRegion().boundingRect().contains(selection->rect()),
        "Screen selection must be available beside the canvas"
    );
    QVERIFY(!window.m_coreProcess.isStarted());
  }
  void realHomeShowsBothLocalScreensEvenWithOldPeer()
  {
    MainWindow window;
    seed(window, true);
    auto *view = window.m_desktopWorkspace;
    QVERIFY(view->isVisible());
    QCOMPARE(view->workspace().displays().size(), 2);
    QCOMPARE(view->workspace().unknownDevices(), QStringList{"Receiver"});
    QVERIFY(!window.findChild<QPushButton *>("homeLayout")->isVisible());
    QVERIFY(!window.findChild<QScrollArea *>("homeScroll")->isVisible());
    QVERIFY(!window.findChild<QPushButton *>("inspectLocalDisplays"));
    capture(window, "controller-old-peer");
  }
  void actualWindowDraftStatusAndScreenDetails()
  {
    QTranslator translator;
    QVERIFY(translator.load(":/translations/deskflow_zh_CN.qm"));
    qApp->installTranslator(&translator);
    const auto restore = qScopeGuard([&] { qApp->removeTranslator(&translator); });
    MainWindow window;
    seed(window);
    window.resize(900, 720);
    auto *view = window.m_desktopWorkspace;
    auto plan = view->workspace().state();
    for (auto &placement : plan.layout.placements)
      if (placement.device == "Receiver")
        placement.position = {590, -1080};
    view->setDraft(plan.encode(), plan.layout.encode());
    const auto active = MainWindow::tr("The current sharing core is using the last confirmed arrangement.");
    view->setActiveRouting(active);
    auto state = window.m_home->state();
    state.stage = ConnectionHome::Stage::Connected;
    state.peerName = "Receiver";
    state.explanation.clear();
    window.m_home->setState(state);
    QTest::qWait(20);
    capture(window, "state-applied-above-first-screen");
    auto *selection = view->findChild<QComboBox *>("workspaceSelection");
    view->findChild<QScrollArea *>("workspaceScroll")->ensureWidgetVisible(selection, 4, 4);
    selection->setFocus();
    QTest::keyClick(selection, Qt::Key_Home);
    for (int index = 0; index < selection->findData("Controller/b"); ++index)
      QTest::keyClick(selection, Qt::Key_Down);
    QTest::qWait(20);
    view->findChild<QScrollArea *>("workspaceScroll")
        ->ensureWidgetVisible(view->findChild<QLabel *>("workspaceInspector"), 4, 4);
    capture(window, "details-rounded-175-percent");
    const bool accurateScale = view->findChild<QLabel *>("workspaceInspector")->text().contains("175%") &&
                               !view->findChild<QLabel *>("workspaceInspector")->text().contains("175.1%");
    QTest::mouseClick(view->findChild<QPushButton *>("workspaceEdit"), Qt::LeftButton);
    selection->setCurrentIndex(selection->findData("Receiver/r"));
    QTest::keyClick(view->canvas(), Qt::Key_Up);
    QTest::qWait(20);
    capture(window, "state-invalid-draft-keeps-last-applied");
    QVERIFY(!view->workspace().validationError().isEmpty());
    QVERIFY(!view->findChild<QPushButton *>("workspaceApply")->isEnabled());
    QCOMPARE(view->findChild<QPushButton *>("workspaceApply")->text(), DesktopWorkspaceView::tr("Apply arrangement"));
    QVERIFY(view->findChild<QLabel *>("workspaceActiveRouting")->text() != active);
    QVERIFY(view->findChild<QLabel *>("workspaceActiveRouting")->text().contains(QString::fromUtf8("当前草稿尚未应用"))
    );
    QVERIFY(!view->findChild<QLabel *>("workspaceActiveRouting")->text().contains(QString::fromUtf8("此排列")));
    const auto draft = view->workspace().state().encode();
    view->setAppliedLayout(plan.layout.encode());
    QCOMPARE(view->workspace().state().encode(), draft); // A previous plan report cannot replace the new draft.
    QTest::mouseClick(view->findChild<QPushButton *>("workspaceCancel"), Qt::LeftButton);
    QCOMPARE(view->workspace().state().encode(), plan.encode());
    QCOMPARE(view->findChild<QLabel *>("workspaceActiveRouting")->text(), active);
    QVERIFY2(accurateScale, "Native logical-pixel rounding must not be presented as different X/Y scale");
    QVERIFY(!window.m_coreProcess.isStarted());
  }
  void settingsDiagnosticsAndAdvancedRemainReachable()
  {
    MainWindow window;
    seed(window);
    bool preferences = false, unexpectedDirty = false;
    QTimer::singleShot(0, &window, [&] {
      auto *dialog = qobject_cast<SettingsDialog *>(QApplication::activeModalWidget());
      QVERIFY(dialog);
      preferences = true;
      const auto close = qScopeGuard([dialog] {
        if (dialog->isVisible())
          dialog->reject();
      });
      dialog->findChild<QTabWidget *>("tabWidget")->setCurrentIndex(3);
      QTimer::singleShot(0, dialog, [&] {
        if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
          unexpectedDirty = true;
          box->button(QMessageBox::Discard)->click();
        }
      });
      dialog->findChild<QPushButton *>("connectionSetup")->click();
    });
    QTest::mouseClick(window.findChild<QPushButton *>("homeSettings"), Qt::LeftButton);
    QVERIFY(preferences);
    QVERIFY(!unexpectedDirty);
    QCOMPARE(window.m_homePages->currentWidget(), window.m_connectionOptionsPage);
    QCOMPARE(window.findChild<QLabel *>("secondaryPageTitle")->text(), QString("Computers and pairing"));
    bool inspected = false;
    QTimer::singleShot(0, &window, [&] {
      auto *dialog = qobject_cast<ServerConfigDialog *>(QApplication::activeModalWidget());
      QVERIFY(dialog);
      auto *pages = dialog->findChild<QStackedWidget *>("serverConfigPages");
      QVERIFY(pages);
      QCOMPARE(pages->currentIndex(), 1);
      QVERIFY(!dialog->findChild<QPushButton *>("inspectLocalDisplays")->isVisible());
      inspected = true;
      dialog->reject();
    });
    QTest::mouseClick(window.findChild<QPushButton *>("advancedSharingSettings"), Qt::LeftButton);
    QVERIFY(inspected);
    bool returned = false;
    QTimer::singleShot(0, &window, [&] {
      auto *dialog = qobject_cast<SettingsDialog *>(QApplication::activeModalWidget());
      QVERIFY(dialog);
      returned = true;
      QCOMPARE(dialog->findChild<QTabWidget *>("tabWidget")->currentIndex(), 3);
      dialog->reject();
    });
    QTest::mouseClick(window.findChild<QPushButton *>("backToComputers"), Qt::LeftButton);
    QVERIFY(returned);
    QCOMPARE(window.m_homePages->currentWidget(), static_cast<QWidget *>(window.m_home));
    QTest::mouseClick(window.findChild<QPushButton *>("homeDiagnostics"), Qt::LeftButton);
    QCOMPARE(window.findChild<QLabel *>("secondaryPageTitle")->text(), QString("Diagnostics"));
    QVERIFY(!window.findChild<QPushButton *>("advancedSharingSettings")->isVisible());
    QVERIFY(!window.findChild<QWidget *>("serverDiscoveryControls")->isVisible());
    QVERIFY(!window.findChild<QWidget *>("groupBox")->isVisible());
    window.updateModeControls(); // A normal controller refresh must not leak setup controls back into diagnostics.
    QVERIFY(!window.findChild<QWidget *>("serverDiscoveryControls")->isVisible());
    QTest::mouseClick(window.findChild<QPushButton *>("backToComputers"), Qt::LeftButton);
    QCOMPARE(window.m_homePages->currentWidget(), static_cast<QWidget *>(window.m_home));
  }
  void addComputerShowsExistingPreparationWithoutChangingExposure()
  {
    MainWindow window;
    seed(window);
    Settings::save(false);
    const auto before = bytes(Settings::settingsFile());
    auto *advertise = window.findChild<QCheckBox *>("advertiseNearbyServer");
    QVERIFY(advertise);
    const bool previouslyEnabled = advertise->isChecked();
    window.m_home->findChild<QPushButton *>("homeFind")->click();
    QVERIFY(!QApplication::activeModalWidget());
    QCOMPARE(window.m_homePages->currentWidget(), window.m_connectionOptionsPage);
    QVERIFY(window.findChild<QWidget *>("serverDiscoveryControls")->isVisible());
    QVERIFY(window.findChild<QLabel *>("connectionPreparation")->isVisible());
    QVERIFY(!window.findChild<QWidget *>("groupBox")->isVisible());
    QCOMPARE(advertise->isChecked(), previouslyEnabled);
    QCOMPARE(bytes(Settings::settingsFile()), before);
    QVERIFY(!window.m_coreProcess.isStarted());
    auto *back = window.findChild<QPushButton *>("backToComputers");
    back->setFocus();
    QTest::keyClick(back, Qt::Key_Space);
    QCOMPARE(window.m_homePages->currentWidget(), static_cast<QWidget *>(window.m_home));
  }
  void recoveryCancelReturnsToSelectedSavedDevice()
  {
    MainWindow window;
    seed(window);
    auto &bindings = window.m_identityBindings;
    const QString fp(64, 'b');
    const auto g = bindings.begin({"192.168.50.20", 34800});
    bindings.observe(g, fp);
    QVERIFY(bindings.confirm(g, "Receiver", true));
    QVERIFY(bindings.authenticated(g, fp, true, true));
    const auto stored = bindings.encode();
    bool returned = false;
    QTimer::singleShot(0, &window, [&] {
      auto *devices = qobject_cast<TrustedDevicesDialog *>(QApplication::activeModalWidget());
      QVERIFY(devices);
      const auto close = qScopeGuard([devices] { devices->reject(); });
      QVERIFY(devices->findChild<QLabel *>("trustedConnectionHint")->text().contains("provides keyboard"));
      QTimer::singleShot(0, devices, [&] {
        auto *recovery = qobject_cast<AutomaticRecoveryDialog *>(QApplication::activeModalWidget());
        QVERIFY(recovery);
        const auto closeRecovery = qScopeGuard([recovery] {
          if (recovery->isVisible())
            recovery->reject();
        });
        QCOMPARE(recovery->parentWidget(), static_cast<QWidget *>(devices));
        recovery->activateWindow();
        QTest::qWait(1);
        QSignalSpy cancelled(recovery, &QDialog::rejected);
        QTest::keyClick(recovery, Qt::Key_Escape);
        QCOMPARE(cancelled.count(), 1);
      });
      auto *automatic = devices->findChild<QPushButton *>("configureAutomaticRecovery");
      automatic->setFocus();
      QTest::keyClick(automatic, Qt::Key_Space);
      QVERIFY(devices->isVisible());
      QCOMPARE(devices->findChild<QListWidget *>("trustedDeviceList")->currentRow(), 0);
      returned = true;
      const auto output = qEnvironmentVariable("DESKFLOW_UI_TEST_SCREENSHOTS");
      if (!output.isEmpty())
        QVERIFY(devices->grab().save(output + "/saved-device-after-recovery-cancel.png"));
    });
    window.findChild<QPushButton *>("savedDevicesButton")->click();
    QVERIFY(returned);
    QCOMPARE(bindings.encode(), stored);
    QVERIFY(!window.m_coreProcess.isStarted());
  }
  void cancelAndSaveUseRealWindowAndAtomicConfig()
  {
    MainWindow window;
    seed(window);
    Settings::save();
    const auto before = bytes(Settings::settingsFile());
    auto *view = window.m_desktopWorkspace;
    const auto state = view->workspace().state().encode();
    QTest::mouseClick(view->findChild<QPushButton *>("workspaceEdit"), Qt::LeftButton);
    auto *selection = view->findChild<QComboBox *>("workspaceSelection");
    selection->setCurrentIndex(selection->findData("Receiver/r"));
    QTest::keyClick(view->canvas(), Qt::Key_Down);
    QVERIFY(view->workspace().state().encode() != state);
    QTest::mouseClick(view->findChild<QPushButton *>("workspaceCancel"), Qt::LeftButton);
    QCOMPARE(bytes(Settings::settingsFile()), before);
    QCOMPARE(view->workspace().state().encode(), state);
    QTest::mouseClick(view->findChild<QPushButton *>("workspaceEdit"), Qt::LeftButton);
    QTest::keyClick(view->canvas(), Qt::Key_Down);
    const auto edited = view->workspace().state().encode();
    auto *apply = view->findChild<QPushButton *>("workspaceApply");
    QVERIFY(apply->isEnabled());
    QTest::mouseClick(apply, Qt::LeftButton);
    auto saved = *WorkspaceState::decode(edited);
    ++saved.revision;
    QCOMPARE(window.m_serverConfig.workspaceState(), saved.encode());
    QVERIFY(view->findChild<QLabel *>("workspaceActiveRouting")->text().contains("not yet marked active"));
    QProcess reader;
    reader.start(QCoreApplication::applicationFilePath(), {"--read-workspace", Settings::settingsFile()});
    QVERIFY(reader.waitForFinished(15000));
    QCOMPARE(reader.exitCode(), 0);
    QCOMPARE(reader.readAllStandardOutput().trimmed(), saved.encode());
  }
  void realWindowSaveOnlyBecomesAppliedAfterExactCurrentCoreResult()
  {
    MainWindow window;
    seed(window);
    auto &core = window.m_coreProcess;
    core.m_startRequested = true;
    core.m_recoveryBlocked = false;
    core.m_processState = deskflow::core::ProcessState::Started;
    auto *ipc = new MemoryCoreIpc(&core);
    core.m_coreIpcClient = ipc;
    core.bindCoreIpcClient(ipc);
    core.m_workspaceLive = window.m_workspaceSnapshot;
    auto *view = window.m_desktopWorkspace;
    QTest::mouseClick(view->findChild<QPushButton *>("workspaceEdit"), Qt::LeftButton);
    QTest::mouseClick(view->findChild<QPushButton *>("workspaceApply"), Qt::LeftButton);
    const auto request = WorkspaceApplyRequest::decode(ipc->last);
    QVERIFY(request);
    QVERIFY(view->findChild<QLabel *>("workspaceActiveRouting")->text().contains("not yet marked active"));
    QVERIFY(validateWorkspaceApply(*request, *core.m_workspaceLive, window.m_serverConfig.workspaceState()).isEmpty());
    WorkspaceApplyResult reply{request->id, request->session,    workspacePlanId(request->state),
                               {},          request->generation, request->state.revision,
                               true};
    auto wrong = reply;
    wrong.planId = QString(64, '0');
    Q_EMIT ipc->commandReceived("workspaceApplyResult", QString::fromUtf8(wrong.encode()));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(core.m_workspacePending);
    Q_EMIT ipc->commandReceived("workspaceApplyResult", QString::fromUtf8(reply.encode()));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(!core.m_workspacePending);
    QCOMPARE(
        view->findChild<QLabel *>("workspaceActiveRouting")->text(),
        QString("The current sharing core is using the last confirmed arrangement.")
    );
    QCOMPARE(Settings::value(Settings::Server::WorkspaceApplied).toByteArray(), request->state.encode());
    QProcess reader;
    reader.start(QCoreApplication::applicationFilePath(), {"--read-applied", Settings::settingsFile()});
    QVERIFY(reader.waitForFinished(15000));
    QCOMPARE(reader.exitCode(), 0);
    QCOMPARE(reader.readAllStandardOutput().trimmed(), request->state.encode());
  }
  void realWindowRejectedOrDisconnectedApplyPreservesSavedDraft()
  {
    MainWindow window;
    seed(window);
    auto &core = window.m_coreProcess;
    core.m_startRequested = true;
    core.m_processState = deskflow::core::ProcessState::Started;
    core.m_workspaceLive = window.m_workspaceSnapshot;
    auto *ipc = new MemoryCoreIpc(&core);
    core.m_coreIpcClient = ipc;
    core.bindCoreIpcClient(ipc);
    auto *view = window.m_desktopWorkspace;
    view->beginEdit();
    QTest::mouseClick(view->findChild<QPushButton *>("workspaceApply"), Qt::LeftButton);
    auto connected = window.m_home->state();
    connected.stage = ConnectionHome::Stage::Connected;
    connected.peerName = "Receiver";
    connected.explanation.clear();
    auto captureStatus = [&](const QString &name) {
      window.m_home->setState(connected);
      QTest::qWait(20);
      capture(window, name);
    };
    captureStatus("state-saved-awaiting-confirmation");
    const auto first = WorkspaceApplyRequest::decode(ipc->last);
    QVERIFY(first);
    const auto saved = window.m_serverConfig.workspaceState();
    WorkspaceApplyResult reply{first->id,
                               first->session,
                               workspacePlanId(first->state),
                               "stale-topology",
                               first->generation,
                               first->state.revision,
                               false};
    Q_EMIT ipc->commandReceived("workspaceApplyResult", QString::fromUtf8(reply.encode()));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(view->findChild<QLabel *>("workspaceActiveRouting")->text().contains("rejected"));
    QCOMPARE(window.m_serverConfig.workspaceState(), saved);
    captureStatus("state-save-confirmation-rejected");
    view->beginEdit();
    QTest::mouseClick(view->findChild<QPushButton *>("workspaceApply"), Qt::LeftButton);
    const auto second = WorkspaceApplyRequest::decode(ipc->last);
    QVERIFY(second);
    const auto retained = window.m_serverConfig.workspaceState();
    core.invalidatePendingEvents();
    QVERIFY(view->findChild<QLabel *>("workspaceActiveRouting")->text().contains("unknown"));
    captureStatus("state-save-confirmation-unknown");
    view->beginEdit();
    QTest::keyClick(view->canvas(), Qt::Key_Down);
    const auto newerDraft = view->workspace().state().encode();
    QVERIFY(newerDraft != retained);
    reply = {second->id, second->session,    workspacePlanId(second->state),
             {},         second->generation, second->state.revision,
             true};
    Q_EMIT ipc->commandReceived("workspaceApplyResult", QString::fromUtf8(reply.encode()));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(view->findChild<QLabel *>("workspaceActiveRouting")->text().contains("unknown"));
    QCOMPARE(window.m_serverConfig.workspaceState(), retained);
    QCOMPARE(view->workspace().state().encode(), newerDraft);
    QVERIFY(view->findChild<QPushButton *>("workspaceCancel")->isVisible());
    captureStatus("state-late-ack-retains-new-draft");
  }
  void receiverHomeCannotEditOrPresentImaginaryAppliedLayout()
  {
    Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::Client);
    Settings::setValue(Settings::Client::RemoteHost, "controller.invalid");
    MainWindow window;
    window.resize(760, 700);
    window.show();
    QApplication::processEvents();
    auto *view = window.m_desktopWorkspace;
    QVERIFY(view->isVisible());
    QVERIFY(view->findChild<QLabel *>("workspaceReceiverSummary")->isVisible());
    for (const auto *name :
         {"workspaceEdit", "workspaceApply", "workspaceAdd", "workspaceIdentify", "workspaceDetails"})
      QVERIFY(!view->findChild<QPushButton *>(name)->isVisible());
    QSignalSpy save(view, &DesktopWorkspaceView::applyRequested);
    window.showDesktopArrangement();
    view->beginEdit();
    QCOMPARE(save.count(), 0);
    QVERIFY(!view->findChild<QPushButton *>("workspaceEdit")->isVisible());
    QVERIFY(view->findChild<QLabel *>("workspaceActiveRouting")->y() < 100);
    QVERIFY(view->findChild<QLabel *>("workspaceReceiverSummary")->y() < 160);
    capture(window, "receiver-home");
  }
  void receiverShowsOnlyScreensInTheRealAppliedPlan()
  {
    Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::Client);
    Settings::setValue(Settings::Client::RemoteHost, "controller.invalid");
    MainWindow window;
    window.resize(800, 750);
    window.show();
    QApplication::processEvents();
    WorkspaceSnapshot snapshot;
    snapshot.devices = {"Controller", "Receiver", "Unapplied"};
    snapshot.onlineDevices = snapshot.devices;
    snapshot.topologies = {{"Controller", localScreens()}, {"Receiver", remoteScreen()}, {"Unapplied", remoteScreen()}};
    DeviceSnapshot local;
    local.id = "Controller";
    local.local = true;
    local.topology = localScreens();
    local.observation = Observation::Observed;
    DesktopWorkspace actual({local});
    snapshot.applied = actual.state();
    snapshot.perDisplayActive = true;
    window.m_workspaceSnapshot = snapshot;
    window.m_workspaceLive = true;
    window.m_workspaceSourceFingerprint = QString(64, 'a');
    window.refreshDesktopWorkspace();
    auto *view = window.m_desktopWorkspace;
    QCOMPARE(view->workspace().displays().size(), 2);
    for (const auto &screen : view->workspace().displays())
      QCOMPARE(screen.deviceId, QString("Controller"));
    QVERIFY(!view->findChild<QPushButton *>("workspaceEdit")->isVisible());
    QTest::mouseClick(view->findChild<QPushButton *>("workspaceDetails"), Qt::LeftButton);
    QVERIFY(view->canvas()->isVisible());
    capture(window, "receiver-applied-readonly");
  }
  void actualChinesePageFitsLongTextDarkAndLight_data()
  {
    QTest::addColumn<bool>("dark");
    QTest::newRow("light") << false;
    QTest::newRow("dark") << true;
  }
  void actualChinesePageFitsLongTextDarkAndLight()
  {
    QFETCH(bool, dark);
    QTranslator translator;
    QVERIFY(translator.load(":/translations/deskflow_zh_CN.qm"));
    const auto before = qApp->palette();
    qApp->installTranslator(&translator);
    const auto restore = qScopeGuard([&] {
      qApp->removeTranslator(&translator);
      qApp->setPalette(before);
    });
    if (dark) {
      // Use a coherent Fusion dark palette. Changing only Window would leave
      // native menu/button text using the light fixture's black foreground.
      auto palette = before;
      for (const auto role : {QPalette::Window, QPalette::Button})
        palette.setColor(role, QColor("#202124"));
      palette.setColor(QPalette::Base, QColor("#181b22"));
      for (const auto role : {QPalette::WindowText, QPalette::ButtonText, QPalette::Text})
        palette.setColor(role, QColor("#f5f5f7"));
      qApp->setPalette(palette);
    }
    MainWindow window;
    seed(window);
    window.resize(620, 820);
    auto state = window.m_home->state();
    state.stage = ConnectionHome::Stage::Connected;
    state.peerName = QString::fromUtf8("用于确认混合缩放和超长中文名称完整显示的办公电脑");
    state.explanation = state.peerName;
    window.m_home->setState(state);
    QCOMPARE(window.findChild<QPushButton *>("homePrimary")->text(), QString::fromUtf8("断开连接"));
    for (int points : {12, 18, 24}) {
      auto *primary = window.findChild<QPushButton *>("homePrimary");
      auto font = primary->font();
      font.setPointSize(points);
      primary->setFont(font);
      window.m_home->setState(state);
      QApplication::processEvents();
      for (auto *button : window.findChildren<QPushButton *>())
        if (button->isVisible()) {
          QStyleOptionButton option;
          option.initFrom(button);
          option.text = button->text();
          option.rect = button->rect();
          const auto box = button->style()->subElementRect(QStyle::SE_PushButtonContents, &option, button);
          QVERIFY2(
              box.width() + .5 >= button->fontMetrics().horizontalAdvance(button->text()),
              qPrintable(button->objectName() + ':' + button->text())
          );
        }
    }
    capture(window, dark ? "controller-zh-dark" : "controller-zh-light");
    if (dark) {
      auto *canvas = window.m_desktopWorkspace->canvas();
      const auto pixels = canvas->grab().toImage();
      const auto scale = pixels.devicePixelRatio();
      int light = 0, darkPixels = 0;
      for (int y = 0; y < pixels.height(); ++y)
        for (int x = 0; x < pixels.width(); ++x) {
          const auto color = pixels.pixelColor(x, y);
          if (color.lightness() > 180)
            ++light;
          if (color.lightness() < 100)
            ++darkPixels;
        }
      QVERIFY(light > 20 * scale);
      QVERIFY(darkPixels > pixels.width() * pixels.height() * 0.75);
    }
    bool settingsOpened = false;
    QTimer::singleShot(20, &window, [&] {
      auto *dialog = qobject_cast<SettingsDialog *>(QApplication::activeModalWidget());
      QVERIFY(dialog);
      const auto close = qScopeGuard([dialog] {
        if (dialog->isVisible())
          dialog->reject();
      });
      settingsOpened = true;
      dialog->activateWindow();
      auto *navigation = dialog->findChild<QListWidget *>("settingsNavigation");
      navigation->setFocus();
      QTest::qWait(1);
      QSignalSpy rejected(dialog, &QDialog::rejected);
      QTest::keyClick(navigation, Qt::Key_Escape);
      QCOMPARE(rejected.count(), 1);
    });
    auto *settings = window.findChild<QPushButton *>("homeSettings");
    settings->setFocus(Qt::TabFocusReason);
    QTest::keyClick(settings, Qt::Key_Space);
    window.activateWindow();
    QApplication::processEvents();
    QVERIFY(settingsOpened);
    QVERIFY(settings->hasFocus());
    QCOMPARE(window.m_homePages->currentWidget(), static_cast<QWidget *>(window.m_home));
  }
  void realProductButtonsFitAtMinimumWidthAndLargeFont()
  {
    MainWindow window;
    seed(window);
    window.resize(620, 820);
    auto state = window.m_home->state();
    state.stage = ConnectionHome::Stage::Connected;
    state.explanation = QString::fromUtf8("已连接到用于混合缩放和长中文设备名称回归的电脑");
    window.m_home->setState(state);
    for (int points : {12, 18, 24}) {
      auto *primary = window.findChild<QPushButton *>("homePrimary");
      auto font = primary->font();
      font.setPointSize(points);
      primary->setFont(font);
      window.m_home->setState(state);
      QApplication::processEvents();
      for (auto *button : window.findChildren<QPushButton *>())
        if (button->isVisible()) {
          QStyleOptionButton option;
          option.initFrom(button);
          option.text = button->text();
          option.rect = button->rect();
          const auto box = button->style()->subElementRect(QStyle::SE_PushButtonContents, &option, button);
          QVERIFY2(
              box.width() + .5 >= button->fontMetrics().horizontalAdvance(button->text()),
              qPrintable(button->objectName() + ':' + button->text())
          );
        }
    }
    capture(window, "controller-narrow-large-font");
  }
};
int main(int argc, char **argv)
{
  const auto allowed = qEnvironmentVariable("DESKFLOW_TEST_ROOT");
  if (allowed.isEmpty() || !QDir(allowed).isAbsolute() || !QDir(allowed).exists())
    return 2;
  qputenv("XDG_CONFIG_HOME", (allowed + "/config").toUtf8());
  qputenv("XDG_STATE_HOME", (allowed + "/state").toUtf8());
  QDir().mkpath(allowed + "/config/DeskflowLab");
  QDir().mkpath(allowed + "/state");
  QApplication app(argc, argv);
  app.setApplicationName("DeskflowLab");
  app.setStyle("Fusion");
  if (!prepareWindowsOffscreenFont(app))
    return 6;
  if (argc == 3 &&
      (QString::fromLocal8Bit(argv[1]) == "--read-workspace" || QString::fromLocal8Bit(argv[1]) == "--read-applied")) {
    const auto path = QString::fromLocal8Bit(argv[2]);
    if (QFileInfo(path).absolutePath() != allowed)
      return 3;
    Settings::setSettingsFile(path);
    MainWindow window;
    const auto state = QString::fromLocal8Bit(argv[1]) == "--read-applied"
                           ? Settings::value(Settings::Server::WorkspaceApplied).toByteArray()
                           : window.serverConfig().workspaceState();
    if (!WorkspaceState::decode(state))
      return 4;
    std::fwrite(state.constData(), 1, state.size(), stdout);
    return 0;
  }
  DesktopProductFlowTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "DesktopProductFlowTests.moc"
