// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
#include "../WindowsOffscreenFontSupport.h"
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "gui/config/MonitorLayoutDraft.h"
#include "gui/dialogs/DesktopLayoutEditor.h"
#include "gui/widgets/DesktopWorkspaceView.h"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QLabel>
#include <QPushButton>
#include <QScopeGuard>
#include <QScrollArea>
#include <QSignalSpy>
#include <QStyleOptionButton>
#include <QTest>
#include <QTranslator>
using namespace deskflow::display;
using deskflow::gui::DesktopWorkspaceView;
namespace {
DeviceSnapshots fixtures(bool oldPeer = false, bool cached = false)
{
  DeviceSnapshot win;
  win.id = "Windows";
  win.displayName = "工作电脑 Windows";
  win.local = true;
  win.connection = Connection::Live;
  win.observation = Observation::Observed;
  win.topologyCapability = Capability::Supported;
  win.routingCapability = Capability::Supported;
  win.topology = Topology{
      {{"a", {0, 0, 2560, 1440}, {0, 0, 3840, 2160}, {3840, 2160}, {}},
       {"b", {3840, 132, 1463, 914}, {3840, 132, 2560, 1600}, {2560, 1600}, {}}}
  };
  DeviceSnapshot mac;
  mac.id = "Mac";
  mac.displayName = "Mac mini";
  mac.authenticated = true;
  mac.connection = cached ? Connection::Offline : Connection::Live;
  mac.observation = oldPeer ? Observation::Unsupported : cached ? Observation::Cached : Observation::Observed;
  mac.topologyCapability = oldPeer ? Capability::Unsupported : Capability::Supported;
  mac.routingCapability = mac.topologyCapability;
  if (!oldPeer)
    mac.topology = Topology{{{"m", {0, 0, 1920, 1080}, {0, 0, 1920, 1080}, {3840, 2160}, {}}}};
  return {win, mac};
}
void show(DesktopWorkspaceView &view, bool old = false, bool cached = false)
{
  view.setSnapshot(fixtures(old, cached));
  view.resize(760, 720);
  view.show();
  QApplication::processEvents();
}
} // namespace
class WorkspaceViewTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase()
  {
    const auto scale = qEnvironmentVariable("QT_SCALE_FACTOR").toDouble();
    if (scale > 0)
      QVERIFY(qAbs(qApp->devicePixelRatio() - scale) < 0.01);
  }
  void oldMacAndActualWindowsDualAreOneSelectableCanvas()
  {
    DesktopWorkspaceView view;
    show(view, true);
    QCOMPARE(view.workspace().displays().size(), 2);
    auto *picker = view.findChild<QComboBox *>("workspaceSelection");
    QCOMPARE(picker->count(), 3);
    QVERIFY(!view.findChild<QPushButton *>("inspectLocalDisplays"));
    QTest::mouseClick(view.canvas(), Qt::LeftButton, Qt::NoModifier, view.canvas()->unknownDeviceRect("Mac").center());
    QVERIFY(view.findChild<QLabel *>("workspaceInspector")->text().contains("update this computer"));
    QVERIFY(!view.findChild<QLabel *>("workspaceInspector")->text().contains("1440"));
    QVERIFY(view.findChild<QLabel *>("workspacePhysicalNote")->text().contains("unknown"));
    const auto output = qEnvironmentVariable("WORKSPACE_SCREENSHOTS");
    if (!output.isEmpty()) {
      QDir().mkpath(output);
      QVERIFY(view.grab().save(output + "/known-two-screens-old-mac.png"));
    }
  }
  void selectionAndGroupKeyboardMoveCancelNeverApply()
  {
    DesktopWorkspaceView view;
    show(view);
    const auto original = view.workspace().state().encode();
    QSignalSpy apply(&view, &DesktopWorkspaceView::applyRequested),
        save(&view, &DesktopWorkspaceView::saveDraftRequested), cancel(&view, &DesktopWorkspaceView::cancelRequested);
    const auto token = deskflow::gui::MonitorLayoutDraft::token({"Windows", "a", {}});
    QTest::mouseClick(
        view.canvas(), Qt::LeftButton, Qt::NoModifier,
        view.canvas()->desktopRect(token.toStdString()).center().toPoint()
    );
    QCOMPARE(view.workspace().state().encode(), original);
    QTest::mouseClick(view.findChild<QPushButton *>("workspaceEdit"), Qt::LeftButton);
    QTest::keyClick(view.canvas(), Qt::Key_Left);
    QVERIFY(view.workspace().state().encode() != original);
    QTest::mouseClick(view.findChild<QPushButton *>("workspaceCancel"), Qt::LeftButton);
    QCOMPARE(view.workspace().state().encode(), original);
    QCOMPARE(cancel.count(), 1);
    QCOMPARE(apply.count(), 0);
    QCOMPARE(save.count(), 0);
  }
  void disconnectedDraftCannotClaimLiveRouting()
  {
    DesktopWorkspaceView view;
    view.setSubmitAvailable(true);
    show(view, false, true);
    QSignalSpy apply(&view, &DesktopWorkspaceView::applyRequested),
        save(&view, &DesktopWorkspaceView::saveDraftRequested);
    QTest::mouseClick(view.findChild<QPushButton *>("workspaceEdit"), Qt::LeftButton);
    auto *button = view.findChild<QPushButton *>("workspaceApply");
    QCOMPARE(button->text(), QString("Save draft"));
    QVERIFY(button->isEnabled());
    QTest::mouseClick(button, Qt::LeftButton);
    QCOMPARE(apply.count(), 0);
    QCOMPARE(save.count(), 1);
    QVERIFY(view.findChild<QLabel *>("workspaceActiveRouting")->text().contains("not been reported"));
  }
  void receiverHasNoEditorOrGrayControllerActions()
  {
    DesktopWorkspaceView view;
    show(view);
    view.setSubmitAvailable(true);
    view.setActionsAvailable(true, true);
    view.setRole(DesktopWorkspaceView::Role::Receiver, "工作电脑 Windows", false);
    const auto original = view.workspace().state().encode();
    QSignalSpy apply(&view, &DesktopWorkspaceView::applyRequested),
        save(&view, &DesktopWorkspaceView::saveDraftRequested);
    for (const auto *name :
         {"workspaceAdd", "workspaceIdentify", "workspaceEdit", "workspaceApply", "workspaceUndo", "workspaceCancel",
          "workspaceDetails"})
      QVERIFY2(!view.findChild<QPushButton *>(name)->isVisible(), name);
    QVERIFY(!view.findChild<QScrollArea *>("workspaceScroll")->isVisible());
    QVERIFY(view.findChild<QLabel *>("workspaceReceiverSummary")->text().contains("Windows"));
    view.beginEdit();
    QCOMPARE(view.workspace().state().encode(), original);
    QVERIFY(!view.findChild<QPushButton *>("workspaceApply")->isVisible());
    view.setRole(DesktopWorkspaceView::Role::Receiver, "工作电脑 Windows", true);
    QTest::mouseClick(view.findChild<QPushButton *>("workspaceDetails"), Qt::LeftButton);
    QVERIFY(view.findChild<QScrollArea *>("workspaceScroll")->isVisible());
    const auto token = deskflow::gui::MonitorLayoutDraft::token({"Windows", "a", {}});
    const auto center = view.canvas()->desktopRect(token.toStdString()).center().toPoint();
    QTest::mousePress(view.canvas(), Qt::LeftButton, Qt::NoModifier, center);
    QTest::mouseRelease(view.canvas(), Qt::LeftButton, Qt::NoModifier, center + QPoint(-30, 20));
    QTest::keyClick(view.canvas(), Qt::Key_Right);
    QCOMPARE(view.workspace().state().encode(), original);
    QCOMPARE(apply.count(), 0);
    QCOMPARE(save.count(), 0);
    const auto output = qEnvironmentVariable("WORKSPACE_SCREENSHOTS");
    if (!output.isEmpty()) {
      QTest::mouseClick(view.findChild<QPushButton *>("workspaceDetails"), Qt::LeftButton);
      QVERIFY(view.grab().save(output + "/receiver-summary.png"));
    }
  }
  void pointerDragNearEdgeSnapsTheWholeDevice()
  {
    DesktopWorkspaceView view;
    show(view);
    view.beginEdit();
    const auto before = view.workspace().displays();
    const auto token = deskflow::gui::MonitorLayoutDraft::token({"Windows", "a", {}});
    const auto center = view.canvas()->desktopRect(token.toStdString()).center().toPoint();
    QTest::mousePress(view.canvas(), Qt::LeftButton, Qt::NoModifier, center);
    QTest::mouseRelease(view.canvas(), Qt::LeftButton, Qt::NoModifier, center + QPoint(-6, 4));
    const auto after = view.workspace().displays();
    QVERIFY(view.workspace().candidateForApply());
    QPointF delta;
    for (int i = 0; i < before.size(); ++i)
      if (before[i].stableId == "a")
        delta = after[i].canvasRect.topLeft() - before[i].canvasRect.topLeft();
    QVERIFY(qAbs(delta.x()) < 1e-8);
    QVERIFY(delta.y() > 0);
    for (int i = 0; i < before.size(); ++i)
      if (before[i].deviceId == "Windows")
        QVERIFY(QLineF(after[i].canvasRect.topLeft() - before[i].canvasRect.topLeft(), delta).length() < 1e-8);
  }
  void unwiredAndExternalConfigurationAreHonest()
  {
    DesktopWorkspaceView view;
    show(view);
    QTest::mouseClick(view.findChild<QPushButton *>("workspaceEdit"), Qt::LeftButton);
    QVERIFY(!view.findChild<QPushButton *>("workspaceApply")->isEnabled());
    QVERIFY(view.findChild<QLabel *>("workspaceDraftStatus")->text().contains("Preview only"));
    view.setSubmitAvailable(true);
    view.setReadOnlyReason("External configuration is active; edit its file to change routes.");
    QSignalSpy apply(&view, &DesktopWorkspaceView::applyRequested);
    QVERIFY(!view.findChild<QPushButton *>("workspaceApply")->isEnabled());
    const auto before = view.workspace().state().encode();
    QTest::keyClick(view.canvas(), Qt::Key_Right);
    QCOMPARE(view.workspace().state().encode(), before);
    QCOMPARE(apply.count(), 0);
  }
  void successfulSignalDoesNotPretendPersistenceOrActivation()
  {
    DesktopWorkspaceView view;
    show(view);
    view.setSubmitAvailable(true);
    QSignalSpy apply(&view, &DesktopWorkspaceView::applyRequested);
    QTest::mouseClick(view.findChild<QPushButton *>("workspaceEdit"), Qt::LeftButton);
    QTest::mouseClick(view.findChild<QPushButton *>("workspaceApply"), Qt::LeftButton);
    QCOMPARE(apply.count(), 1);
    QVERIFY(view.findChild<QPushButton *>("workspaceCancel")->isVisible());
    QVERIFY(view.findChild<QLabel *>("workspaceActiveRouting")->text().contains("not been reported"));
    const auto args = apply.takeFirst();
    QVERIFY(WorkspaceState::decode(args[0].toByteArray()));
    QVERIFY(Layout::decode(args[1].toByteArray()));
    view.acknowledgeSaved(args[0].toByteArray(), args[1].toByteArray());
    QVERIFY(!view.findChild<QPushButton *>("workspaceCancel")->isVisible());
  }
  void duplicateApplyWaitsForAcknowledgmentAndFailureKeepsDraft()
  {
    DesktopWorkspaceView view;
    show(view);
    view.setSubmitAvailable(true);
    view.beginEdit();
    QSignalSpy apply(&view, &DesktopWorkspaceView::applyRequested);
    auto *button = view.findChild<QPushButton *>("workspaceApply");
    const auto before = view.workspace().state().encode();
    QTest::mouseClick(button, Qt::LeftButton);
    QTest::mouseClick(button, Qt::LeftButton);
    QCOMPARE(apply.count(), 1);
    QVERIFY(!button->isEnabled());
    QVERIFY(!view.findChild<QPushButton *>("workspaceCancel")->isEnabled());
    QTest::keyClick(view.canvas(), Qt::Key_Right);
    QCOMPARE(view.workspace().state().encode(), before);
    view.rejectSave("The file could not be saved. Your draft is retained.");
    QVERIFY(button->isEnabled());
    QCOMPARE(view.workspace().state().encode(), before);
    QVERIFY(view.findChild<QLabel *>("workspaceDraftStatus")->text().contains("could not be saved"));
    QTest::mouseClick(button, Qt::LeftButton);
    QCOMPARE(apply.count(), 2);
  }
  void interfaceFitsWindowAndButtonContents_data()
  {
    QTest::addColumn<int>("width");
    QTest::addColumn<bool>("dark");
    QTest::addColumn<bool>("chinese");
    for (int width : {620, 860})
      for (bool dark : {false, true})
        for (bool chinese : {false, true})
          QTest::newRow(qPrintable(QString("%1-%2-%3").arg(width).arg(dark).arg(chinese))) << width << dark << chinese;
  }
  void interfaceFitsWindowAndButtonContents()
  {
    QFETCH(int, width);
    QFETCH(bool, dark);
    QFETCH(bool, chinese);
    const auto old = qApp->palette();
    auto palette = old;
    palette.setColor(QPalette::Window, dark ? QColor("#202124") : QColor("#fafafa"));
    qApp->setPalette(palette);
    QTranslator translator;
    if (chinese) {
      QVERIFY(translator.load(":/translations/deskflow_zh_CN.qm"));
      qApp->installTranslator(&translator);
    }
    const auto reset = qScopeGuard([&] {
      qApp->removeTranslator(&translator);
      qApp->setPalette(old);
    });
    DesktopWorkspaceView view;
    show(view);
    view.resize(width, 720);
    view.setSubmitAvailable(true);
    view.beginEdit();
    QApplication::processEvents();
    if (chinese)
      QCOMPARE(view.findChild<QPushButton *>("workspaceApply")->text(), QString::fromUtf8("应用排列"));
    QCOMPARE(view.width(), width);
    for (const auto *button : view.findChildren<QPushButton *>()) {
      if (!button->isVisible())
        continue;
      QStyleOptionButton option;
      option.initFrom(button);
      option.text = button->text();
      option.rect = button->rect();
      const auto contents = button->style()->subElementRect(QStyle::SE_PushButtonContents, &option, button);
      QVERIFY2(
          contents.width() + .5 >= button->fontMetrics().horizontalAdvance(button->text()), qPrintable(button->text())
      );
      QVERIFY2(contents.height() + .5 >= button->fontMetrics().height(), qPrintable(button->text()));
      const auto left = button->mapTo(&view, QPoint()).x();
      QVERIFY2(left >= 0 && left + button->width() <= view.width(), qPrintable(button->text()));
    }
    const auto output = qEnvironmentVariable("WORKSPACE_SCREENSHOTS");
    if (!output.isEmpty() && width == 860) {
      QDir().mkpath(output);
      QVERIFY(view.grab().save(
          output + QString("/workspace-%1-%2.png").arg(chinese ? "zh" : "en").arg(dark ? "dark" : "light")
      ));
    }
  }
};
int main(int argc, char **argv)
{
  QApplication app(argc, argv);
  app.setStyle("Fusion");
  if (!prepareWindowsOffscreenFont(app))
    return 6;
  WorkspaceViewTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "WorkspaceViewTests.moc"
