// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "gui/IdentityBindings.h"
#include "gui/dialogs/TrustedDevicesDialog.h"
#include "gui/widgets/ConnectionHome.h"
#include <QAbstractItemView>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalSpy>
#include <QStyleHints>
#include <QTest>
#include <QTranslator>
#include <QVBoxLayout>
using Home = deskflow::gui::ConnectionHome;
class HomeTests : public QObject
{
  Q_OBJECT
  static QPushButton *b(Home &w, const char *name)
  {
    return w.findChild<QPushButton *>(name);
  }
  static Home::State client()
  {
    Home::State s;
    s.role = Home::Role::Receive;
    s.localName = "Test Mac";
    return s;
  }
private Q_SLOTS:
  void coldMainWindowSizesWorkspaceFromFinalViewport()
  {
    QMainWindow main;
    auto *home = new Home;
    main.setCentralWidget(home);
    home->setDesktopWidget(new QWidget);
    auto state = client();
    state.role = Home::Role::Share;
    state.identityReady = true;
    home->setState(state);
    main.resize(1140, 580);
    main.show();
    QTest::qWait(10);
    auto *scroll = home->findChild<QScrollArea *>("homeScroll");
    auto *column = home->findChild<QWidget *>("homeColumn");
    QCOMPARE(column->width(), qMin(880, scroll->viewport()->width()));
    QVERIFY(column->width() > 600);
  }
  void attachedWorkspaceDoesNotCompressOverview()
  {
    Home w;
    auto *workspace = new QWidget;
    auto *layout = new QVBoxLayout(workspace);
    auto *canvas = new QWidget(workspace);
    canvas->setMinimumSize(240, 280);
    layout->addWidget(canvas);
    w.setDesktopWidget(workspace);
    auto state = client();
    state.role = Home::Role::Share;
    state.identityReady = true;
    state.canStart = true;
    for (const auto size : {QSize(1140, 580), QSize(1024, 740), QSize(620, 480)}) {
      w.resize(size);
      w.setState(state);
      w.show();
      QTest::qWait(5);
      auto *scroll = w.findChild<QScrollArea *>("homeScroll");
      QVERIFY(scroll->isVisible());
      QVERIFY(scroll->widget()->isAncestorOf(workspace));
      auto *session = w.findChild<QWidget *>("homeSession");
      QVERIFY(session);
      QVERIFY(session->height() >= session->minimumSizeHint().height());
      auto *actions = w.findChild<QWidget *>("homeActionCard");
      QVERIFY(actions->height() >= actions->minimumSizeHint().height());
      auto *primary = b(w, "homePrimary");
      QVERIFY(primary->height() >= primary->sizeHint().height());
      scroll->ensureWidgetVisible(primary);
      QTest::qWait(1);
      const QRect primaryRect(primary->mapTo(scroll->viewport(), QPoint()), primary->size());
      QVERIFY(scroll->viewport()->rect().contains(primaryRect));
    }
  }
  void footerActionsKeepTheirLabelsAtMinimumWidth()
  {
    Home w;
    auto state = client();
    state.hasAddress = true;
    w.setState(state);
    w.resize(620, 480);
    w.show();
    QTest::qWait(1);
    for (const char *name : {"homeChangeDirection", "homeSaved", "homeOptions", "homeDiagnostics", "homeSettings"}) {
      auto *control = b(w, name);
      QVERIFY(control->isVisible());
      QVERIFY2(control->width() >= control->sizeHint().width(), name);
      auto *viewport = w.findChild<QScrollArea *>("homeScroll")->viewport();
      const auto left = control->mapTo(viewport, QPoint()).x();
      QVERIFY(left >= 0);
      QVERIFY(left + control->width() <= viewport->width());
    }
  }
  void visibleActionsStayConciseAndNavigationIconsArePresent()
  {
    Home w;
    auto state = client();
    state.hasAddress = true;
    state.identityReady = true;
    state.canStart = true;
    w.setState(state);
    w.resize(1024, 740);
    w.show();
    QTest::qWait(1);
    int visibleActions = 0;
    for (const char *name : {"homeFind", "homeManual", "homeChangeDirection", "homeSaved", "homeSettings"}) {
      auto *control = b(w, name);
      if (control->isVisible()) {
        ++visibleActions;
        QVERIFY2(!control->text().contains(QChar(0x2026)), name);
      }
    }
    QVERIFY(visibleActions >= 3);
    auto *caption = w.findChild<QLabel *>("homeDirectionCaption");
    auto *summary = w.findChild<QLabel *>("homeDirectionSummary");
    auto *switchDirection = b(w, "homeChangeDirection");
    QCOMPARE(caption->text(), QString("Keyboard and mouse control"));
    QCOMPARE(summary->text(), QString("The other computer controls this computer"));
    QCOMPARE(switchDirection->text(), QString("Let this computer control"));
    const int summaryRight = summary->mapTo(&w, QPoint(summary->width(), 0)).x();
    const int buttonLeft = switchDirection->mapTo(&w, QPoint()).x();
    auto *directionBox = qobject_cast<QBoxLayout *>(switchDirection->parentWidget()->layout());
    if (directionBox->direction() == QBoxLayout::LeftToRight)
      QVERIFY(buttonLeft > summaryRight);
    else
      QVERIFY(switchDirection->mapTo(&w, QPoint()).y() > summary->mapTo(&w, QPoint(0, summary->height() - 1)).y());
    auto *directionRow = w.findChild<QWidget *>("homeDirectionRow");
    auto *actionArea = w.findChild<QWidget *>("homeActions");
    const QRect wideDirection(directionRow->mapTo(&w, QPoint()), directionRow->size());
    const QRect wideActions(actionArea->mapTo(&w, QPoint()), actionArea->size());
    QVERIFY(wideActions.top() > wideDirection.bottom());
    QVERIFY(qAbs(wideActions.left() - wideDirection.left()) <= 2);
    w.resize(620, 480);
    QApplication::processEvents();
    const QRect compactDirection(directionRow->mapTo(&w, QPoint()), directionRow->size());
    const QRect compactActions(actionArea->mapTo(&w, QPoint()), actionArea->size());
    QVERIFY(compactActions.top() > compactDirection.bottom());
    QVERIFY(qAbs(compactActions.left() - compactDirection.left()) <= 2);
    QVERIFY(!b(w, "sidebarComputers")->icon().isNull());
    QVERIFY(!b(w, "sidebarConnection")->icon().isNull());
  }
  void actionCardKeepsDirectionAndCommandsTogetherAtEverySize()
  {
    for (const auto role : {Home::Role::Receive, Home::Role::Share}) {
      Home w;
      Home::State state;
      state.role = role;
      state.localName = "Test Mac";
      state.peerName = "Test PC";
      state.hasAddress = true;
      state.identityReady = true;
      state.canStart = true;
      w.setState(state);
      w.show();
      for (const auto size : {QSize(620, 480), QSize(760, 560), QSize(1024, 740), QSize(1920, 960)}) {
        w.resize(size);
        QTest::qWait(1);
        auto *session = w.findChild<QWidget *>("homeSession");
        auto *actionCard = w.findChild<QWidget *>("homeActionCard");
        auto *directionRow = w.findChild<QWidget *>("homeDirectionRow");
        auto *directionButton = b(w, "homeChangeDirection");
        auto *actions = w.findChild<QWidget *>("homeActions");
        const QRect actionRect(actionCard->mapTo(&w, QPoint()), actionCard->size());
        const QRect sessionRect(session->mapTo(&w, QPoint()), session->size());
        const QRect directionRect(directionRow->mapTo(&w, QPoint()), directionRow->size());
        const QRect buttonRect(directionButton->mapTo(&w, QPoint()), directionButton->size());
        const QRect actionsRect(actions->mapTo(&w, QPoint()), actions->size());
        QCOMPARE(actionRect.left(), sessionRect.left());
        QCOMPARE(actionRect.width(), sessionRect.width());
        QVERIFY(actionRect.contains(directionRect));
        QVERIFY(actionRect.contains(buttonRect));
        QVERIFY(actionRect.contains(actionsRect));
        QVERIFY(actionsRect.top() > directionRect.bottom());
        QVERIFY(directionButton->width() >= directionButton->sizeHint().width());
      }
    }
  }
  void layoutCanBePreparedBeforeIdentityOrStart()
  {
    Home w;
    Home::State state;
    state.role = Home::Role::Share;
    w.setState(state);
    w.show();
    QSignalSpy layout(&w, &Home::layoutRequested), start(&w, &Home::startStopRequested),
        identity(&w, &Home::identitySetupRequested);
    QVERIFY(b(w, "homeLayout")->isVisible());
    b(w, "homeLayout")->click();
    QCOMPARE(layout.count(), 1);
    QCOMPARE(start.count(), 0);
    QCOMPARE(identity.count(), 0);
  }
  void serverLayoutRemainsReachableWhileSharing_data()
  {
    QTest::addColumn<Home::Stage>("stage");
    QTest::newRow("listening") << Home::Stage::Listening;
    QTest::newRow("connected") << Home::Stage::Connected;
    QTest::newRow("retrying") << Home::Stage::Retrying;
    QTest::newRow("recovering") << Home::Stage::Recovering;
  }
  void serverLayoutRemainsReachableWhileSharing()
  {
    QFETCH(Home::Stage, stage);
    Home w;
    Home::State state;
    state.role = Home::Role::Share;
    state.stage = stage;
    state.identityReady = true;
    state.canStart = true;
    w.setState(state);
    w.show();
    QSignalSpy layout(&w, &Home::layoutRequested), start(&w, &Home::startStopRequested),
        identity(&w, &Home::identitySetupRequested);
    auto *arrange = b(w, "homeLayout");
    QVERIFY(arrange->isVisible());
    QVERIFY(arrange->isEnabled());
    QTest::mouseClick(arrange, Qt::LeftButton);
    QCOMPARE(layout.count(), 1);
    QCOMPARE(start.count(), 0);
    QCOMPARE(identity.count(), 0);
    QCOMPARE(w.state().stage, stage);
  }
  void readableRelationshipAndActionsAcrossSizes()
  {
    Home w;
    auto state = client();
    state.hasAddress = true;
    w.setState(state);
    w.show();
    for (const auto size : {QSize(620, 480), QSize(760, 560), QSize(1920, 960)}) {
      w.resize(size);
      QTest::qWait(1);
      auto *column = w.findChild<QWidget *>("homeColumn");
      QVERIFY(column);
      QVERIFY(column->width() <= 880);
      QVERIFY(column->width() >= 550);
      auto *local = w.findChild<QWidget *>("homeLocalDevice");
      auto *peer = w.findChild<QWidget *>("homePeerDevice");
      const QPoint localCenter = local->mapTo(&w, local->rect().center());
      const QPoint peerCenter = peer->mapTo(&w, peer->rect().center());
      QVERIFY(qAbs(peerCenter.x() - localCenter.x()) <= 4);
      QVERIFY(peerCenter.y() - localCenter.y() > 40);
      QVERIFY(peerCenter.y() - localCenter.y() < 220);
      auto *footer = w.findChild<QWidget *>("homeFooter");
      auto *actions = w.findChild<QWidget *>("homeActions");
      if (footer->isVisible()) {
        const int gap = footer->mapTo(&w, QPoint()).y() - actions->mapTo(&w, QPoint(0, actions->height())).y();
        QVERIFY2(gap >= 0 && gap < 65, qPrintable(QStringLiteral("actions-to-footer gap: %1").arg(gap)));
      }
      auto *scroll = w.findChild<QScrollArea *>("homeScroll");
      QCOMPARE(scroll->horizontalScrollBar()->maximum(), 0);
      QVERIFY(column->mapTo(&w, QPoint()).x() >= 0);
    }
  }
  void relationshipGlyphsMatchKeyboardMouseAndDisplayRoles()
  {
    Home w;
    Home::State state;
    state.role = Home::Role::Share;
    state.localName = "Test Mac";
    state.peerName = "Test PC";
    w.setState(state);
    w.resize(1024, 740);
    w.show();
    QTest::qWait(1);
    auto *local = w.findChild<QWidget *>("homeLocalDevice")->findChild<QWidget *>("computerGlyph");
    auto *peer = w.findChild<QWidget *>("homePeerDevice")->findChild<QWidget *>("computerGlyph");
    QVERIFY(local);
    QVERIFY(peer);
    QCOMPARE(local->property("deviceKind").toString(), QString("input"));
    QCOMPARE(peer->property("deviceKind").toString(), QString("display"));

    state.role = Home::Role::Receive;
    w.setState(state);
    QApplication::processEvents();
    QCOMPARE(local->property("deviceKind").toString(), QString("display"));
    QCOMPARE(peer->property("deviceKind").toString(), QString("input"));
  }
  void reconnectStatusDoesNotChangeSessionGeometry()
  {
    Home w;
    auto state = client();
    state.hasAddress = true;
    state.identityReady = true;
    state.canStart = true;
    w.resize(1024, 740);
    w.show();
    state.stage = Home::Stage::Retrying;
    state.retrySeconds = 9;
    w.setState(state);
    QApplication::processEvents();
    auto *session = w.findChild<QWidget *>("homeSession");
    const auto retryHeight = session->height();
    state.stage = Home::Stage::Verifying;
    state.retrySeconds = -1;
    w.setState(state);
    QApplication::processEvents();
    QCOMPARE(session->height(), retryHeight);
    state.stage = Home::Stage::Recovering;
    state.retrySeconds = 8;
    w.setState(state);
    QApplication::processEvents();
    QCOMPARE(session->height(), retryHeight);
    QCOMPARE(w.findChild<QLabel *>("homeStatus")->text(), QString("Reconnecting and checking identity"));
  }
  void chineseRecoveryCopyAndGeometryStayStable()
  {
    QTranslator translator;
    QVERIFY(translator.load(":/translations/deskflow_zh_CN.qm"));
    QVERIFY(QCoreApplication::installTranslator(&translator));
    Home w;
    auto state = client();
    state.hasAddress = true;
    state.identityReady = true;
    state.canStart = true;
    state.stage = Home::Stage::Retrying;
    state.retrySeconds = 9;
    w.setState(state);
    w.resize(1024, 740);
    w.show();
    QTest::qWait(1);
    auto *session = w.findChild<QWidget *>("homeSession");
    auto *status = w.findChild<QLabel *>("homeStatus");
    auto *statusRow = w.findChild<QWidget *>("homeStatusRow");
    const auto retryGeometry = session->geometry();
    const auto statusRowHeight = statusRow->height();
    QCOMPARE(status->text(), QString::fromUtf8("正在重连并核验身份"));
    state.stage = Home::Stage::Recovering;
    state.retrySeconds = 8;
    w.setState(state);
    QApplication::processEvents();
    QCOMPARE(session->geometry(), retryGeometry);
    QCOMPARE(statusRow->height(), statusRowHeight);
    QCOMPARE(status->text(), QString::fromUtf8("正在重连并核验身份"));
    QCOMPARE(w.findChild<QLabel *>("homeDirectionCaption")->text(), QString::fromUtf8("键鼠控制"));
    QCOMPARE(w.findChild<QLabel *>("homeDirectionSummary")->text(), QString::fromUtf8("另一台电脑控制本机"));
    QCOMPARE(w.findChild<QPushButton *>("homeChangeDirection")->text(), QString::fromUtf8("改由本机控制"));
    QCoreApplication::removeTranslator(&translator);
  }
  void wideLayoutUsesTheSharedSidebarAndKeepsTheMainColumnBounded()
  {
    Home w;
    auto state = client();
    state.hasAddress = true;
    state.identityReady = true;
    state.canStart = true;
    w.setState(state);
    w.resize(1024, 740);
    w.show();
    QTest::qWait(1);
    auto *sidebar = w.findChild<QWidget *>("homeSidebar");
    QVERIFY(sidebar);
    QVERIFY(sidebar->isVisible());
    QVERIFY(w.findChild<QLabel *>("sectionLabel")->text() == QString("Connection"));
    QVERIFY(w.findChild<QWidget *>("homeColumn")->width() <= 880);
    QVERIFY(b(w, "sidebarOverview")->isVisible());
    QVERIFY(b(w, "sidebarComputers")->isVisible());
    QVERIFY(b(w, "sidebarConnection")->isVisible());
    QVERIFY(b(w, "sidebarSettings")->isVisible());
    QVERIFY(b(w, "sidebarDiagnostics")->isVisible());
    QTRY_COMPARE_WITH_TIMEOUT(w.findChild<QScrollArea *>("homeScroll")->horizontalScrollBar()->maximum(), 0, 1000);
    w.resize(760, 560);
    QTest::qWait(1);
    QVERIFY(!sidebar->isVisible());
    QVERIFY(w.findChild<QWidget *>("homeNavigation")->isVisible());
  }
  void tabOrderAndSpaceSelectionFollowTheVisibleRoles()
  {
    Home w;
    w.resize(760, 560);
    w.show();
    w.activateWindow();
    QTest::qWait(1);
    qInfo() << "tab_focus_behavior" << int(qApp->styleHints()->tabFocusBehavior());
    auto *share = b(w, "homeShare");
    auto *receive = b(w, "homeReceive");
    share->setFocus();
    QSignalSpy roles(&w, &Home::roleChosen);
    QTest::keyClick(share, Qt::Key_Tab);
    if (qApp->styleHints()->tabFocusBehavior() == Qt::TabFocusAllControls) {
      QVERIFY(receive->hasFocus());
      QTest::keyClick(receive, Qt::Key_Backtab);
      QVERIFY(share->hasFocus());
    }
    receive->setFocus();
    QTest::keyClick(receive, Qt::Key_Space);
    QCOMPARE(roles.count(), 1);
  }
  void selectorAndWindowRestorationUseOrdinaryQtState()
  {
    QWidget window;
    auto *layout = new QVBoxLayout(&window);
    auto *selector = new QComboBox(&window);
    selector->addItems({"first-run", "receive", "manual"});
    layout->addWidget(selector);
    auto *home = new Home(&window);
    layout->addWidget(home);
    window.resize(760, 560);
    window.show();
    window.activateWindow();
    QTest::qWait(1);
    QSignalSpy changed(selector, &QComboBox::currentIndexChanged);
    selector->setFocus();
    QTest::keyClick(selector, Qt::Key_Down);
    QCOMPARE(selector->currentIndex(), 1);
    QCOMPARE(changed.count(), 1);
    selector->showPopup();
    QVERIFY(selector->view()->isVisible());
    QTest::keyClick(selector->view(), Qt::Key_Escape);
    QTRY_VERIFY(!selector->view()->isVisible());
    QCOMPARE(selector->currentIndex(), 1);
    const auto normal = window.size();
    window.showMaximized();
    QTest::qWait(1);
    QVERIFY(window.isMaximized());
    window.showNormal();
    QTest::qWait(1);
    QVERIFY(!window.isMaximized());
    QCOMPARE(window.size(), normal);
  }
  void noRoleHasNoImplicitSideEffects()
  {
    Home w;
    w.resize(760, 560);
    w.show();
    QSignalSpy start(&w, &Home::startStopRequested), identity(&w, &Home::identitySetupRequested),
        role(&w, &Home::roleChosen);
    QVERIFY(b(w, "homeShare")->isVisible());
    QVERIFY(!b(w, "homePrimary")->isVisible());
    w.showManualEntry();
    QVERIFY(b(w, "homeReceive")->isVisible());
    QCOMPARE(start.count(), 0);
    QCOMPARE(identity.count(), 0);
    QCOMPARE(role.count(), 0);
    b(w, "homeReceive")->click();
    QCOMPARE(role.count(), 1);
    QCOMPARE(start.count(), 0);
  }
  void addressSubmitNeverConnects()
  {
    Home w;
    w.setState(client());
    w.show();
    QSignalSpy address(&w, &Home::addressChosen), start(&w, &Home::startStopRequested),
        identity(&w, &Home::identitySetupRequested);
    w.showManualEntry();
    auto *edit = w.findChild<QLineEdit *>("homeAddress");
    QVERIFY(edit->isVisible());
    edit->setText("https://wrong.example/");
    QTest::keyClick(edit, Qt::Key_Return);
    QCOMPARE(address.count(), 0);
    QVERIFY(!w.findChild<QLabel *>("homeAddressError")->text().isEmpty());
    edit->setText(" work-pc.local ");
    QTest::keyClick(edit, Qt::Key_Return);
    QCOMPARE(address.count(), 1);
    QCOMPARE(address.at(0).at(0).toString(), QString("work-pc.local"));
    QCOMPARE(start.count(), 0);
    QCOMPARE(identity.count(), 0);
  }
  void cancelAndReturnDiscardDraft()
  {
    Home w;
    w.setState(client());
    w.show();
    QSignalSpy address(&w, &Home::addressChosen), role(&w, &Home::roleChosen);
    w.showManualEntry();
    auto *edit = w.findChild<QLineEdit *>("homeAddress");
    edit->setText("discard-me.invalid");
    b(w, "homeBack")->click();
    QCOMPARE(address.count(), 0);
    QVERIFY(edit->text().isEmpty());
    QVERIFY(!edit->isVisible());
    w.showRoleSelection();
    b(w, "homeBack")->click();
    QCOMPARE(role.count(), 0);
    QCOMPARE(w.state().role, Home::Role::Receive);
  }
  void missingIdentityIsExplicit()
  {
    Home w;
    auto s = client();
    s.hasAddress = true;
    s.canStart = true;
    w.setState(s);
    w.show();
    QSignalSpy identity(&w, &Home::identitySetupRequested), start(&w, &Home::startStopRequested);
    b(w, "homePrimary")->click();
    QCOMPARE(identity.count(), 1);
    QCOMPARE(start.count(), 0);
  }
  void repeatedStartAndStopAreBounded()
  {
    Home w;
    auto s = client();
    s.hasAddress = true;
    s.identityReady = true;
    s.canStart = true;
    w.setState(s);
    w.show();
    QSignalSpy start(&w, &Home::startStopRequested);
    b(w, "homePrimary")->click();
    b(w, "homePrimary")->click();
    QCOMPARE(start.count(), 1);
    s.stage = Home::Stage::Connecting;
    w.setState(s);
    QVERIFY(!b(w, "homeManual")->isVisible());
    b(w, "homePrimary")->click();
    b(w, "homePrimary")->click();
    QCOMPARE(start.count(), 2);
    s.stage = Home::Stage::Stopping;
    w.setState(s);
    QVERIFY(!b(w, "homePrimary")->isEnabled());
  }
  void needsAttentionDoesNotReconnect()
  {
    Home w;
    auto s = client();
    s.stage = Home::Stage::NeedsAction;
    s.explanation = "Identity changed";
    w.setState(s);
    w.show();
    QSignalSpy start(&w, &Home::startStopRequested), review(&w, &Home::diagnosticsRequested);
    b(w, "homePrimary")->click();
    QCOMPARE(review.count(), 1);
    QCOMPARE(start.count(), 0);
    QCOMPARE(w.findChild<QLabel *>("homeReason")->text(), s.explanation);
  }
  void cancelledStartCanBeTriedAgain()
  {
    Home w;
    auto s = client();
    s.hasAddress = true;
    s.identityReady = true;
    s.canStart = true;
    w.setState(s);
    w.show();
    QSignalSpy start(&w, &Home::startStopRequested);
    b(w, "homePrimary")->click();
    QCOMPARE(start.count(), 1);
    w.setState(s); // The controller reports that a start was cancelled.
    QVERIFY(b(w, "homePrimary")->isEnabled());
    b(w, "homePrimary")->click();
    QCOMPARE(start.count(), 2);
  }
  void connectedIsOnlyAnExplicitState()
  {
    Home w;
    auto s = client();
    s.hasAddress = true;
    s.identityReady = true;
    s.canStart = true;
    w.setState(s);
    QVERIFY(w.findChild<QLabel *>("homeStatus")->text() != QString("Connected"));
    s.stage = Home::Stage::Connected;
    w.setState(s);
    QCOMPARE(w.findChild<QLabel *>("homeStatus")->text(), QString("Connected"));
  }
  void emptySavedDevicesHasNoFalseStopInstruction()
  {
    deskflow::gui::IdentityBindings empty;
    TrustedDevicesDialog dialog(empty, false);
    dialog.show();
    for (auto *l : dialog.findChildren<QLabel *>())
      if (l->isVisible())
        QVERIFY2(!l->text().contains("Stop the current connection"), qPrintable(l->text()));
    QVERIFY(!dialog.findChild<QPushButton *>("configureAutomaticRecovery")->isVisible());
  }
  void keyboardCanSelectDirection()
  {
    Home w;
    w.show();
    QSignalSpy role(&w, &Home::roleChosen);
    auto *receive = b(w, "homeReceive");
    receive->setFocus();
    QTest::keyClick(receive, Qt::Key_Space);
    QCOMPARE(role.count(), 1);
    QCOMPARE(role.at(0).at(0).value<Home::Role>(), Home::Role::Receive);
  }
  void compactFirstRunKeepsChoicesInView()
  {
    Home w;
    w.resize(620, 448);
    w.show();
    QTest::qWait(1);
    auto *scroll = w.findChild<QScrollArea *>("homeScroll");
    QVERIFY(scroll);
    const auto viewport = scroll->viewport()->rect();
    for (const char *name : {"homeShare", "homeReceive"}) {
      auto *choice = b(w, name);
      const auto topLeft = choice->mapTo(scroll->viewport(), QPoint(0, 0));
      QVERIFY(viewport.contains(QRect(topLeft, choice->size())));
    }
    QCOMPARE(scroll->horizontalScrollBar()->maximum(), 0);
  }
  void escapeDiscardsAddressAndReturnsKeyboardFocus()
  {
    Home w;
    w.setState(client());
    w.resize(760, 560);
    w.show();
    w.activateWindow();
    QTest::qWait(1);
    QSignalSpy address(&w, &Home::addressChosen), role(&w, &Home::roleChosen);
    w.showManualEntry();
    auto *edit = w.findChild<QLineEdit *>("homeAddress");
    edit->setText("discard.invalid");
    QTest::keyClick(edit, Qt::Key_Escape);
    QVERIFY(!edit->isVisible());
    QVERIFY(edit->text().isEmpty());
    QCOMPARE(address.count(), 0);
    QVERIFY(b(w, "homeManual")->hasFocus());
    w.showRoleSelection();
    QTest::keyClick(b(w, "homeShare"), Qt::Key_Escape);
    QCOMPARE(role.count(), 0);
    QCOMPARE(w.state().role, Home::Role::Receive);
    QVERIFY(!b(w, "homeShare")->isVisible());
  }
  void manualFormKeepsReadableWidthAcrossWindowSizes()
  {
    QWidget window;
    window.setMinimumSize(620, 480);
    auto *layout = new QVBoxLayout(&window);
    layout->setContentsMargins(0, 0, 0, 0);
    Home w;
    layout->addWidget(&w);
    w.setState(client());
    window.show();
    w.showManualEntry();
    for (const auto size : {QSize(620, 480), QSize(760, 560), QSize(1024, 740)}) {
      window.resize(size);
      QTest::qWait(1);
      const int expected = qMin(560, size.width() - 64);
      auto *title = w.findChild<QLabel *>("homeTitle");
      auto *address = w.findChild<QLineEdit *>("homeAddress");
      QCOMPARE(title->width(), expected);
      QVERIFY(address->width() >= expected - 2);
      auto *viewport = w.findChild<QScrollArea *>("homeScroll")->viewport();
      for (auto *part :
           {static_cast<QWidget *>(title), static_cast<QWidget *>(address),
            static_cast<QWidget *>(b(w, "homePrimary"))}) {
        const auto rect = QRect(part->mapTo(viewport, QPoint()), part->size());
        QVERIFY(viewport->rect().contains(rect));
      }
    }
  }
};
QTEST_MAIN(HomeTests)
#include "HomeTests.moc"
