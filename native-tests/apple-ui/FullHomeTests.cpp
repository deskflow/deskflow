// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/Settings.h"
#include "gui/LabRun.h"
#include "gui/MainWindow.h"
#include "gui/StyleUtils.h"
#include "gui/TlsUtility.h"
#include "gui/core/NetworkMonitor.h"
#include "gui/dialogs/DesktopLayoutEditor.h"
#include "gui/dialogs/NearbyDevicesDialog.h"
#include "gui/dialogs/ServerConfigDialog.h"
#include "gui/dialogs/SettingsDialog.h"
#include "gui/widgets/ConnectionHome.h"
#include "gui/widgets/LogDock.h"
#include "net/FingerprintDatabase.h"
#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QScopeGuard>
#include <QScrollArea>
#include <QSignalSpy>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStyleOptionButton>
#include <QTabBar>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <QTranslator>
#include <QUuid>
#include <cmath>
using Home = deskflow::gui::ConnectionHome;
class FullHomeTests : public QObject
{
  Q_OBJECT
  void capture(QWidget &widget, const QString &name)
  {
    // Settling a layout must not depend on whether screenshots are requested.
    QApplication::processEvents();
    const auto destination = qEnvironmentVariable("DESKFLOW_UI_TEST_SCREENSHOTS");
    if (destination.isEmpty())
      return;
    QVERIFY(QDir().mkpath(destination));
    QVERIFY(widget.grab().save(destination + "/" + name + ".png"));
  }
  static QRect indicatorRect(QAbstractButton *button, QStyle::SubElement element)
  {
    QStyleOptionButton option;
    option.initFrom(button);
    option.text = button->text();
    option.state |= button->isChecked() ? QStyle::State_On : QStyle::State_Off;
    return button->style()->subElementRect(element, &option, button);
  }
  static double luminance(const QColor &color)
  {
    const auto linear = [](double value) {
      return value <= 0.04045 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
  }
  static QJsonObject boundaryEvidence(QWidget &dialog, QAbstractButton *button, QStyle::SubElement element)
  {
    const auto image = dialog.grab().toImage();
    const auto scale = image.devicePixelRatio();
    const auto local = indicatorRect(button, element);
    const auto logical = QRect(button->mapTo(&dialog, local.topLeft()), local.size());
    const QRect pixels(
        qRound(logical.x() * scale), qRound(logical.y() * scale), qRound(logical.width() * scale),
        qRound(logical.height() * scale)
    );
    const auto background = image.pixelColor(pixels.right() + qRound(3 * scale), pixels.center().y());
    const auto backgroundLuminance = luminance(background);
    const auto contrast = [&](const QColor &color) {
      const auto l = luminance(color);
      return (qMax(l, backgroundLuminance) + 0.05) / (qMin(l, backgroundLuminance) + 0.05);
    };
    QJsonArray sides;
    double minimum = 100;
    QColor strongest;
    double best = 0;
    // Check the strongest visible boundary on every side, rather than merely
    // matching a stylesheet token or checking only one edge.
    for (int side = 0; side < 4; ++side) {
      double peak = 0;
      for (int y = pixels.top(); y <= pixels.bottom(); ++y)
        for (int x = pixels.left(); x <= pixels.right(); ++x) {
          const bool inBand = (side == 0 && y < pixels.top() + pixels.height() / 3) ||
                              (side == 1 && y > pixels.bottom() - pixels.height() / 3) ||
                              (side == 2 && x < pixels.left() + pixels.width() / 3) ||
                              (side == 3 && x > pixels.right() - pixels.width() / 3);
          if (!inBand)
            continue;
          const auto color = image.pixelColor(x, y);
          const auto value = contrast(color);
          peak = qMax(peak, value);
          if (value > best) {
            best = value;
            strongest = color;
          }
        }
      sides.append(peak);
      minimum = qMin(minimum, peak);
    }
    return {
        {"control", button->objectName()},
        {"background", background.name()},
        {"strongest_boundary_color", strongest.name()},
        {"side_peak_contrasts", sides},
        {"minimum_side_peak_contrast", minimum},
        {"device_pixel_ratio", scale},
        {"indicator_width", local.width()},
        {"indicator_height", local.height()}
    };
  }
private Q_SLOTS:
#ifdef DESKFLOW_UI_REVIEW_BUILD
  void fullWindowRoutesReachTheRealDialogsWithoutExecution()
  {
    Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::None);
    Settings::setValue(Settings::Client::RemoteHost, QString());
    MainWindow window;
    window.show();
    QTest::qWait(1);
    auto *home = window.findChild<Home *>();
    QVERIFY(home);
    bool inspected = false;
    const auto port = Settings::value(Settings::Core::Port);
    QTimer::singleShot(0, &window, [&] {
      auto *dialog = qobject_cast<SettingsDialog *>(QApplication::activeModalWidget());
      QVERIFY(dialog);
      const auto close = qScopeGuard([dialog] {
        if (dialog->isVisible())
          dialog->reject();
      });
      inspected = true;
      QVERIFY(dialog->findChild<QLabel *>("settingsReviewBoundary"));
      QVERIFY(!dialog->findChild<QPushButton *>("btnTlsRegenCert")->isEnabled());
      QVERIFY(!dialog->findChild<QPushButton *>("btnTlsCertPath")->isEnabled());
      dialog->findChild<QSpinBox *>("sbPort")->setValue(port.toInt() + 1);
      capture(*dialog, "full-route-settings");
      auto *save = dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save);
      QVERIFY(save);
      save->click();
    });
    home->findChild<QPushButton *>("homeSettings")->click();
    QVERIFY(inspected);
    QCOMPARE(Settings::value(Settings::Core::Port), port);
    home->findChild<QPushButton *>("homeReceive")->click();
    inspected = false;
    QTimer::singleShot(0, &window, [&] {
      auto *dialog = qobject_cast<deskflow::gui::NearbyDevicesDialog *>(QApplication::activeModalWidget());
      QVERIFY(dialog);
      const auto close = qScopeGuard([dialog] {
        if (dialog->isVisible())
          dialog->reject();
      });
      inspected = true;
      QVERIFY(!dialog->findChild<QPushButton *>("refreshNearby")->isEnabled());
      capture(*dialog, "full-route-discovery");
      dialog->findChild<QPushButton *>("nearbyManualEntry")->click();
    });
    home->findChild<QPushButton *>("homePrimary")->click();
    QVERIFY(inspected);
    auto *address = home->findChild<QLineEdit *>("homeAddress");
    QVERIFY(address->isVisible());
    QTest::keyClick(address, Qt::Key_Escape);
    home->showRoleSelection();
    home->findChild<QPushButton *>("homeShare")->click();
    capture(window, "full-route-share-before-layout");
    inspected = false;
    QTimer::singleShot(0, &window, [&] {
      auto *dialog = qobject_cast<ServerConfigDialog *>(QApplication::activeModalWidget());
      QVERIFY(dialog);
      const auto close = qScopeGuard([dialog] {
        if (dialog->isVisible())
          dialog->reject();
      });
      inspected = true;
      auto *editor = dialog->findChild<DesktopLayoutEditor *>("serverArrangement");
      QVERIFY(editor);
      QVERIFY(editor->isVisible());
      capture(*dialog, "full-route-layout");
      QVERIFY(dialog->addClient("Lab-Windows")); // Synthetic second computer in this isolated draft only.
      const auto draft = editor->desktops();
      capture(*dialog, "full-route-layout-two-computers");
      auto *initialScroll = dialog->findChild<QScrollArea *>("arrangementScroll");
      auto *initialAdvanced = dialog->findChild<QPushButton *>("advancedServerSettings");
      const auto actionRect =
          QRect(initialAdvanced->mapTo(initialScroll->viewport(), QPoint()), initialAdvanced->size());
      QVERIFY2(
          initialScroll->viewport()->rect().contains(actionRect),
          qPrintable(QString("Default editor height %1, minimum %2, hint %3, viewport %4, action bottom %5")
                         .arg(editor->height())
                         .arg(editor->minimumHeight())
                         .arg(editor->minimumSizeHint().height())
                         .arg(initialScroll->viewport()->height())
                         .arg(actionRect.bottom()))
      );
      editor->findChild<QToolButton *>("layoutAdvancedToggle")->click();
      dialog->resize(680, 680);
      capture(*dialog, "full-route-layout-minimum-details");
      QCOMPARE(dialog->width(), 680);
      auto *scroll = dialog->findChild<QScrollArea *>("arrangementScroll");
      QVERIFY(scroll);
      auto *advanced = dialog->findChild<QPushButton *>("advancedServerSettings");
      scroll->ensureWidgetVisible(advanced);
      QApplication::processEvents();
      QVERIFY(scroll->viewport()->rect().contains(QRect(advanced->mapTo(scroll->viewport(), QPoint()), advanced->size())
      ));
      capture(*dialog, "full-route-layout-minimum-scrolled");
      if (!qEnvironmentVariable("DESKFLOW_UI_TEST_TRANSLATION").isEmpty())
        QCOMPARE(advanced->text(), QString("高级设置…"));
      editor->findChild<QToolButton *>("layoutAdvancedToggle")->click();
      dialog->findChild<QPushButton *>("advancedServerSettings")->click();
      capture(*dialog, "full-route-layout-hotkeys");
      auto *tabs = dialog->findChild<QTabWidget *>("tabWidget");
      tabs->setCurrentIndex(0);
      QVERIFY(dialog->findChild<QLabel *>("lblNewComputer")->pixmap().width() <= 28);
      capture(*dialog, "full-route-layout-compatibility");
      tabs->setCurrentIndex(2);
      capture(*dialog, "full-route-layout-advanced");
      dialog->findChild<QPushButton *>("backToArrangement")->click();
      QVERIFY(editor->isVisible());
      QCOMPARE(editor->desktops(), draft);
      capture(*dialog, "full-route-layout-returned");
      QTest::keyClick(editor, Qt::Key_Escape);
      QVERIFY(!dialog->isVisible());
    });
    home->findChild<QPushButton *>("homeLayout")->click();
    QVERIFY(inspected);
    QVERIFY(home->isVisible());
    QVERIFY(!deskflow::gui::TlsUtility::generateCertificate());
    const auto certificate = Settings::value(Settings::Security::Certificate).toString();
    QVERIFY(!QFileInfo::exists(certificate));
    FingerprintDatabase database;
    QVERIFY(!database.write(Settings::tlsTrustedServersDb()));
    QVERIFY(!QFileInfo::exists(Settings::tlsTrustedServersDb()));
    QVERIFY(!QFileInfo::exists(Settings::tlsTrustedClientsDb()));
    QVERIFY(deskflow::gui::NetworkMonitor::validAddresses().isEmpty());
    capture(window, "full-route-returned-home");
  }
#endif
  void fullControllerLayoutRemainsBounded()
  {
    Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::Client);
    Settings::setValue(Settings::Client::RemoteHost, "isolated-host.invalid");
    MainWindow window;
    window.show();
    QTest::qWait(1);
    auto *home = window.findChild<Home *>();
    QVERIFY(home);
    for (const auto size : {QSize(760, 560), QSize(1920, 960), QSize(620, 480)}) {
      window.resize(size);
      QTest::qWait(1);
      auto *column = home->findChild<QWidget *>("homeColumn");
      QVERIFY(column->width() <= 880);
      auto *primary = home->findChild<QPushButton *>("homePrimary");
      auto *footer = home->findChild<QWidget *>("homeFooter");
      QVERIFY(footer->mapTo(home, QPoint()).y() - primary->mapTo(home, QPoint(0, primary->height())).y() < 65);
      capture(window, QString("full-home-%1x%2").arg(size.width()).arg(size.height()));
    }
  }
  void uncheckedIndicatorsRemainReadableAndOperable()
  {
    const auto showVersion = Settings::value(Settings::Gui::ShowVersionInTitle);
    const auto autoHide = Settings::value(Settings::Gui::Autohide);
    ServerConfig config;
    SettingsDialog dialog(nullptr, config);
    dialog.show();
    dialog.findChild<QTabWidget *>("tabWidget")->setCurrentIndex(1);
    auto *check = dialog.findChild<QCheckBox *>("cbShowVersion");
    QVERIFY(check);
    auto *radio = dialog.findChild<QRadioButton *>("rbAutoHide");
    QVERIFY(radio);
    auto *show = dialog.findChild<QRadioButton *>("rbShowOnStart");
    QVERIFY(show);
    check->setChecked(false);
    show->setChecked(true);
    dialog.findChild<QListWidget *>("settingsNavigation")->setFocus();
    QApplication::processEvents();
    capture(dialog, "controls-unchecked");
    const auto checkEvidence = boundaryEvidence(dialog, check, QStyle::SE_CheckBoxIndicator);
    const auto radioEvidence = boundaryEvidence(dialog, radio, QStyle::SE_RadioButtonIndicator);
    const auto checkRect = indicatorRect(check, QStyle::SE_CheckBoxIndicator);
    const auto radioRect = indicatorRect(radio, QStyle::SE_RadioButtonIndicator);
    check->setFocus(Qt::TabFocusReason);
    QTest::keyClick(check, Qt::Key_Space);
    QVERIFY(check->isChecked());
    QCOMPARE(indicatorRect(check, QStyle::SE_CheckBoxIndicator), checkRect);
    radio->setFocus(Qt::TabFocusReason);
    QTest::keyClick(radio, Qt::Key_Space);
    QVERIFY(radio->isChecked());
    QVERIFY(!show->isChecked());
    QCOMPARE(indicatorRect(radio, QStyle::SE_RadioButtonIndicator), radioRect);
    capture(dialog, "controls-checked-keyboard");
    QTest::keyClick(check, Qt::Key_Space);
    QVERIFY(!check->isChecked());
    check->setEnabled(false);
    QTest::keyClick(check, Qt::Key_Space);
    QVERIFY(!check->isChecked());
    capture(dialog, "controls-disabled");
    check->setEnabled(true);
    check->setFocus(Qt::TabFocusReason);
    capture(dialog, "controls-unchecked-focus");
    QTest::keyClick(&dialog, Qt::Key_Escape);
    QVERIFY(!dialog.isVisible());
    QCOMPARE(Settings::value(Settings::Gui::ShowVersionInTitle), showVersion);
    QCOMPARE(Settings::value(Settings::Gui::Autohide), autoHide);
    QVERIFY(Settings::value(Settings::Security::TlsEnabled).toBool());
    QVERIFY(Settings::value(Settings::Security::CheckPeers).toBool());
    const auto destination = qEnvironmentVariable("DESKFLOW_UI_TEST_SCREENSHOTS");
    if (!destination.isEmpty()) {
      QFile file(destination + "/indicator-contrast.json");
      QVERIFY(file.open(QIODevice::WriteOnly));
      file.write(QJsonDocument(QJsonArray{checkEvidence, radioEvidence}).toJson());
    }
    if (qEnvironmentVariable("DESKFLOW_UI_TEST_THEME") == "dark") {
      QVERIFY2(
          checkEvidence["minimum_side_peak_contrast"].toDouble() >= 3.0,
          "Unchecked checkbox boundary must reach 3:1 on each side"
      );
      QVERIFY2(
          radioEvidence["minimum_side_peak_contrast"].toDouble() >= 3.0,
          "Unchecked radio boundary must reach 3:1 on each side"
      );
    }
  }
  void firstRunAddressAndBackUseTheRealController()
  {
    Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::None);
    Settings::setValue(Settings::Client::RemoteHost, QString());
    MainWindow window;
    window.show();
    QTest::qWait(10);
    auto *home = window.findChild<Home *>();
    QVERIFY(home);
    QVERIFY(home->isVisible());
    QCOMPARE(home->state().role, Home::Role::None);
    capture(window, "controller-first-run");
    window.resize(620, 480);
    capture(window, "controller-first-run-minimum");
    window.resize(760, 560);
    home->findChild<QPushButton *>("homeReceive")->click();
    QCOMPARE(Settings::value(Settings::Core::CoreMode).value<Settings::CoreMode>(), Settings::CoreMode::Client);
    QCOMPARE(home->state().role, Home::Role::Receive);
    home->findChild<QPushButton *>("homeManual")->click();
    auto *address = home->findChild<QLineEdit *>("homeAddress");
    QVERIFY(address->isVisible());
    capture(window, "controller-manual-entry");
    const auto fieldBottom = address->mapTo(home, QPoint(0, address->height())).y();
    const auto actionTop = home->findChild<QPushButton *>("homePrimary")->mapTo(home, QPoint()).y();
    QVERIFY(actionTop - fieldBottom < 100);
    window.resize(1024, 740);
    capture(window, "controller-manual-entry-large");
    window.resize(620, 480);
    capture(window, "controller-manual-entry-minimum");
    window.resize(760, 560);
    address->setText("discard.invalid");
    QTest::keyClick(address, Qt::Key_Escape);
    QVERIFY(!address->isVisible());
    QCOMPARE(Settings::value(Settings::Client::RemoteHost).toString(), QString());
    capture(window, "controller-keyboard-cancel");
    home->findChild<QPushButton *>("homeManual")->click();
    address->setText("isolated-host.invalid");
    home->findChild<QPushButton *>("homePrimary")->click();
    QCOMPARE(Settings::value(Settings::Client::RemoteHost).toString(), QString("isolated-host.invalid"));
    QVERIFY(!home->state().identityReady);
    capture(window, "controller-identity-missing");
#ifndef DESKFLOW_UI_REVIEW_BUILD
    home->findChild<QPushButton *>("homePrimary")->click(); // Basic Lab refuses settings actions.
#endif
    QVERIFY(!QFileInfo::exists(Settings::value(Settings::Security::Certificate).toString()));
    QVERIFY(!QFileInfo::exists(Settings::tlsTrustedServersDb()));
    QVERIFY(!QFileInfo::exists(Settings::tlsTrustedClientsDb()));
    home->findChild<QPushButton *>("homeOptions")->click();
    QVERIFY(!home->isVisible());
    capture(window, "controller-connection-options");
    window.findChild<QPushButton *>("backToComputers")->click();
    QVERIFY(home->isVisible());
    QCOMPARE(Settings::value(Settings::Gui::AutoStartCore).toBool(), false);
    QVERIFY(Settings::value(Settings::Security::TlsEnabled).toBool());
    QVERIFY(Settings::value(Settings::Security::CheckPeers).toBool());
    auto *log = window.findChild<LogDock *>();
    QVERIFY(log);
    QVERIFY(!log->isVisible());
  }
  void settingsCancelDoesNotChangeIdentityOrConfiguration()
  {
    ServerConfig config;
    const auto port = Settings::value(Settings::Core::Port);
    const auto certificate = Settings::value(Settings::Security::Certificate).toString();
    SettingsDialog dialog(nullptr, config);
    dialog.show();
    auto *tabs = dialog.findChild<QTabWidget *>("tabWidget");
    QVERIFY(tabs);
    QVERIFY(!tabs->tabBar()->isVisible());
    QCOMPARE(dialog.findChild<QListWidget *>("settingsNavigation")->count(), 5);
    auto *navigation = dialog.findChild<QListWidget *>("settingsNavigation");
    navigation->setFocus();
    QTest::keyClick(navigation, Qt::Key_Down);
    QCOMPARE(tabs->currentIndex(), 1);
    capture(dialog, "settings-keyboard-navigation");
    auto *buttons = dialog.findChild<QDialogButtonBox *>();
    QVERIFY(buttons);
    QVERIFY(!buttons->button(QDialogButtonBox::Reset)->isVisible());
    QVERIFY(!buttons->button(QDialogButtonBox::RestoreDefaults)->isVisible());
    QVERIFY(dialog.findChild<QToolButton *>("settingsMoreActions")->isVisible());
    for (int page = 0; page < tabs->count(); ++page) {
      tabs->setCurrentIndex(page);
      capture(dialog, QString("settings-tab-%1").arg(page));
      if (page == 3) {
        auto *details = dialog.findChild<QWidget *>("connectionDetailsBody");
        QVERIFY(!details->isVisible());
        QVERIFY(dialog.findChild<QPushButton *>("btnTlsRegenCert")->isVisible());
        dialog.findChild<QToolButton *>("connectionDetailsToggle")->click();
        QVERIFY(details->isVisible());
        for (const char *name : {"comboInterface", "sbPort", "lineTlsCertPath", "btnTlsCertPath"}) {
          auto *field = dialog.findChild<QWidget *>(name);
          QVERIFY(field);
          QVERIFY(details->isAncestorOf(field));
          QVERIFY(field->isVisibleTo(details));
        }
        capture(dialog, "settings-connection-expanded");
        auto *scroll = tabs->currentWidget()->findChild<QScrollArea *>("settingsPageScroll");
        QVERIFY(scroll);
        auto *choose = dialog.findChild<QPushButton *>("btnTlsCertPath");
        scroll->ensureWidgetVisible(choose);
        QApplication::processEvents();
        const auto fieldRect = QRect(choose->mapTo(scroll->viewport(), QPoint()), choose->size());
        QVERIFY(scroll->viewport()->rect().contains(fieldRect));
        capture(dialog, "settings-connection-expanded-scrolled");
        dialog.findChild<QToolButton *>("connectionDetailsToggle")->click();
        dialog.resize(700, 500);
        capture(dialog, "settings-connection-minimum");
        dialog.resize(760, 560);
      }
    }
    for (const char *name : {"btnTlsCertPath", "btnBrowseLog"}) {
      auto *browse = dialog.findChild<QPushButton *>(name);
      QVERIFY(!browse->text().isEmpty());
      QVERIFY(!browse->accessibleName().isEmpty());
    }
    QVERIFY(dialog.findChild<QLabel *>("identityStatus"));
    buttons->button(QDialogButtonBox::Cancel)->click();
    QVERIFY(!dialog.isVisible());
    QCOMPARE(Settings::value(Settings::Core::Port), port);
    QCOMPARE(Settings::value(Settings::Security::Certificate).toString(), certificate);
    QVERIFY(!QFileInfo::exists(certificate));
    QVERIFY(Settings::value(Settings::Security::TlsEnabled).toBool());
    QVERIFY(Settings::value(Settings::Security::CheckPeers).toBool());
  }
  void settingsEscapeDiscardsDraft()
  {
    ServerConfig config;
    const auto port = Settings::value(Settings::Core::Port).toInt();
    SettingsDialog dialog(nullptr, config);
    dialog.show();
    dialog.findChild<QTabWidget *>("tabWidget")->setCurrentIndex(3);
    dialog.findChild<QToolButton *>("connectionDetailsToggle")->click();
    dialog.findChild<QSpinBox *>("sbPort")->setValue(port + 1);
    QTest::keyClick(&dialog, Qt::Key_Escape);
    QVERIFY(!dialog.isVisible());
    QCOMPARE(Settings::value(Settings::Core::Port).toInt(), port);
  }
  void invalidIdentityIsNotPresentedAsReady()
  {
    const auto original = Settings::value(Settings::Security::Certificate).toString();
    const auto fixture = original + ".invalid-fixture";
    QFile file(fixture);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("invalid test fixture\n");
    file.close();
    Settings::setValue(Settings::Security::Certificate, fixture);
    ServerConfig config;
    SettingsDialog dialog(nullptr, config);
    dialog.show();
    dialog.findChild<QTabWidget *>("tabWidget")->setCurrentIndex(3);
    QCOMPARE(dialog.findChild<QLabel *>("identityHeading")->property("identityState").toString(), QString("invalid"));
    capture(dialog, "settings-invalid-identity");
    dialog.reject();
    Settings::setValue(Settings::Security::Certificate, original);
  }
  void emptyDiscoveryHasAManualExitWithoutSearching()
  {
    deskflow::gui::NearbyDevicesDialog dialog(nullptr, [] { return QVector<deskflow::gui::DiscoveryScope>{}; });
    QSignalSpy searching(&dialog, &deskflow::gui::NearbyDevicesDialog::searchingChanged);
    QSignalSpy manual(&dialog, &deskflow::gui::NearbyDevicesDialog::manualEntryRequested);
    dialog.show();
    QVERIFY(dialog.findChild<QLabel *>("nearbyEmpty")->isVisible());
    QVERIFY(!dialog.findChild<QPushButton *>("refreshNearby")->isEnabled());
    capture(dialog, "nearby-empty");
    dialog.findChild<QPushButton *>("nearbyManualEntry")->click();
    QCOMPARE(manual.count(), 1);
    QVERIFY(!dialog.isVisible());
    for (const auto &event : searching)
      QCOMPARE(event.at(0).toBool(), false);
  }
};
int main(int argc, char **argv)
{
  const auto root =
      QStringLiteral(DESKFLOW_LAB_RUNS_ROOT) + "/home-flow-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  QByteArray executable = argv[0], flag = "--lab-root", path = root.toUtf8();
  char *preArgs[] = {executable.data(), flag.data(), path.data()};
  if (deskflow::gui::lab::prepare(3, preArgs).isEmpty())
    return 2;
  QApplication app(argc, argv);
  app.setApplicationName("DeskflowLab");
  if (qEnvironmentVariable("DESKFLOW_UI_TEST_THEME") == "dark") {
    auto palette = app.palette();
    palette.setColor(QPalette::Window, QColor("#202124"));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor("#2c2d31"));
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor("#303136"));
    palette.setColor(QPalette::ButtonText, Qt::white);
    app.setPalette(palette);
  }
  QTranslator translation, qtTranslation;
  if (translation.load(qEnvironmentVariable("DESKFLOW_UI_TEST_TRANSLATION")))
    app.installTranslator(&translation);
  if (qtTranslation.load(qEnvironmentVariable("DESKFLOW_QT_TRANSLATION")))
    app.installTranslator(&qtTranslation);
  deskflow::gui::updateIconTheme();
  FullHomeTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "FullHomeTests.moc"
