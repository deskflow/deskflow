// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "gui/dialogs/CandidatePermissionDialog.h"
#include "gui/dialogs/PairingReviewDialog.h"
#include "gui/dialogs/TrustedDevicesDialog.h"
#include "gui/widgets/ConnectionHome.h"
#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QFontMetricsF>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLibraryInfo>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScopeGuard>
#include <QScrollArea>
#include <QSignalSpy>
#include <QStyleOptionButton>
#include <QTest>
#include <QTextLayout>
#include <QTranslator>
#include <memory>
using Home = deskflow::gui::ConnectionHome;
using Bindings = deskflow::gui::IdentityBindings;
class HomeUxTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void setupDialogsRemainReadable_data()
  {
    QTest::addColumn<bool>("chinese");
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("state");
    for (bool zh : {false, true})
      for (int width : {540, 620})
        for (int state : {0, 1, 2, 3})
          QTest::newRow(qPrintable(QString("%1-%2-%3").arg(zh ? "zh" : "en").arg(width).arg(state)))
              << zh << width << state;
  }
  void setupDialogsRemainReadable()
  {
    QFETCH(bool, chinese);
    QFETCH(int, width);
    QFETCH(int, state);
    QTranslator translator, qtTranslator;
    if (chinese) {
      QVERIFY(translator.load(":/translations/deskflow_zh_CN.qm"));
      qApp->installTranslator(&translator);
      QVERIFY(qtTranslator.load("qtbase_zh_CN", QLibraryInfo::path(QLibraryInfo::TranslationsPath)));
      qApp->installTranslator(&qtTranslator);
    }
    const auto restore = qScopeGuard([&] {
      qApp->removeTranslator(&qtTranslator);
      qApp->removeTranslator(&translator);
    });
    struct Provider : deskflow::gui::AccessibilityPermissionProvider
    {
      bool granted = false;
      int requests = 0;
      bool isGranted() const override
      {
        return granted;
      }
      void request() override
      {
        ++requests;
      }
    } provider;
    Bindings bindings;
    const auto g = bindings.begin({"work-pc.local", 34800});
    bindings.observe(g, QString(64, 'a'));
    std::unique_ptr<QDialog> dialog;
    if (state < 2) {
      auto *review = new PairingReviewDialog(bindings, QString(64, 'b'));
      dialog.reset(review);
      if (state == 1)
        review->expireReview();
    } else {
      provider.granted = state == 3;
      dialog = std::make_unique<deskflow::gui::CandidatePermissionDialog>(provider, 27);
    }
    dialog->resize(width, dialog->minimumHeight());
    dialog->show();
    QTest::qWait(1);
    QCOMPARE(dialog->width(), width);
    const auto output = qEnvironmentVariable("UX_REVIEW_SCREENSHOTS");
    if (chinese && width == 540 && !output.isEmpty()) {
      QDir().mkpath(output);
      QVERIFY(dialog->grab().save(output + QString("/dialog-%1.png").arg(state)));
    }
    for (auto *button : dialog->findChildren<QPushButton *>()) {
      if (!button->isVisible())
        continue;
      QStyleOptionButton option;
      option.initFrom(button);
      option.text = button->text();
      option.rect = button->rect();
      const auto contents = button->style()->subElementRect(QStyle::SE_PushButtonContents, &option, button);
      const QFontMetricsF metrics(button->font(), button);
      const auto detail = QString("%1: button=%2x%3 contents=%4x%5 glyph=%6x%7 window=%8x%9")
                              .arg(button->text())
                              .arg(button->width())
                              .arg(button->height())
                              .arg(contents.width())
                              .arg(contents.height())
                              .arg(metrics.horizontalAdvance(button->text()))
                              .arg(metrics.height())
                              .arg(dialog->width())
                              .arg(dialog->height());
      QVERIFY2(contents.width() + 0.5 >= metrics.horizontalAdvance(button->text()), qPrintable(detail));
      QVERIFY2(contents.height() + 0.5 >= metrics.height(), qPrintable(detail));
      QVERIFY2(
          dialog->rect().contains(QRect(button->mapTo(dialog.get(), QPoint()), button->size())),
          qPrintable(button->text())
      );
    }
    for (auto *label : dialog->findChildren<QLabel *>()) {
      if (!label->isVisible() || !label->wordWrap() || label->text().isEmpty())
        continue;
      QTextLayout text(label->text(), label->font());
      QTextOption option;
      option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
      text.setTextOption(option);
      text.beginLayout();
      qreal needed = 0;
      while (true) {
        auto line = text.createLine();
        if (!line.isValid())
          break;
        line.setLineWidth(label->contentsRect().width());
        needed += line.height();
      }
      text.endLayout();
      QVERIFY2(label->contentsRect().height() + 1 >= needed, qPrintable(label->text()));
    }
    if (state >= 2) {
      auto *check = dialog->findChild<QPushButton *>("checkPermission");
      QVERIFY(check->isEnabled());
      check->click();
      QCOMPARE(provider.requests, 0);
      QCOMPARE(dialog->findChild<QPushButton *>("continuePaused")->isEnabled(), provider.granted);
      QVERIFY(!dialog->findChild<QLabel *>("permissionInstructions")->text().isEmpty());
      QVERIFY(!dialog->findChild<QLabel *>("permissionStatus")->text().isEmpty());
    }
    dialog->reject();
    QVERIFY(bindings.devices().isEmpty());
    QCOMPARE(provider.requests, 0);
  }
  void initTestCase()
  {
    qInfo() << "devicePixelRatio" << qApp->devicePixelRatio();
    bool ok = false;
    const auto requested = qEnvironmentVariable("QT_SCALE_FACTOR").toDouble(&ok);
    if (ok)
      QVERIFY(qAbs(qApp->devicePixelRatio() - requested) < 0.01);
  }
  void translatedActionsFit_data()
  {
    QTest::addColumn<bool>("chinese");
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("stage");
    QTest::addColumn<bool>("share");
    QTest::addColumn<int>("points");
    QTest::addColumn<bool>("dark");
    for (const bool zh : {false, true})
      for (const int width : {620, 760})
        for (const bool share : {false, true})
          for (const int points : {12, 18, 24})
            for (const bool dark : {false, true})
              for (const auto stage :
                   {Home::Stage::Idle, Home::Stage::Starting, Home::Stage::Connecting, Home::Stage::Verifying,
                    Home::Stage::Connected, Home::Stage::Listening, Home::Stage::Retrying, Home::Stage::Recovering,
                    Home::Stage::Stopping, Home::Stage::NeedsAction, Home::Stage::Paused})
                QTest::newRow(qPrintable(QString("%1-%2-%3-%4-%5-%6")
                                             .arg(zh ? "zh" : "en")
                                             .arg(width)
                                             .arg(share ? "share" : "receive")
                                             .arg(int(stage))
                                             .arg(points)
                                             .arg(dark ? "dark" : "light"))
                ) << zh
                  << width << int(stage) << share << points << dark;
  }
  void translatedActionsFit()
  {
    QFETCH(bool, chinese);
    QFETCH(int, width);
    QFETCH(int, stage);
    QFETCH(bool, share);
    QFETCH(int, points);
    QFETCH(bool, dark);
    const auto originalPalette = qApp->palette();
    auto palette = originalPalette;
    palette.setColor(QPalette::Window, dark ? QColor("#202124") : QColor("#fafafa"));
    qApp->setPalette(palette);
    QTranslator translator;
    if (chinese) {
      QVERIFY(translator.load(":/translations/deskflow_zh_CN.qm"));
      qApp->installTranslator(&translator);
    }
    Home home;
    Home::State state;
    state.role = share ? Home::Role::Share : Home::Role::Receive;
    state.stage = Home::Stage(stage);
    state.localName = "Test computer";
    state.identityReady = true;
    state.hasAddress = true;
    state.canStart = true;
    for (auto *button : home.findChildren<QPushButton *>()) {
      auto font = button->font();
      font.setPointSize(points);
      button->setFont(font);
    }
    home.setState(state);
    home.resize(width, 480);
    home.show();
    QTest::qWait(1);
    auto *viewport = home.findChild<QScrollArea *>("homeScroll")->viewport();
    for (const auto *name :
         {"homePrimary", "homeLayout", "homeManual", "homeOptions", "homeDiagnostics", "homeSettings"}) {
      auto *button = home.findChild<QPushButton *>(name);
      if (!button->isVisible())
        continue;
      QStyleOptionButton option;
      option.initFrom(button);
      option.text = button->text();
      option.rect = button->rect();
      const auto contents = button->style()->subElementRect(QStyle::SE_PushButtonContents, &option, button);
      const QFontMetricsF metrics(button->font(), button);
      const QString detail = QString("%1 text=%2 size=%3x%4 hint=%5x%6 content=%7x%8 glyph=%9x%10 dpi=%11")
                                 .arg(name, button->text())
                                 .arg(button->width())
                                 .arg(button->height())
                                 .arg(button->sizeHint().width())
                                 .arg(button->sizeHint().height())
                                 .arg(contents.width())
                                 .arg(contents.height())
                                 .arg(metrics.horizontalAdvance(button->text()))
                                 .arg(metrics.height())
                                 .arg(button->logicalDpiX());
      const auto shot = qEnvironmentVariable("UX_TEST_SCREENSHOTS");
      if (!shot.isEmpty() && chinese && stage == int(Home::Stage::Connected) && width == 620) {
        QDir().mkpath(shot);
        home.grab().save(
            shot +
            QString("/connected-%1-%2-%3.png").arg(share ? "share" : "receive").arg(points).arg(dark ? "dark" : "light")
        );
      }
      QVERIFY2(contents.width() + 0.5 >= metrics.horizontalAdvance(button->text()), qPrintable(detail));
      QVERIFY2(contents.height() + 0.5 >= metrics.height(), qPrintable(detail));
      const auto left = button->mapTo(viewport, QPoint()).x();
      QVERIFY2(left >= 0 && left + button->width() <= viewport->width(), qPrintable(detail));
    }
    if (chinese)
      qApp->removeTranslator(&translator);
    qApp->setPalette(originalPalette);
  }
  void pairingNameIsPrefilledButNeverConfirmsTrust()
  {
    Bindings bindings;
    const auto generation = bindings.begin({"work-pc.local", 34800});
    bindings.observe(generation, QString(64, 'a'));
    PairingReviewDialog dialog(bindings, QString(64, 'b'));
    dialog.show();
    auto *name = dialog.findChild<QLineEdit *>("pairingDisplayName");
    auto *check = dialog.findChild<QCheckBox *>("pairingCompared");
    auto *confirm = dialog.findChild<QPushButton *>("pairingConfirm");
    QCOMPARE(name->text(), QString("work-pc.local"));
    QVERIFY(name->isEnabled());
    QVERIFY(!check->isChecked());
    QVERIFY(!confirm->isEnabled());
    dialog.accept();
    QVERIFY(bindings.devices().isEmpty());
    QVERIFY(dialog.result() != QDialog::Accepted);
    check->setChecked(true);
    QVERIFY(confirm->isEnabled());
    QTest::mouseClick(confirm, Qt::LeftButton);
    QCOMPARE(dialog.result(), int(QDialog::Accepted));
    QVERIFY(bindings.devices().isEmpty());
    QVERIFY(bindings.authenticated(generation, QString(64, 'a'), true, true));
    QCOMPARE(bindings.devices().first().label, QString("work-pc.local"));
  }
  void preparationPrecedesDiscoveryWithoutSideEffects()
  {
    Home home;
    Home::State state;
    state.role = Home::Role::Receive;
    home.setState(state);
    home.show();
    QSignalSpy identity(&home, &Home::identitySetupRequested), find(&home, &Home::findDeviceRequested),
        start(&home, &Home::startStopRequested);
    QTest::mouseClick(home.findChild<QPushButton *>("homePrimary"), Qt::LeftButton);
    QCOMPARE(identity.count(), 1);
    QCOMPARE(find.count(), 0);
    QCOMPARE(start.count(), 0);
    state.identityReady = true;
    home.setState(state);
    QTest::mouseClick(home.findChild<QPushButton *>("homePrimary"), Qt::LeftButton);
    QCOMPARE(identity.count(), 1);
    QCOMPARE(find.count(), 1);
    QCOMPARE(start.count(), 0);
  }
  void savedComputerRouteNeverRestartsPairingOrConnectsOnOpen()
  {
    Home home;
    Home::State state;
    state.role = Home::Role::Receive;
    state.hasSavedDevices = true;
    home.setState(state);
    home.show();
    QSignalSpy identity(&home, &Home::identitySetupRequested), saved(&home, &Home::savedDevicesRequested),
        find(&home, &Home::findDeviceRequested), start(&home, &Home::startStopRequested);
    auto *action = home.findChild<QPushButton *>("homePrimary");
    QTest::mouseClick(action, Qt::LeftButton);
    QCOMPARE(identity.count(), 1);
    QCOMPARE(saved.count(), 0);
    state.identityReady = true;
    home.setState(state);
    QCOMPARE(action->text(), QString("Choose a saved computer…"));
    QTest::mouseClick(action, Qt::LeftButton);
    QCOMPARE(saved.count(), 1);
    QCOMPARE(find.count(), 0);
    QCOMPARE(start.count(), 0);
    state.hasAddress = true;
    state.peerKnown = true;
    state.canStart = true;
    state.peerName = "Work-PC";
    state.stage = Home::Stage::Paused;
    home.setState(state);
    QCOMPARE(action->text(), QString("Reconnect"));
    QCOMPARE(start.count(), 0);
    QTest::mouseClick(action, Qt::LeftButton);
    QCOMPARE(start.count(), 1);
    QCOMPARE(identity.count(), 1);
  }
  void unknownAddressIsNotPresentedAsAlreadyPaired()
  {
    Home home;
    Home::State state;
    state.role = Home::Role::Receive;
    state.identityReady = true;
    state.hasAddress = true;
    state.canStart = true;
    home.setState(state);
    QCOMPARE(home.findChild<QPushButton *>("homePrimary")->text(), QString("Pair and connect"));
    QVERIFY(home.findChild<QLabel *>("homeReason")->text().contains("No serial number"));
    state.peerKnown = true;
    home.setState(state);
    QCOMPARE(home.findChild<QPushButton *>("homePrimary")->text(), QString("Reconnect"));
    QVERIFY(home.findChild<QLabel *>("homeReason")->text().contains("changed identity"));
  }
  void unambiguousSavedSelectionNeedsOnlyExplicitConnect()
  {
    Bindings bindings;
    auto g = bindings.begin({"work-pc.local", 34800});
    bindings.observe(g, QString(64, 'a'));
    QVERIFY(bindings.confirm(g, "Work PC", true));
    QVERIFY(bindings.authenticated(g, QString(64, 'a'), true, true));
    TrustedDevicesDialog dialog(bindings, true);
    QSignalSpy requests(&dialog, &TrustedDevicesDialog::connectRequested);
    dialog.show();
    QCOMPARE(dialog.findChild<QListWidget *>("trustedDeviceList")->currentRow(), 0);
    auto *button = dialog.findChild<QPushButton *>("connectTrustedDevice");
    QVERIFY(button->isEnabled());
    QCOMPARE(requests.count(), 0);
    QTest::mouseClick(button, Qt::LeftButton);
    QCOMPARE(requests.count(), 1);
    QCOMPARE(requests.first()[0].toString(), QString(64, 'a'));
  }
  void reportedNameOnlyReplacesAddressDefaultAndKeepsSchema()
  {
    Bindings bindings;
    const QString fp(64, 'a');
    auto g = bindings.begin({"192.168.50.20", 34800});
    bindings.observe(g, fp);
    QVERIFY(bindings.confirm(g, "192.168.50.20", true));
    QVERIFY(bindings.authenticated(g, fp, true, true));
    QVERIFY(!bindings.updateDefaultLabel(QString(64, 'b'), "Imposter"));
    QVERIFY(!bindings.updateDefaultLabel(fp, "bad\nname"));
    QVERIFY(bindings.updateDefaultLabel(fp, "WIN-H7VRVU0C520"));
    QCOMPARE(bindings.devices()[fp].endpoint.host, QString("192.168.50.20"));
    QVERIFY(bindings.rename(fp, QString::fromUtf8("办公室电脑")));
    QVERIFY(!bindings.updateDefaultLabel(fp, "Remote-renamed"));
    Bindings restored;
    QVERIFY(restored.load(bindings.encode()));
    QCOMPARE(restored.devices(), bindings.devices());
    QCOMPARE(QJsonDocument::fromJson(bindings.encode())["schema"].toInt(), 1);
    const auto selected = restored.selectedDevice({"192.168.50.20", 34800});
    QVERIFY(selected);
    QCOMPARE(selected->sha256, fp);
  }
  void ambiguousAddressNeverSelectsAnArbitraryIdentity()
  {
    Bindings bindings;
    const Bindings::Endpoint endpoint{"192.168.50.20", 34800};
    for (const auto fp : {QString(64, 'a'), QString(64, 'b')}) {
      auto g = bindings.begin(endpoint);
      bindings.observe(g, fp);
      QVERIFY(bindings.confirm(g, "Computer", true));
      QVERIFY(bindings.authenticated(g, fp, true, true));
    }
    QCOMPARE(bindings.atEndpoint(endpoint).size(), 2);
    QVERIFY(!bindings.selectedDevice(endpoint));
    auto selected = bindings.selectedDevice(endpoint, QString(64, 'b'));
    QVERIFY(selected);
    QCOMPARE(selected->sha256, QString(64, 'b'));
    QVERIFY(!bindings.selectedDevice(endpoint, QString(64, 'c')));
    QVERIFY(!bindings.selectedDevice({"192.168.50.30", 34800}));
  }
  void editablePairingNameAndCancelledReviewStayUntrusted()
  {
    Bindings bindings;
    const auto g = bindings.begin({"192.168.50.20", 34800});
    bindings.observe(g, QString(64, 'a'));
    PairingReviewDialog dialog(bindings, QString(64, 'b'));
    dialog.show();
    auto *name = dialog.findChild<QLineEdit *>("pairingDisplayName");
    QVERIFY(!name->text().isEmpty());
    name->setText(QString::fromUtf8("办公室电脑"));
    QCOMPARE(dialog.displayName(), QString::fromUtf8("办公室电脑"));
    dialog.findChild<QCheckBox *>("pairingCompared")->setChecked(true);
    dialog.reject();
    QVERIFY(bindings.devices().isEmpty());
    QVERIFY(!bindings.authenticated(g, QString(64, 'a'), true, true));
  }
  void changedOrExpiredReviewCannotBeConfirmed()
  {
    Bindings bindings;
    auto g = bindings.begin({"work-pc.local", 34800});
    bindings.observe(g, QString(64, 'a'));
    QVERIFY(bindings.confirm(g, "My computer", true));
    QVERIFY(bindings.authenticated(g, QString(64, 'a'), true, true));
    const auto saved = bindings.encode();
    g = bindings.begin({"work-pc.local", 34800}, QString(64, 'a'));
    bindings.observe(g, QString(64, 'c'));
    PairingReviewDialog changed(bindings, QString(64, 'b'));
    changed.show();
    QVERIFY(!changed.findChild<QLineEdit *>("pairingDisplayName")->isVisible());
    QVERIFY(!changed.findChild<QPushButton *>("pairingConfirm")->isVisible());
    changed.accept();
    QCOMPARE(bindings.encode(), saved);
    changed.reject();
    Bindings fresh;
    g = fresh.begin({"work-pc.local", 34800});
    fresh.observe(g, QString(64, 'a'));
    PairingReviewDialog expired(fresh, QString(64, 'b'));
    expired.show();
    expired.findChild<QCheckBox *>("pairingCompared")->setChecked(true);
    expired.expireReview();
    expired.accept();
    QVERIFY(fresh.devices().isEmpty());
    QVERIFY(!fresh.authenticated(g, QString(64, 'a'), true, true));
  }
  void captureFirstPreparationAndPrefilledReview()
  {
    const auto output = qEnvironmentVariable("UX_TEST_SCREENSHOTS");
    if (output.isEmpty())
      return;
    QTranslator qtTranslator;
    QVERIFY(qtTranslator.load("qtbase_zh_CN", QLibraryInfo::path(QLibraryInfo::TranslationsPath)));
    qApp->installTranslator(&qtTranslator);
    QTranslator translator;
    QVERIFY(translator.load(":/translations/deskflow_zh_CN.qm"));
    qApp->installTranslator(&translator);
    Home home;
    Home::State state;
    state.role = Home::Role::Receive;
    state.localName = "Test computer";
    home.setState(state);
    home.resize(760, 560);
    home.show();
    QTest::qWait(1);
    QDir().mkpath(output);
    QVERIFY(home.grab().save(output + "/first-preparation.png"));
    state.identityReady = true;
    state.hasSavedDevices = true;
    home.setState(state);
    QTest::qWait(1);
    QVERIFY(home.grab().save(output + "/choose-saved-computer.png"));
    state.hasAddress = true;
    state.canStart = true;
    state.peerKnown = true;
    state.peerName = QString::fromUtf8("办公室电脑");
    state.stage = Home::Stage::Paused;
    home.setState(state);
    QTest::qWait(1);
    QVERIFY(home.grab().save(output + "/saved-computer-paused.png"));
    Bindings bindings;
    const auto g = bindings.begin({"work-pc.local", 34800});
    bindings.observe(g, QString(64, 'a'));
    PairingReviewDialog dialog(bindings, QString(64, 'b'));
    dialog.show();
    QTest::qWait(1);
    QVERIFY(dialog.grab().save(output + "/prefilled-review.png"));
    dialog.reject();
    qApp->removeTranslator(&translator);
    qApp->removeTranslator(&qtTranslator);
  }
};
int main(int argc, char **argv)
{
  QApplication app(argc, argv);
  app.setStyle("Fusion");
  HomeUxTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "HomeUxTests.moc"
