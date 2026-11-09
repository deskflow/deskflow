// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/CandidatePolicy.h"
#include "common/Settings.h"
#include "gui/MainWindow.h"
#include "gui/core/NearbyDiscovery.h"
#include "gui/dialogs/AutomaticRecoveryDialog.h"
#include "gui/dialogs/DesktopLayoutEditor.h"
#include "gui/dialogs/NearbyDevicesDialog.h"
#include "gui/dialogs/PairingReviewDialog.h"
#include "gui/dialogs/ServerConfigDialog.h"
#include "gui/dialogs/SettingsDialog.h"
#include "gui/dialogs/TrustedDevicesDialog.h"
#include "gui/widgets/ConnectionDiagnostics.h"
#include "gui/widgets/SettingsDialogButtonBox.h"
#include <QAbstractButton>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QGroupBox>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyleOptionViewItem>
#include <QTextLayout>
#include <QTranslator>
using deskflow::gui::NearbyDevicesDialog;
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QToolButton>
class FullWindowTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void tlsKeyLengthRemainsDraftUntilSave_data()
  {
    QTest::addColumn<QString>("action");
    QTest::newRow("cancel") << QString("cancel");
    QTest::newRow("reset") << QString("reset");
    QTest::newRow("save") << QString("save");
  }
  void tlsKeyLengthRemainsDraftUntilSave()
  {
#ifdef DESKFLOW_UI_REVIEW_BUILD
    QSKIP("This regression exercises the production save path in the isolated Lab build.");
#endif
    QFETCH(QString, action);
    Settings::setValue(Settings::Security::KeySize, 2048);
    Settings::save(false);
    const auto certificate = Settings::value(Settings::Security::Certificate);
    const auto tls = Settings::value(Settings::Security::TlsEnabled);
    const auto peers = Settings::value(Settings::Security::CheckPeers);
    ServerConfig config;
    SettingsDialog dialog(nullptr, config);
    auto *keys = dialog.findChild<QComboBox *>("comboTlsKeyLength");
    QVERIFY(keys);
    const auto index = keys->findText("4096");
    QVERIFY(index >= 0);
    keys->setCurrentIndex(index);
    QCOMPARE(Settings::value(Settings::Security::KeySize).toInt(), 2048);
    auto *buttons = dialog.findChild<SettingsDialogButtonBox *>()->findChild<QDialogButtonBox *>();
    QVERIFY(buttons->button(QDialogButtonBox::Save)->isEnabled());
    if (action == "save") {
      buttons->button(QDialogButtonBox::Save)->click();
      QCOMPARE(Settings::value(Settings::Security::KeySize).toInt(), 4096);
    } else {
      if (action == "reset") {
        buttons->button(QDialogButtonBox::Reset)->click();
        QCOMPARE(keys->currentText(), QString("2048"));
      }
      dialog.reject();
      QCOMPARE(Settings::value(Settings::Security::KeySize).toInt(), 2048);
    }
    QCOMPARE(Settings::value(Settings::Security::Certificate), certificate);
    QCOMPARE(Settings::value(Settings::Security::TlsEnabled), tls);
    QCOMPARE(Settings::value(Settings::Security::CheckPeers), peers);
    Settings::save(false);
    QSettings saved(Settings::settingsFile(), QSettings::IniFormat);
    QCOMPARE(saved.value(Settings::Security::KeySize, 2048).toInt(), action == "save" ? 4096 : 2048);
    QVERIFY(Settings::tlsDir().startsWith(qEnvironmentVariable("DESKFLOW_TEST_ROOT")));
    QVERIFY(QDir(Settings::tlsDir()).entryList(QDir::Files | QDir::NoDotAndDotDot).isEmpty());
  }
  void manualConnectionRevealsAndFocusesTheAddressWithoutStarting()
  {
    MainWindow w;
    w.open();
    QVERIFY(w.labScenario("empty"));
    QTest::qWait(30);
    auto *toggle = w.findChild<QToolButton *>("connectionOptionsToggle");
    auto *input = w.findChild<QLineEdit *>("lineHostname");
    auto *card = w.findChild<ConnectionDiagnostics *>();
    auto *action = card->findChild<QPushButton *>("diagnosticAction");
    auto *scroll = w.findChild<QScrollArea *>("mainContentScroll");
    for (int i = 0; i < 3; ++i) {
      toggle->setChecked(false);
      QTest::qWait(10);
      QVERIFY(!input->isVisible());
      action->setFocus();
      QTest::keyClick(action, Qt::Key_Space);
      QTest::qWait(30);
      QVERIFY(toggle->isChecked());
      QVERIFY(input->isVisible());
      QVERIFY(input->hasFocus());
      QVERIFY(scroll->viewport()->rect().contains(QRect(input->mapTo(scroll->viewport(), QPoint()), input->size())));
      QCOMPARE(card->phase(), ConnectionDiagnostics::Phase::Idle);
      QVERIFY(!Settings::value(Settings::Gui::AutoStartCore).toBool());
    }
  }
  void serverDiscoveryAndErrorSettingsStayOnTheFirstScreen()
  {
    MainWindow w;
    w.open();
    w.findChild<QRadioButton *>("rbModeServer")->setChecked(true);
    QVERIFY(!w.findChild<QPushButton *>("diagnosticAction")->isVisible());
    QVERIFY(w.labScenario("listening"));
    QTest::qWait(40);
    w.resize(800, 600);
    QTest::qWait(20);
    auto *scroll = w.findChild<QScrollArea *>("mainContentScroll");
    for (const auto *id : {"advertiseNetwork", "advertiseNearbyServer"}) {
      auto *control = w.findChild<QWidget *>(id);
      QVERIFY(control->isVisible());
      QVERIFY(w.rect().contains(QRect(control->mapTo(&w, QPoint()), control->size())));
      QVERIFY(!scroll->isAncestorOf(control));
    }
    w.findChild<QRadioButton *>("rbModeClient")->setChecked(true);
    QVERIFY(w.labScenario("long-error"));
    QTest::qWait(40);
    w.resize(640, 480);
    QTest::qWait(20);
    auto *toggle = w.findChild<QToolButton *>("connectionOptionsToggle");
    QVERIFY(scroll->viewport()->rect().contains(QRect(toggle->mapTo(scroll->viewport(), QPoint()), toggle->size())));
    QVERIFY(toggle->mapTo(&w, QPoint()).y() < w.findChild<ConnectionDiagnostics *>()->mapTo(&w, QPoint()).y());
  }
  void slimScrollbarRetainsKeyboardAndDrag()
  {
    MainWindow w;
    w.open();
    QVERIFY(w.labScenario("long-error"));
    QTest::qWait(30);
    w.findChild<QToolButton *>("connectionOptionsToggle")->setChecked(true);
    QTest::qWait(20);
    w.resize(640, 320);
    QTest::qWait(30);
    auto *bar = w.findChild<QScrollArea *>("mainContentScroll")->verticalScrollBar();
    QVERIFY(bar->maximum() > 0);
    QVERIFY(bar->width() <= 14);
    bar->setFocus();
    bar->setValue(0);
    QTest::keyClick(bar, Qt::Key_End);
    QCOMPARE(bar->value(), bar->maximum());
    QTest::keyClick(bar, Qt::Key_Home);
    QCOMPARE(bar->value(), 0);
    QTest::mousePress(bar, Qt::LeftButton, Qt::NoModifier, QPoint(bar->width() / 2, 10));
    QTest::mouseMove(bar, QPoint(bar->width() / 2, bar->height() - 12), 20);
    QTest::mouseRelease(bar, Qt::LeftButton, Qt::NoModifier, QPoint(bar->width() / 2, bar->height() - 12));
    QVERIFY(bar->value() > 0);
  }
  void layoutCanvasAndSelectorKeepSeparateFocusBounds()
  {
    MainWindow window;
    window.open();
    DesktopLayoutEditor editor(
        {{"Lab-Mac", 0, 0, 1920, 1080, "这台 Mac"}, {"Work-PC", 1920, 200, 2560, 1440, "工作电脑"}}, &window
    );
    auto *dialog = &editor;
    dialog->show();
    QTest::qWait(20);
    auto *canvas = dialog->findChild<QWidget *>("layoutCanvas");
    auto *combo = dialog->findChild<QComboBox *>("layoutComputer");
    QVERIFY(canvas);
    QVERIFY(combo);
    combo->setFocus();
    const auto gap = combo->mapTo(dialog, QPoint()).y() - (canvas->mapTo(dialog, QPoint()).y() + canvas->height());
    QVERIFY2(
        gap >= 12,
        qPrintable(QString("Canvas/selector focus gap: %1; window=%2; minimumHint=%3; canvasY=%4 height=%5 comboY=%6")
                       .arg(gap)
                       .arg(dialog->height())
                       .arg(dialog->minimumSizeHint().height())
                       .arg(canvas->mapTo(dialog, QPoint()).y())
                       .arg(canvas->height())
                       .arg(combo->mapTo(dialog, QPoint()).y()))
    );
  }
  void discoveryDurationHasVisibleBuddyAndErrorHelpIsRelevant()
  {
    MainWindow window;
    window.show();
    QCoreApplication::processEvents();
    auto *duration = window.findChild<QComboBox *>("advertiseDuration");
    auto *label = window.findChild<QLabel *>("advertiseDurationLabel");
    QVERIFY(duration);
    QVERIFY(label);
    QCOMPARE(label->buddy(), duration);
    QVERIFY(!label->text().isEmpty());
    auto *card = window.findChild<ConnectionDiagnostics *>("connectionDiagnostics");
    QVERIFY(card);
    card->beginAttempt(false, "fixture.local", 34800, true, true);
    card->processStateChanged(deskflow::core::ProcessState::Started);
    card->connectionRefused(deskflow::core::ConnectionRefusal::ProtocolError);
    const auto text = window.findChild<QLabel *>("diagnosticDetails")->text();
    QVERIFY(!text.contains("Spaces"));
    QVERIFY(!text.contains("pointer"));
  }

  void init()
  {
    Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::Client);
    Settings::setValue(Settings::Gui::LogExpanded, false);
    Settings::setValue(Settings::Gui::AutoStartCore, false);
  }
  void explicitPauseSurvivesWindowRecreation()
  {
    Settings::setValue(Settings::Gui::AutoStartCore, true);
    Settings::setValue(deskflow::candidate::pausedKey, false);
    {
      MainWindow w;
      w.open();
      w.labUserStop();
    }
    QVERIFY(Settings::value(deskflow::candidate::pausedKey).toBool());
    QVERIFY(!Settings::value(Settings::Gui::AutoStartCore).toBool());
    {
      MainWindow w;
      w.open();
      QVERIFY(Settings::value(deskflow::candidate::pausedKey).toBool());
      w.labUserStart();
    }
    QVERIFY(!Settings::value(deskflow::candidate::pausedKey).toBool());
  }
  void identityRejectionOpensRealReviewWithoutChangingPinOrAddress()
  {
    MainWindow w;
    w.open();
    QVERIFY(w.labScenario("identity-changed"));
    QTest::qWait(20);
    auto *dialog = w.findChild<PairingReviewDialog *>();
    QVERIFY(dialog);
    QVERIFY(dialog->isVisible());
    const auto saved = Settings::value("gui/identityBindings").toByteArray();
    QVERIFY(saved.contains("192.168.50.20"));
    QVERIFY(!saved.contains("192.168.50.30"));
    auto *confirm = dialog->findChild<QPushButton *>("pairingConfirm");
    QVERIFY(!confirm->isVisible());
    QTest::keyClick(dialog, Qt::Key_Escape);
    QVERIFY(!dialog->isVisible());
    QCOMPARE(Settings::value("gui/identityBindings").toByteArray(), saved);
    QCOMPARE(w.findChild<ConnectionDiagnostics *>()->phase(), ConnectionDiagnostics::Phase::NeedsAction);
  }
  void discoveryRecoveryRunsThroughRealWindowPersistence_data()
  {
    QTest::addColumn<QString>("scenario");
    QTest::addColumn<bool>("verified");
    QTest::newRow("same-identity") << QString("recover-device") << true;
    QTest::newRow("forged-candidate") << QString("recover-rejected") << false;
    QTest::newRow("worker-unavailable") << QString("recover-unavailable") << false;
  }
  void discoveryRecoveryRunsThroughRealWindowPersistence()
  {
    QFETCH(QString, scenario);
    QFETCH(bool, verified);
    Settings::setValue(Settings::Security::TlsEnabled, true);
    Settings::setValue(Settings::Security::CheckPeers, true);
    MainWindow w;
    w.open();
    QVERIFY(w.labScenario(scenario));
    QVERIFY(Settings::value("gui/identityBindings").toByteArray().contains("old.local"));
    QTest::qWait(60);
    deskflow::gui::IdentityBindings stored;
    QVERIFY(stored.load(Settings::value("gui/identityBindings").toByteArray()));
    QCOMPARE(
        stored.devices().value(QString(64, 'a')).endpoint.host,
        verified ? QString("192.168.50.30") : QString("old.local")
    );
    QCOMPARE(
        w.findChild<QLineEdit *>("lineHostname")->text(), verified ? QString("192.168.50.30") : QString("old.local")
    );
    QVERIFY(w.findChild<ConnectionDiagnostics *>()->phase() != ConnectionDiagnostics::Phase::Connected);
    if (!verified) {
      auto *dialog = w.findChild<NearbyDevicesDialog *>();
      QVERIFY(dialog);
      QVERIFY(dialog->isVisible());
      auto *select = dialog->findChild<QPushButton *>("selectNearbyAddress");
      QVERIFY(!select->isVisible());
      dialog->resize(540, 360);
      QTest::qWait(10);
      for (auto *button : dialog->findChildren<QAbstractButton *>())
        if (button->isVisible()) {
          const QRect bounds(button->mapTo(dialog, QPoint()), button->size());
          QVERIFY(dialog->rect().contains(bounds));
          QVERIFY(button->height() >= button->sizeHint().height());
        }
      for (auto *label : dialog->findChildren<QLabel *>())
        if (label->isVisible() && label->wordWrap()) {
          QTextLayout text(label->text(), label->font());
          QTextOption option;
          option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
          text.setTextOption(option);
          text.beginLayout();
          qreal height = 0;
          while (true) {
            auto line = text.createLine();
            if (!line.isValid())
              break;
            line.setLineWidth(label->contentsRect().width());
            height += line.height();
          }
          text.endLayout();
          QVERIFY2(label->height() + 2 >= height, qPrintable("Recovery text clipped: " + label->objectName()));
        }
      QTest::keyClick(dialog, Qt::Key_Escape);
      QCOMPARE(Settings::value("gui/identityBindings").toByteArray(), stored.encode());
    }
  }
  void automaticRecoveryUsesIntegratedSignalsAndPersistence_data()
  {
    QTest::addColumn<QString>("scenario");
    QTest::addColumn<bool>("resumed");
    QTest::newRow("verified-address") << QString("automatic-recovered") << true;
    QTest::newRow("user-paused") << QString("automatic-paused") << false;
  }
  void automaticRecoveryUsesIntegratedSignalsAndPersistence()
  {
    QFETCH(QString, scenario);
    QFETCH(bool, resumed);
    Settings::setValue(Settings::Core::ProcessMode, Settings::ProcessMode::Desktop);
    Settings::setValue(Settings::Security::TlsEnabled, true);
    Settings::setValue(Settings::Security::CheckPeers, true);
    MainWindow w;
    w.open();
    QVERIFY(w.labScenario(scenario));
    QTest::qWait(80);
    deskflow::gui::IdentityBindings saved;
    QVERIFY(saved.load(Settings::value("gui/identityBindings").toByteArray()));
    QCOMPARE(
        saved.devices().value(QString(64, 'a')).endpoint.host, resumed ? QString("192.168.50.30") : QString("old.local")
    );
    QCOMPARE(
        w.findChild<QLineEdit *>("lineHostname")->text(), resumed ? QString("192.168.50.30") : QString("old.local")
    );
    QVERIFY(w.findChild<ConnectionDiagnostics *>()->phase() != ConnectionDiagnostics::Phase::Connected);
    QCOMPARE(Settings::value("lab/automaticState").toInt(), resumed ? 8 : 1);
    if (!resumed)
      QCOMPARE(w.findChild<ConnectionDiagnostics *>()->phase(), ConnectionDiagnostics::Phase::Paused);
  }
  void recoveryActionOpensSavedDeviceSettings()
  {
    Settings::setValue(Settings::Core::ProcessMode, Settings::ProcessMode::Desktop);
    Settings::setValue(Settings::Security::TlsEnabled, true);
    Settings::setValue(Settings::Security::CheckPeers, true);
    MainWindow w;
    w.open();
    QVERIFY(w.labScenario("automatic-recovered"));
    QTest::qWait(80);
    bool opened = false;
    QTimer::singleShot(0, &w, [&] {
      auto *dialog = qobject_cast<TrustedDevicesDialog *>(QApplication::activeModalWidget());
      opened = dialog != nullptr;
      if (dialog)
        dialog->reject();
    });
    w.findChild<QPushButton *>("diagnosticAction")->click();
    QVERIFY(opened);
  }
  void automaticRecoveryFormGeometryAndKeyboard()
  {
    MainWindow w;
    w.open();
    QVERIFY(w.labScenario("automatic-config"));
    QTest::qWait(20);
    auto *d = w.findChild<AutomaticRecoveryDialog *>();
    QVERIFY(d);
    d->resize(500, 320);
    QTest::qWait(10);
    auto *enabled = d->findChild<QCheckBox *>("automaticRecoveryEnabled");
    enabled->setFocus();
    QVERIFY(enabled->hasFocus());
    QTest::keyClick(enabled, Qt::Key_Space);
    QVERIFY(!enabled->isChecked());
    QTest::keyClick(enabled, Qt::Key_Space);
    QVERIFY(enabled->isChecked());
    for (auto *button : d->findChildren<QAbstractButton *>())
      if (button->isVisible()) {
        QVERIFY(d->rect().contains(QRect(button->mapTo(d, QPoint()), button->size())));
        QVERIFY(button->height() >= button->sizeHint().height());
      }
    for (auto *label : d->findChildren<QLabel *>())
      if (label->isVisible() && label->wordWrap()) {
        QTextLayout text(label->text(), label->font());
        QTextOption option;
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        text.setTextOption(option);
        text.beginLayout();
        qreal height = 0;
        while (true) {
          auto line = text.createLine();
          if (!line.isValid())
            break;
          line.setLineWidth(label->contentsRect().width());
          height += line.height();
        }
        text.endLayout();
        QVERIFY2(
            label->contentsRect().height() + 2 >= height,
            qPrintable("Automatic recovery text clipped: " + label->objectName())
        );
      }
    const auto before = Settings::value("gui/automaticRecovery");
    QTest::keyClick(d, Qt::Key_Escape);
    QCOMPARE(Settings::value("gui/automaticRecovery"), before);
  }
  void transientConnectionSummaryDoesNotResizeMainWindow()
  {
    MainWindow w;
    w.open();
    w.show();
    QTest::qWait(30);
    w.resize(900, 700);
    QTest::qWait(30);
    auto *card = w.findChild<ConnectionDiagnostics *>("connectionDiagnostics");
    QVERIFY(card);
    card->beginAttempt(false, "fixture.local", 34800, true, true);
    card->processStateChanged(deskflow::core::ProcessState::Started);
    card->connectionStateChanged(deskflow::core::ConnectionState::Connecting);
    QTest::qWait(20);
    const QSize settled = w.size();
    card->secureSocket(true);
    card->connectionStateChanged(deskflow::core::ConnectionState::Disconnected);
    card->retryIn(9);
    QTest::qWait(20);
    QCOMPARE(w.size(), settled);
    for (int seconds : {8, 7, 6}) {
      card->retryIn(seconds);
      QTest::qWait(20);
      QCOMPARE(w.size(), settled);
    }
  }
  void wholeWindowGeometry_data()
  {
    QTest::addColumn<QString>("scenario");
    QTest::addColumn<int>("requestedWidth");
    QTest::addColumn<bool>("expanded");
    for (const auto *scenario :
         {"empty", "connecting", "connected", "identity", "error", "retry", "long-error", "verifying", "listening",
          "stopping", "automatic-recovered", "automatic-paused"})
      for (int width : {640, 800})
        for (bool expanded : {false, true})
          QTest::newRow(qPrintable(QString("%1-%2-%3").arg(scenario).arg(width).arg(expanded)))
              << QString(scenario) << width << expanded;
  }
  void wholeWindowGeometry()
  {
    QFETCH(QString, scenario);
    QFETCH(int, requestedWidth);
    QFETCH(bool, expanded);
    MainWindow w;
    w.open();
    QVERIFY(w.labScenario(scenario));
    w.findChild<QToolButton *>("connectionOptionsToggle")->setChecked(expanded);
    QTest::qWait(40);
    w.resize(requestedWidth, 420);
    QTest::qWait(40);
    QVERIFY2(w.width() <= requestedWidth, "The main window cannot adapt to the supported narrow width");
    const auto check = [&](QWidget *widget) {
      for (auto *parent = widget->parentWidget(); parent && parent != &w; parent = parent->parentWidget()) {
        if (qobject_cast<QScrollArea *>(parent) || parent->objectName() == "qt_scrollarea_viewport")
          break;
        const QRect inside(widget->mapTo(parent, QPoint()), widget->size());
        if (!parent->rect().contains(inside))
          return false;
      }
      return true;
    };
    for (auto *button : w.findChildren<QAbstractButton *>()) {
      if (!button->isVisible() || button->window() != &w)
        continue;
      QVERIFY2(check(button), qPrintable("Clipped button: " + button->objectName()));
      QVERIFY2(
          button->height() >= button->sizeHint().height(),
          qPrintable(QString("Button too short: %1 actual=%2 hint=%3 minimum=%4")
                         .arg(button->objectName())
                         .arg(button->height())
                         .arg(button->sizeHint().height())
                         .arg(button->minimumHeight()))
      );
    }
    for (auto *label : w.findChildren<QLabel *>()) {
      if (!label->isVisible() || label->text().isEmpty() || label->window() != &w)
        continue;
      QVERIFY2(check(label), qPrintable("Clipped label: " + label->objectName()));
      if (label->wordWrap()) {
        QTextLayout text(label->text(), label->font());
        QTextOption option;
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        text.setTextOption(option);
        text.beginLayout();
        qreal height = 0;
        while (true) {
          auto line = text.createLine();
          if (!line.isValid())
            break;
          line.setLineWidth(label->contentsRect().width());
          height += line.height();
        }
        text.endLayout();
        QVERIFY2(label->contentsRect().height() + 2 >= height, qPrintable("Text clipped: " + label->objectName()));
      }
    }
  }
  void defaultWindowShowsEntireContent_data()
  {
    QTest::addColumn<QString>("scenario");
    for (const auto *s :
         {"empty", "connected", "retry", "long-error", "listening", "automatic-recovered", "automatic-paused"})
      QTest::newRow(s) << QString(s);
  }
  void defaultWindowShowsEntireContent()
  {
    QFETCH(QString, scenario);
    MainWindow w;
    w.open();
    QVERIFY(w.labScenario(scenario));
    QTest::qWait(50);
    auto *scroll = w.findChild<QScrollArea *>("mainContentScroll");
    QVERIFY(scroll);
    QCOMPARE(scroll->verticalScrollBar()->maximum(), 0);
    QCOMPARE(scroll->horizontalScrollBar()->maximum(), 0);
    if (scenario == "listening")
      QVERIFY(!w.findChild<QPushButton *>("btnSaveServerConfig")->text().isEmpty());
  }
  void discoveryPreviewDoesNotPretendToConnect()
  {
    MainWindow w;
    w.open();
    QVERIFY(w.labScenario("discovering"));
    QTest::qWait(40);
    QCOMPARE(w.findChild<ConnectionDiagnostics *>()->phase(), ConnectionDiagnostics::Phase::Discovering);
    auto *dialog = w.findChild<NearbyDevicesDialog *>();
    QVERIFY(dialog);
    dialog->close();
    QVERIFY(!Settings::value(Settings::Gui::AutoStartCore).toBool());
  }
  void collapsedSettingsHaveNoEmptyRows()
  {
    MainWindow w;
    w.open();
    QVERIFY(w.labScenario("connected"));
    QTest::qWait(40);
    auto *group = w.findChild<QGroupBox *>("groupBox");
    QVERIFY(group);
    QVERIFY2(group->height() <= 80, "Collapsed settings kept hidden rows or excessive whitespace");
  }
  void minimumWindowKeepsAllActionsReachableByKeyboard()
  {
    MainWindow w;
    w.open();
    QVERIFY(w.labScenario("long-error"));
    QTest::qWait(40);
    w.findChild<QToolButton *>("connectionOptionsToggle")->setChecked(true);
    QTest::qWait(20);
    w.resize(640, 320);
    QTest::qWait(30);
    QCOMPARE(w.size(), QSize(640, 320));
    auto *scroll = w.findChild<QScrollArea *>("mainContentScroll");
    QVERIFY(scroll);
    QVERIFY(scroll->verticalScrollBar()->maximum() > 0);
    auto *nearby = w.findChild<QPushButton *>("nearbyComputersButton");
    auto *first = w.findChild<QPushButton *>("diagnosticDetailsButton");
    first->setFocus();
    for (int i = 0; i < 30 && !nearby->hasFocus(); ++i)
      QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
    QVERIFY(nearby->hasFocus());
    QTest::qWait(10);
    QVERIFY(w.rect().contains(QRect(nearby->mapTo(&w, QPoint()), nearby->size())));
    QVERIFY(!scroll->isAncestorOf(nearby));
    QCOMPARE(scroll->horizontalScrollBar()->maximum(), 0);
  }
  void pinnedDeviceActionsSurviveRepeatedExpansionAndScrolling()
  {
    MainWindow w;
    w.open();
    QVERIFY(w.labScenario("long-error"));
    QTest::qWait(40);
    auto *scroll = w.findChild<QScrollArea *>("mainContentScroll");
    auto *toggle = w.findChild<QToolButton *>("connectionOptionsToggle");
    auto *nearby = w.findChild<QPushButton *>("nearbyComputersButton");
    for (int i = 0; i < 4; ++i) {
      toggle->setChecked(i % 2 == 0);
      QTest::qWait(20);
      w.resize(640, 320);
      QTest::qWait(20);
      scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());
      for (const auto *id : {"nearbyComputersButton", "savedDevicesButton", "recentServersButton", "btnToggleCore"}) {
        auto *action = w.findChild<QAbstractButton *>(id);
        QVERIFY(action);
        QVERIFY(action->isVisible());
        QVERIFY(w.rect().contains(QRect(action->mapTo(&w, QPoint()), action->size())));
        QVERIFY(!scroll->isAncestorOf(action));
      }
      const auto before = Settings::value(Settings::Client::RemoteHost);
      bool opened = false;
      QTimer::singleShot(0, &w, [&] {
        auto *dialog = qobject_cast<NearbyDevicesDialog *>(QApplication::activeModalWidget());
        opened = dialog != nullptr;
        if (dialog)
          QTest::keyClick(dialog, Qt::Key_Escape);
      });
      nearby->click();
      QVERIFY(opened);
      QCOMPARE(Settings::value(Settings::Client::RemoteHost), before);
    }
  }
  void wideConnectedWindowKeepsAContentSizedHeight()
  {
    MainWindow w;
    w.open();
    QVERIFY(w.labScenario("connected"));
    QTest::qWait(60);
    w.resize(980, 620);
    QTest::qWait(30);
    QCOMPARE(w.width(), 980);
    QVERIFY2(w.height() < 520, "Wide connected window kept an empty lower half");
    auto *scroll = w.findChild<QScrollArea *>("mainContentScroll");
    QCOMPARE(scroll->horizontalScrollBar()->maximum(), 0);
    QCOMPARE(scroll->verticalScrollBar()->maximum(), 0);
  }
  void discoveryHasVisibleNetworkLabelAndNoTrustOnCancel()
  {
    MainWindow w;
    w.open();
    QVERIFY(w.labScenario("discovering"));
    QTest::qWait(30);
    auto *dialog = w.findChild<NearbyDevicesDialog *>();
    QVERIFY(dialog);
    auto *label = dialog->findChild<QLabel *>("nearbyNetworkLabel");
    QCOMPARE(label->buddy(), dialog->findChild<QComboBox *>("nearbyNetwork"));
    QVERIFY(!dialog->findChild<QLabel *>("nearbyHeading")->text().isEmpty());
    const auto before = Settings::value("gui/identityBindings");
    QTest::keyClick(dialog, Qt::Key_Escape);
    QCOMPARE(Settings::value("gui/identityBindings"), before);
  }
  void fullLayoutFixtureUsesChineseLabelsAndRealValidGeometry()
  {
    MainWindow w;
    w.open();
    bool checked = false;
    QTimer::singleShot(80, &w, [&] {
      auto *editor = w.findChild<DesktopLayoutEditor *>();
      if (editor) {
        checked = editor->desktops()[0].width == 1920 && editor->desktops()[1].width == 2560 &&
                  editor->desktops()[1].name == "Work-PC" &&
                  QString::fromStdString(editor->desktops()[1].displayName) == "工作电脑" &&
                  editor->findChild<QPushButton *>("layoutSave")->isEnabled();
        editor->reject();
      }
    });
    QVERIFY(w.labScenario("layout"));
    QTest::qWait(110);
    QVERIFY(checked);
  }
  void completeWindowEmptyStateAndKeyboardFocus()
  {
    MainWindow w;
    w.open();
    QVERIFY(w.labScenario("empty"));
    QTest::qWait(20);
    auto *toggle = w.findChild<QToolButton *>("connectionOptionsToggle");
    QVERIFY(toggle);
    toggle->setFocus();
    QVERIFY(toggle->hasFocus());
    QVERIFY(!toggle->isChecked());
    QTest::keyClick(toggle, Qt::Key_Space);
    QVERIFY(toggle->isChecked());
    QTest::keyClick(toggle, Qt::Key_Space);
    QVERIFY(!toggle->isChecked());
    auto *details = w.findChild<QPushButton *>("diagnosticDetailsButton");
    details->setFocus();
    QTest::keyClick(details, Qt::Key_Space);
    QVERIFY(w.findChild<QLabel *>("diagnosticDetails")->isVisible());
    QTest::keyClick(details, Qt::Key_Tab);
    QVERIFY(QApplication::focusWidget() != details);
  }
  void connectedStateCollapsesDuplicateControls()
  {
    MainWindow w;
    w.open();
    QVERIFY(w.labScenario("connected"));
    QTest::qWait(20);
    QVERIFY(!w.findChild<QToolButton *>("connectionOptionsToggle")->isChecked());
    auto *d = w.findChild<ConnectionDiagnostics *>();
    QVERIFY(d);
    QCOMPARE(d->phase(), ConnectionDiagnostics::Phase::Connected);
  }
  void errorsAndRetryRemainDistinct()
  {
    MainWindow w;
    w.open();
    w.labScenario("error");
    auto *d = w.findChild<ConnectionDiagnostics *>();
    QCOMPARE(d->phase(), ConnectionDiagnostics::Phase::NeedsAction);
    w.labScenario("retry");
    QCOMPARE(d->phase(), ConnectionDiagnostics::Phase::Recovering);
    QVERIFY(
        w.findChild<QLabel *>("diagnosticReason")->text().contains("20") ||
        w.findChild<QLabel *>("diagnosticState")->text().contains("20")
    );
  }
  void nearbyDialogIsReachableAndEscapeKeepsConfiguration()
  {
    MainWindow w;
    w.open();
    const auto before = Settings::value(Settings::Client::RemoteHost);
    bool opened = false;
    QTimer::singleShot(0, &w, [&] {
      auto *dialog = qobject_cast<NearbyDevicesDialog *>(QApplication::activeModalWidget());
      opened = dialog != nullptr;
      if (dialog)
        QTest::keyClick(dialog, Qt::Key_Escape);
    });
    w.findChild<QPushButton *>("nearbyComputersButton")->click();
    QVERIFY(opened);
    QCOMPARE(Settings::value(Settings::Client::RemoteHost), before);
  }
  void nearbySelectionWorksFromKeyboard()
  {
    NearbyDevicesDialog dialog;
    dialog.show();
    QTest::qWait(10);
    auto *provider = dialog.findChild<deskflow::gui::NearbyDiscovery *>();
    Q_EMIT provider->found("one", "Synthetic", "pc.local", 34800);
    auto *list = dialog.findChild<QListWidget *>("nearbyDevices");
    list->setFocus();
    QTest::keyClick(list, Qt::Key_Down);
    QCOMPARE(list->currentRow(), 0);
    auto *select = dialog.findChild<QPushButton *>("selectNearbyAddress");
    select->setFocus();
    QTest::keyClick(select, Qt::Key_Space);
    QCOMPARE(dialog.result(), int(QDialog::Accepted));
  }
  void deviceListsKeepBothNameAndEndpointVisible()
  {
    MainWindow w;
    w.open();
    for (const auto *scenario : {"discovering", "trusted-devices"}) {
      QVERIFY(w.labScenario(scenario));
      QTest::qWait(20);
      auto *dialog = scenario == QString("discovering") ? static_cast<QDialog *>(w.findChild<NearbyDevicesDialog *>())
                                                        : static_cast<QDialog *>(w.findChild<TrustedDevicesDialog *>());
      QVERIFY(dialog);
      auto *list = dialog->findChild<QListWidget *>();
      QVERIFY(list);
      QTRY_VERIFY_WITH_TIMEOUT(list->count() > 0, 1000);
      const auto *item = list->item(0);
      QVERIFY(item->text().contains('\n'));
      QStyleOptionViewItem option;
      option.initFrom(list);
      option.widget = list;
      option.rect = list->visualItemRect(item);
      option.features = QStyleOptionViewItem::HasDisplay;
      option.text = item->text();
      option.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
      const auto text = list->style()->subElementRect(QStyle::SE_ItemViewItemText, &option, list);
      QVERIFY2(
          text.height() >= 2 * list->fontMetrics().height(),
          qPrintable(
              QString("Two-line device row clipped: %1 < %2").arg(text.height()).arg(2 * list->fontMetrics().height())
          )
      );
      dialog->close();
    }
  }
  void actualLayoutDialogKeyboardMoveUndoAndCancel()
  {
    MainWindow w;
    w.open();
    auto *server = w.findChild<QRadioButton *>("rbModeServer");
    server->setChecked(true);
    w.serverConfig().addClient("Work-PC");
    QVERIFY(w.serverConfig().setDesktopLayout({{"Lab-Mac", 0, 0, 1920, 1080}, {"Work-PC", 1920, 0, 1920, 1080}}));
    const auto before = w.serverConfig();
    bool editorSeen = false, keyboardWorked = false;
    QTimer::singleShot(0, &w, [&] {
      auto *config = qobject_cast<ServerConfigDialog *>(QApplication::activeModalWidget());
      if (!config) {
        if (auto *modal = QApplication::activeModalWidget())
          modal->close();
        return;
      }
      QTimer::singleShot(0, config, [&] {
        auto *editor = qobject_cast<DesktopLayoutEditor *>(QApplication::activeModalWidget());
        if (!editor)
          return;
        editorSeen = true;
        editor->activateWindow();
        QTest::qWait(10);
        auto *canvas = editor->findChild<LayoutCanvas *>("layoutCanvas");
        canvas->setFocus();
        QTest::keyClick(canvas, Qt::Key_PageDown);
        QTest::keyClick(canvas, Qt::Key_Right, Qt::ShiftModifier);
        const auto moved = editor->desktops()[1].x;
        keyboardWorked = moved == 1921;
        QTest::keySequence(editor, QKeySequence::Undo);
        keyboardWorked = keyboardWorked && editor->desktops()[1].x == 1920;
        qInfo() << "keyboard move x" << moved << "undo x" << editor->desktops()[1].x;
        QTest::keyClick(editor, Qt::Key_Escape);
      });
      config->findChild<QPushButton *>("editDesktopLayout")->click();
      QTest::keyClick(config, Qt::Key_Escape);
    });
    w.findChild<QPushButton *>("btnConfigureServer")->click();
    QVERIFY(editorSeen);
    QVERIFY(keyboardWorked);
    QVERIFY(w.serverConfig() == before);
  }
};
int main(int argc, char **argv)
{
  const auto root = qEnvironmentVariable("DESKFLOW_TEST_ROOT");
  if (root.isEmpty() || !QDir(root).exists())
    return 2;
  QTemporaryDir dir(root + "/full-window-XXXXXX");
  if (!dir.isValid())
    return 2;
  qputenv("XDG_CONFIG_HOME", (dir.path() + "/config").toUtf8());
  qputenv("XDG_STATE_HOME", (dir.path() + "/state").toUtf8());
  QApplication app(argc, argv);
  app.setStyle("Fusion");
  QTranslator translator;
  const auto translation = qEnvironmentVariable("DESKFLOW_TEST_TRANSLATION");
  if (!translation.isEmpty()) {
    if (!translator.load(translation))
      return 3;
    app.installTranslator(&translator);
  }
  if (qEnvironmentVariable("DESKFLOW_TEST_THEME") == "dark") {
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#202630"));
    palette.setColor(QPalette::WindowText, QColor("#e7edf5"));
    palette.setColor(QPalette::Base, QColor("#18202c"));
    palette.setColor(QPalette::Text, QColor("#e7edf5"));
    palette.setColor(QPalette::Button, QColor("#303b4b"));
    palette.setColor(QPalette::ButtonText, QColor("#e7edf5"));
    app.setPalette(palette);
  }
  Settings::setValue(Settings::Core::ComputerName, "Lab-Mac");
  Settings::setValue(Settings::Core::ProcessMode, Settings::ProcessMode::Desktop);
  Settings::setValue(Settings::Client::RemoteHost, "fixture.invalid");
  Settings::setValue(Settings::Security::TlsEnabled, true);
  Settings::setValue(Settings::Security::CheckPeers, true);
  Settings::setValue(Settings::Gui::AutoUpdateCheck, false);
  FullWindowTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "FullWindowTests.moc"
