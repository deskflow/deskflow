// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/DesktopWorkspace.h"
#include "common/IpcLineBuffer.h"
#include "common/WorkspaceApply.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QTest>
#include <algorithm>
#include <cstdio>
using namespace deskflow::display;
namespace {
Topology windowsDual()
{
  // Real kinds of coordinate spaces reported at 150%/175%. Qt's origin is
  // intentionally not divided by the individual screen's scale (the island).
  return {
      {{"w-a", {0, 0, 2560, 1440}, {0, 0, 3840, 2160}, {3840, 2160}, {}},
       {"w-b", {-2560, 320, 1463, 914}, {-2560, 320, 2560, 1600}, {2560, 1600}, {}}}
  };
}
Topology macSingle()
{
  return {{{"m-a", {0, 0, 1920, 1080}, {0, 0, 1920, 1080}, {3840, 2160}, {}}}};
}
Topology macDual()
{
  return {
      {{"w-a", {0, 0, 2560, 1440}, {0, 0, 2560, 1440}, {5120, 2880}, {}},
       {"w-b", {-1600, 100, 1600, 1000}, {-1600, 100, 1600, 1000}, {3200, 2000}, {}}}
  };
}
Topology windowsSingle()
{
  return {{{"m-a", {0, 0, 1920, 1080}, {0, 0, 2880, 1620}, {2880, 1620}, {}}}};
}
DeviceSnapshot ready(QString id, Topology topology, bool local = false)
{
  DeviceSnapshot d;
  d.id = id;
  d.displayName = id;
  d.local = local;
  d.authenticated = !local;
  d.connection = Connection::Live;
  d.observation = Observation::Observed;
  d.topologyCapability = Capability::Supported;
  d.routingCapability = Capability::Supported;
  d.topology = std::move(topology);
  return d;
}
DeviceSnapshots pair()
{
  return {ready("Windows", windowsDual(), true), ready("Mac", macSingle())};
}
WorkspaceDisplay screen(const DesktopWorkspace &model, const QString &id)
{
  for (const auto &s : model.displays())
    if (s.stableId == id)
      return s;
  return {};
}
int child(const QString &path, bool write)
{
  if (write) {
    DesktopWorkspace model(pair());
    if (!model.moveDevice("Mac", {0, 120}))
      return 10;
    if (!model.calibratePhysicalSize("Windows", "w-a", QSizeF(700, 390)))
      return 11;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
      return 12;
    const auto bytes = model.state().encode();
    return file.write(bytes) == bytes.size() ? 0 : 13;
  }
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return 14;
  DesktopWorkspace model(pair(), file.readAll());
  if (!model.error().isEmpty())
    return 15;
  const auto s = screen(model, "m-a"), w = screen(model, "w-a");
  const auto bytes = QJsonDocument(
                         QJsonObject{
                             {"count", int(model.displays().size())},
                             {"macY", s.canvasRect.y()},
                             {"winNumber", w.number},
                             {"calibrated", w.calibrated},
                             {"widthMm", w.physicalMm ? w.physicalMm->width() : 0},
                             {"valid", model.validationError().isEmpty()}
                         }
  ).toJson(QJsonDocument::Compact);
  std::fwrite(bytes.constData(), 1, bytes.size(), stdout);
  return 0;
}
} // namespace
class DesktopWorkspaceTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void restartKeepsLastConfirmedPlanAndNeverActivatesPendingDraft()
  {
    DesktopWorkspace model(pair());
    auto confirmed = model.state();
    confirmed.revision = 1;
    QVERIFY(model.moveDevice("Mac", {0, 120}));
    auto draft = model.state();
    draft.revision = 2;
    const auto previous = workspaceLayoutOnRestart(confirmed.encode(), draft.encode(), draft.layout.encode(), false);
    QVERIFY(previous);
    QCOMPARE(previous->encode(), confirmed.layout.encode());
    QVERIFY(!workspaceLayoutOnRestart({}, draft.encode(), draft.layout.encode(), false));
    QVERIFY(!workspaceLayoutOnRestart(confirmed.encode(), draft.encode(), draft.layout.encode(), true));
    const auto legacy = workspaceLayoutOnRestart({}, {}, confirmed.layout.encode(), false);
    QVERIFY(legacy);
    QCOMPARE(legacy->encode(), confirmed.layout.encode());
  }
  void certificateRenamePreservesPositionsNumbersAndInputGeometry()
  {
    auto devices = pair();
    devices[0].identityReference = QString(64, 'a');
    devices[1].identityReference = QString(64, 'b');
    DesktopWorkspace model(devices);
    QVERIFY(model.moveDevice("Mac", {0, 48}));
    const auto before = screen(model, "m-a");
    devices[1].id = "Renamed-Mac";
    devices[1].displayName = QString::fromUtf8("改名后的电脑");
    DesktopWorkspace renamed(devices, model.state().encode());
    const auto after = screen(renamed, "m-a");
    QVERIFY(renamed.conflicts().isEmpty());
    QCOMPARE(after.canvasRect, before.canvasRect);
    QCOMPARE(after.number, before.number);
    QCOMPARE(after.inputRect, before.inputRect);
    QCOMPARE(after.deviceId, QString("Renamed-Mac"));
    QVERIFY(renamed.candidateForApply());
  }
  void sameNameDifferentCertificateCannotClaimOrReviewOldLayout()
  {
    auto devices = pair();
    devices[1].identityReference = QString(64, 'a');
    DesktopWorkspace model(devices);
    devices[1].identityReference = QString(64, 'b');
    DesktopWorkspace changed(devices, model.state().encode());
    QCOMPARE(changed.conflicts().value("Mac"), QString("device-identity-changed"));
    QVERIFY(!changed.reviewOsLayout("Mac"));
    QVERIFY(!changed.candidateForApply());
  }
  void applyRequiresSavedPlanCurrentGenerationAndVerifiedEndpoints_data()
  {
    QTest::addColumn<QString>("mutation");
    for (const auto *c :
         {"valid", "session", "generation", "saved-plan", "offline", "old-peer", "identity", "topology", "external",
          "version", "draft-only"})
      QTest::newRow(c) << QString(c);
  }
  void applyRequiresSavedPlanCurrentGenerationAndVerifiedEndpoints()
  {
    QFETCH(QString, mutation);
    auto devices = pair();
    devices[0].identityReference = QString(64, 'a');
    devices[1].identityReference = QString(64, 'b');
    DesktopWorkspace model(devices);
    auto state = model.state();
    state.revision = 4;
    WorkspaceSnapshot live;
    live.devices = {"Windows", "Mac", "Old-Mac"};
    live.onlineDevices = {"Windows", "Mac"};
    live.topologyCapable = live.onlineDevices;
    live.topologies = model.topologies();
    live.identityReferences = state.identityReferences;
    live.session = QString(32, '1');
    live.generation = 3;
    WorkspaceApplyRequest request{QString(32, '2'), live.session, live.generation, state};
    auto saved = state.encode();
    if (mutation == "session")
      live.session = QString(32, '3');
    if (mutation == "generation")
      ++live.generation;
    if (mutation == "saved-plan")
      saved = "{}";
    if (mutation == "offline")
      live.onlineDevices.removeAll("Mac");
    if (mutation == "old-peer")
      live.topologyCapable.removeAll("Mac");
    if (mutation == "identity")
      live.identityReferences["Mac"] = QString(64, 'c');
    if (mutation == "topology")
      live.topologies["Mac"].monitors[0].input.setWidth(900);
    if (mutation == "external")
      live.externalConfig = true;
    if (mutation == "version")
      live.planRevision = 5;
    const auto error = validateWorkspaceApply(request, live, saved, mutation != "draft-only");
    QCOMPARE(error.isEmpty(), mutation == "valid");
    QVERIFY(WorkspaceApplyRequest::decode(request.encode()));
    WorkspaceApplyResult result{request.id,         request.session, workspacePlanId(state), error,
                                request.generation, state.revision,  error.isEmpty()};
    const auto decoded = WorkspaceApplyResult::decode(result.encode());
    QVERIFY(decoded);
    QVERIFY(decoded->matches(request));
    ++request.generation;
    QVERIFY(!decoded->matches(request));
  }
  void fragmentedApplicationMessageRetainsExactBytesWithoutOpeningIpc()
  {
    deskflow::ipc::LineBuffer buffer;
    QVector<QByteArray> lines;
    const QByteArray message = "applyWorkspace=" + QByteArray(13900, 'x');
    QVERIFY(buffer.append(message.left(7), lines));
    QVERIFY(lines.isEmpty());
    QVERIFY(buffer.append(message.mid(7), lines));
    QVERIFY(lines.isEmpty());
    QVERIFY(buffer.append("\nnoop\n", lines));
    QCOMPARE(lines, QVector<QByteArray>({message, "noop"}));
    lines.clear();
    QVERIFY(!buffer.append(QByteArray(65537, 'x'), lines));
    QVERIFY(lines.isEmpty());
  }
  void coreAvailabilityIsPerDeviceAndDoesNotRequireAllConfiguredPeers()
  {
    DesktopWorkspace model(pair());
    const auto plan = *model.candidateForApply();
    auto live = model.topologies();
    // An old peer has no DTOP entry. Its presence cannot alter valid routes.
    const auto projected = plan.available(live);
    QCOMPARE(projected.encode(), plan.encode());
    QVERIFY(!projected.routes(live).isEmpty());
    live.remove("Mac");
    const auto disconnected = plan.available(live);
    QCOMPARE(disconnected.placements.size(), 2);
    QVERIFY(disconnected.routes(live).isEmpty());
    live["Mac"] = macSingle();
    QCOMPARE(plan.available(live).encode(), plan.encode());
  }
  void topologyChangeWithdrawsOnlyAffectedDevices()
  {
    DesktopWorkspace model(pair());
    const auto plan = *model.candidateForApply();
    auto live = model.topologies();
    live["Mac"].monitors[0].input.setWidth(1700);
    const auto projected = plan.available(live);
    QCOMPARE(projected.placements.size(), 2);
    for (const auto &p : projected.placements)
      QCOMPARE(p.device, QString("Windows"));
    QVERIFY(!plan.error(live).isEmpty()); // A fresh submission is rejected.
  }
  void unavailableBridgeDoesNotRemoveUnrelatedValidEdge()
  {
    auto devices = pair();
    auto third = ready("Other", macSingle());
    devices.append(third);
    DesktopWorkspace model(devices);
    const auto plan = model.candidateForApply();
    QVERIFY(plan);
    auto live = model.topologies();
    live.remove("Other");
    const auto projected = plan->available(live);
    QCOMPARE(projected.placements.size(), 3);
    QVERIFY(!projected.routes(live).isEmpty());
    for (const auto &r : projected.routes(live))
      QVERIFY(r.device != "Other" && r.targetDevice != "Other");
  }
  void corruptOverlapNeverProducesPartialRoutes()
  {
    DesktopWorkspace model(pair());
    auto plan = *model.candidateForApply();
    plan.placements.last().position = plan.placements.first().position;
    QVERIFY(plan.available(model.topologies()).placements.isEmpty());
  }
  void serverSnapshotRoundTripNeverSerializesAuthentication()
  {
    DesktopWorkspace model(pair());
    WorkspaceSnapshot snapshot;
    snapshot.devices = {"Windows", "Mac", "Old-Mac"};
    snapshot.onlineDevices = {"Windows", "Mac"};
    snapshot.topologyCapable = {"Windows", "Mac"};
    snapshot.topologies = model.topologies();
    snapshot.applied = model.state();
    snapshot.perDisplayActive = true;
    const auto bytes = snapshot.encode();
    const auto decoded = WorkspaceSnapshot::decode(bytes);
    QVERIFY(decoded);
    QCOMPARE(decoded->encode(), bytes);
    QVERIFY(decoded->perDisplayActive);
    QVERIFY(!bytes.contains("authenticated"));
    QVERIFY(!bytes.contains("fingerprint"));
    QCOMPARE(decoded->devices.size(), 3);
    QCOMPARE(decoded->topologies.size(), 2);
  }
  void invalidServerSnapshotRejected_data()
  {
    QTest::addColumn<QString>("change");
    for (const auto &change :
         {"duplicate", "terminal-newline", "unknown-online", "unknown-capability", "bad-topology", "foreign-placement",
          "foreign-number", "oversize", "wrong-type"})
      QTest::newRow(change) << QString(change);
  }
  void invalidServerSnapshotRejected()
  {
    QFETCH(QString, change);
    DesktopWorkspace model(pair());
    WorkspaceSnapshot snapshot;
    snapshot.devices = {"Windows", "Mac"};
    snapshot.onlineDevices = snapshot.devices;
    snapshot.topologyCapable = snapshot.devices;
    snapshot.topologies = model.topologies();
    snapshot.applied = model.state();
    auto object = QJsonDocument::fromJson(snapshot.encode()).object();
    if (change == "duplicate")
      object["devices"] = QJsonArray{"Windows", "Windows"};
    if (change == "terminal-newline")
      object["devices"] = QJsonArray{"Windows", "Mac\n"};
    if (change == "unknown-online")
      object["online"] = QJsonArray{"Stranger"};
    if (change == "unknown-capability")
      object["topologyCapable"] = QJsonArray{"Stranger"};
    if (change == "bad-topology") {
      auto t = object["topologies"].toObject();
      t["Mac"] = QJsonObject{{"schema", 999}};
      object["topologies"] = t;
    }
    if (change == "foreign-placement") {
      snapshot.applied.layout.placements[0].device = "Stranger";
      object["applied"] = QJsonDocument::fromJson(snapshot.applied.encode()).object();
    }
    if (change == "foreign-number") {
      snapshot.applied.displayNumbers["Stranger/screen"] = 1;
      object["applied"] = QJsonDocument::fromJson(snapshot.applied.encode()).object();
    }
    if (change == "oversize")
      object["padding"] = QString(12001, 'x');
    if (change == "wrong-type")
      object["perDisplayActive"] = "true";
    QVERIFY(!WorkspaceSnapshot::decode(QJsonDocument(object).toJson(QJsonDocument::Compact)));
  }
  void mixedDpiProjectionPreservesOsContactsAndOffsets()
  {
    const auto t = windowsDual();
    const auto r = DesktopWorkspace::projectOsLayout(t);
    QVERIFY(t.monitors[1].logical.right() < t.monitors[0].logical.left()); // Qt island
    QCOMPARE(r["w-b"].right(), r["w-a"].left());
    QVERIFY(qAbs(r["w-b"].y() - r["w-a"].y() - 320.0 / 1.5) < 1e-9);
    QVERIFY(qAbs(r["w-b"].width() - 2560.0 / 1.5) < 1e-9);
    DesktopWorkspace model(pair());
    QCOMPARE(model.displays().size(), 3);
    QCOMPARE(screen(model, "w-b").pixelResolution, QSize(2560, 1600));
    QCOMPARE(screen(model, "w-b").logicalRect.size(), QSizeF(1463, 914));
    QVERIFY(!screen(model, "w-b").physicalMm);
  }
  void recordedWindowsGeometryMatchesSharedContract()
  {
    const Topology recorded{
        {{"a", {0, 0, 2560, 1440}, {0, 0, 3840, 2160}, {3840, 2160}, {}},
         {"b", {3840, 132, 1463, 914}, {3840, 132, 2560, 1600}, {2560, 1600}, {}}}
    };
    const auto projected = DesktopWorkspace::projectOsLayout(recorded);
    QCOMPARE(projected["a"], QRectF(0, 0, 2560, 1440));
    QCOMPARE(projected["b"].topLeft(), QPointF(2560, 88));
    QVERIFY(qAbs(projected["b"].width() - 1706.6666666667) < 1e-7);
    QVERIFY(qAbs(projected["b"].height() - 1066.6666666667) < 1e-7);
    QCOMPARE(projected["a"].right(), projected["b"].left());
    DesktopWorkspace model({ready("Windows", recorded, true)});
    QCOMPARE(screen(model, "b").logicalRect.size(), QSizeF(1463, 914));
    QCOMPARE(screen(model, "b").inputRect, QRect(3840, 132, 2560, 1600));
  }
  void missingOldPeerDoesNotEraseLocalScreensOrInventDimensions()
  {
    auto devices = pair();
    devices[1].topology.reset();
    devices[1].observation = Observation::Unsupported;
    devices[1].topologyCapability = Capability::Unsupported;
    devices[1].routingCapability = Capability::Unsupported;
    DesktopWorkspace model(devices);
    QCOMPARE(model.displays().size(), 2);
    QCOMPARE(model.unknownDevices(), QStringList{"Mac"});
    QCOMPARE(model.state().layout.placements.size(), 2);
    for (const auto &s : model.displays())
      QCOMPARE(s.deviceId, QString("Windows"));
  }
  void unrelatedUnknownDeviceDoesNotEraseKnownPairPreview()
  {
    auto devices = pair();
    DeviceSnapshot old;
    old.id = "Old-Mac";
    old.observation = Observation::Unsupported;
    devices.append(old);
    DesktopWorkspace model(devices);
    QCOMPARE(model.displays().size(), 3);
    QVERIFY(!model.previewRoutes().isEmpty());
    QVERIFY(model.candidateForApply());
    QCOMPARE(model.unknownDevices(), QStringList{"Old-Mac"});
  }
  void cachedSnapshotVisibleButCannotActivateRouting()
  {
    auto devices = pair();
    devices[1].connection = Connection::Offline;
    devices[1].observation = Observation::Cached;
    DesktopWorkspace model(devices);
    QCOMPARE(model.displays().size(), 3);
    QVERIFY(screen(model, "m-a").cached);
    QVERIFY(!model.candidateForApply());
    QVERIFY(!model.previewRoutes().isEmpty());
  }
  void observedUnverifiedPeerNeverBecomesRunnable()
  {
    auto devices = pair();
    devices[1].authenticated = false;
    DesktopWorkspace model(devices);
    QCOMPARE(model.displays().size(), 3);
    QVERIFY(!model.candidateForApply());
    auto saved = WorkspaceState::decode(model.state().encode());
    QVERIFY(saved);
    DesktopWorkspace restored(devices, saved->encode());
    QVERIFY(!restored.candidateForApply());
  }
  void topologyRoutingAndIdentifyCapabilitiesRemainIndependent()
  {
    auto devices = pair();
    devices[1].routingCapability = Capability::Unknown;
    devices[1].identifyCapability = Capability::Unsupported;
    DesktopWorkspace model(devices);
    QCOMPARE(model.displays().size(), 3);
    QVERIFY(!model.candidateForApply());
    QCOMPARE(model.devices()[1].identifyCapability, Capability::Unsupported);
  }
  void groupDragPreservesOsRelationshipAndUndoIsOneOperation()
  {
    DesktopWorkspace model(pair());
    const auto before = model.state().encode();
    const auto a = screen(model, "w-a"), b = screen(model, "w-b");
    QVERIFY(model.moveDisplay("Windows", "w-a", a.canvasRect.topLeft() + QPointF(-300, 120)));
    QCOMPARE(
        screen(model, "w-a").canvasRect.topLeft() - screen(model, "w-b").canvasRect.topLeft(),
        a.canvasRect.topLeft() - b.canvasRect.topLeft()
    );
    QCOMPARE(screen(model, "w-b").inputRect, b.inputRect);
    QVERIFY(model.undo());
    QCOMPARE(model.state().encode(), before);
    QVERIFY(!model.canUndo());
  }
  void explicitCustomModeDoesNotClaimToChangeOsLayout()
  {
    DesktopWorkspace model(pair());
    const auto a = screen(model, "w-a"), b = screen(model, "w-b");
    QVERIFY(model.setMode(ArrangementMode::CustomScreenRoutes));
    QVERIFY(model.moveDisplay("Windows", "w-a", a.canvasRect.topLeft() + QPointF(0, -100)));
    QCOMPARE(screen(model, "w-b").canvasRect, b.canvasRect);
    QCOMPARE(screen(model, "w-a").inputRect, a.inputRect);
    QVERIFY(!model.setMode(ArrangementMode::PreserveOsGroups));
    model.cancel();
    QCOMPARE(model.state().mode, ArrangementMode::PreserveOsGroups);
  }
  void groupDragSnapsToReachableEdgeWithoutChangingOsRelationship()
  {
    DesktopWorkspace model(pair());
    const auto a = screen(model, "w-a"), b = screen(model, "w-b");
    const auto saved = model.state().encode();
    QVERIFY(model.moveDisplay("Windows", "w-a", a.canvasRect.topLeft() + QPointF(-6, 40), 16));
    QVERIFY(qAbs(screen(model, "w-a").canvasRect.x() - a.canvasRect.x()) < 1e-8);
    QCOMPARE(screen(model, "w-a").canvasRect.y(), a.canvasRect.y() + 40);
    QCOMPARE(screen(model, "w-b").canvasRect.topLeft(), b.canvasRect.topLeft() + QPointF(0, 40));
    QVERIFY(model.candidateForApply());
    QVERIFY(!model.previewRoutes().isEmpty());
    QVERIFY(model.undo());
    QCOMPARE(model.state().encode(), saved);
    QVERIFY(model.moveDisplay("Windows", "w-a", a.canvasRect.topLeft() + QPointF(-6, 40), 0));
    QCOMPARE(screen(model, "w-a").canvasRect.x(), a.canvasRect.x() - 6);
    QVERIFY(!model.candidateForApply());
  }
  void physicalCalibrationNeverSilentlyChangesRoutes()
  {
    DesktopWorkspace model(pair());
    const auto before = model.state().layout.encode();
    QVERIFY(model.calibratePhysicalSize("Windows", "w-a", QSizeF(700, 390)));
    QCOMPARE(model.state().layout.encode(), before);
    QCOMPARE(screen(model, "w-a").physicalMm, std::optional<QSizeF>(QSizeF(700, 390)));
    QVERIFY(screen(model, "w-a").calibrated);
    QVERIFY(!model.calibratePhysicalSize("Windows", "w-a", QSizeF(-1, 390)));
    DesktopWorkspace reopened(pair(), model.state().encode());
    QCOMPARE(reopened.state().layout.encode(), before);
    QVERIFY(reopened.calibratePhysicalSize("Windows", "w-a", {}));
    QVERIFY(!screen(reopened, "w-a").physicalMm);
  }
  void stableNumbersSurviveEnumerationOrderAndNewScreen()
  {
    DesktopWorkspace first(pair());
    const int a = screen(first, "w-a").number, b = screen(first, "w-b").number;
    auto devices = pair();
    std::reverse(devices[0].topology->monitors.begin(), devices[0].topology->monitors.end());
    DesktopWorkspace reordered(devices, first.state().encode());
    QCOMPARE(screen(reordered, "w-a").number, a);
    QCOMPARE(screen(reordered, "w-b").number, b);
    devices[0].topology->monitors.append(
        {"w-new", {-3560, -1000, 1000, 1000}, {-3560, -1000, 1000, 1000}, {1000, 1000}, {}}
    );
    DesktopWorkspace changed(devices, first.state().encode());
    QVERIFY(changed.conflicts().contains("Windows"));
    QVERIFY(changed.reviewOsLayout("Windows"));
    QCOMPARE(screen(changed, "w-a").number, a);
    QCOMPARE(screen(changed, "w-b").number, b);
    QVERIFY(screen(changed, "w-new").number > qMax(a, b));
    QVERIFY(changed.undo());
    QVERIFY(changed.conflicts().contains("Windows"));
  }
  void changedResolutionRequiresReviewAndCancelPreservesOriginal()
  {
    DesktopWorkspace original(pair());
    const auto saved = original.state().encode();
    auto devices = pair();
    devices[1].topology->monitors[0].pixels = {5120, 2880};
    DesktopWorkspace model(devices, saved);
    QCOMPARE(model.validationError(), QString("topology-review-required"));
    QVERIFY(!model.candidateForApply());
    QVERIFY(model.reviewOsLayout("Mac"));
    QVERIFY(model.conflicts().isEmpty());
    model.cancel();
    QCOMPARE(model.state().encode(), saved);
    QVERIFY(model.conflicts().contains("Mac"));
  }
  void layoutExtentRoundTripAndLegacySchemaStayDistinct()
  {
    DesktopWorkspace model(pair());
    const auto encoded = model.state().layout.encode();
    QCOMPARE(QJsonDocument::fromJson(encoded)["schema"].toInt(), 2);
    const auto layout = Layout::decode(encoded);
    QVERIFY(layout);
    QCOMPARE(layout->encode(), encoded);
    Layout old{{{"Mac", "m-a", {0, 0}}}};
    QCOMPARE(QJsonDocument::fromJson(old.encode())["schema"].toInt(), 1);
    QVERIFY(Layout::decode(old.encode()));
    QVERIFY(!Layout::decode(
        "{\"schema\":1,\"placements\":[{\"device\":\"Mac\",\"monitor\":\"m-a\",\"x\":0,\"y\":0,\"extent\":[200,100]}]}"
    ));
    QVERIFY(!Layout::decode(
        "{\"schema\":2,\"placements\":[{\"device\":\"Mac\",\"monitor\":\"m-a\",\"x\":0,\"y\":0,\"extent\":[0,100]}]}"
    ));
  }
  void routingUsesInputCoordinatesNotQtIslandOrPhysicalMillimetres_data()
  {
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("target");
    QTest::addColumn<bool>("sourceMac");
    QTest::addColumn<bool>("targetMac");
    QTest::newRow("Windows-dual-to-Mac") << QString("Windows") << QString("Mac") << false << true;
    QTest::newRow("Mac-dual-to-Windows") << QString("Mac") << QString("Windows") << true << false;
    QTest::newRow("Windows-to-Windows-protocol-simulation") << QString("WinA") << QString("WinB") << false << false;
    QTest::newRow("Mac-to-Mac-protocol-simulation") << QString("MacA") << QString("MacB") << true << true;
  }
  void routingUsesInputCoordinatesNotQtIslandOrPhysicalMillimetres()
  {
    QFETCH(QString, source);
    QFETCH(QString, target);
    QFETCH(bool, sourceMac);
    QFETCH(bool, targetMac);
    DesktopWorkspace model(
        {ready(source, sourceMac ? macDual() : windowsDual(), true),
         ready(target, targetMac ? macSingle() : windowsSingle())}
    );
    const auto layout = model.candidateForApply();
    QVERIFY2(layout, qPrintable(model.validationError()));
    const int sourceWidth = sourceMac ? 2560 : 3840, sourceY = sourceMac ? 720 : 1080, targetY = targetMac ? 720 : 1080;
    const auto t = model.topologies();
    const auto forward = layout->cross(t, source, "w-a", Side::Right, {double(sourceWidth - 1), double(sourceY)});
    QVERIFY(forward);
    QCOMPARE(forward->device, target);
    QCOMPARE(forward->monitor, QString("m-a"));
    QCOMPARE(forward->input, QPoint(1, targetY));
    const auto back = layout->cross(t, target, "m-a", Side::Left, {0, double(targetY)});
    QVERIFY(back);
    QCOMPARE(back->device, source);
    QCOMPARE(back->input, QPoint(sourceWidth - 2, sourceY));
    // An internal OS edge never becomes a cross-device route.
    QVERIFY(!layout->cross(t, source, "w-b", Side::Right, {-1, 1000}));
  }
  void malformedWorkspaceAndDuplicateIdsAreRejected()
  {
    QVERIFY(!WorkspaceState::decode(QByteArray(262145, 'x')));
    QVERIFY(!WorkspaceState::decode("{}"));
    auto devices = pair();
    devices.append(devices[0]);
    DesktopWorkspace duplicate(devices);
    QVERIFY(!duplicate.error().isEmpty());
    DesktopWorkspace broken(pair(), "not json");
    QCOMPARE(broken.error(), QString("unreadable-workspace-preserved"));
    QVERIFY(!broken.moveDevice("Mac", {1, 0}));
  }
  void freshProcessReadsSavedGeometryNumbersAndCalibration()
  {
    const auto root = qEnvironmentVariable("DESKFLOW_TEST_ROOT", QDir::tempPath());
    QVERIFY(QDir::isAbsolutePath(root));
    QVERIFY(QDir(root).exists());
    QTemporaryDir directory(root + "/workspace-XXXXXX");
    QVERIFY(directory.isValid());
    const auto path = directory.path() + "/workspace.json";
    QProcess writer;
    writer.start(QCoreApplication::applicationFilePath(), {"--write", path});
    QVERIFY(writer.waitForFinished(10000));
    QCOMPARE(writer.exitCode(), 0);
    QProcess reader;
    reader.start(QCoreApplication::applicationFilePath(), {"--read", path});
    QVERIFY(reader.waitForFinished(10000));
    QCOMPARE(reader.exitCode(), 0);
    const auto result = QJsonDocument::fromJson(reader.readAllStandardOutput()).object();
    QCOMPARE(result["count"].toInt(), 3);
    QCOMPARE(result["macY"].toDouble(), 120.0);
    QVERIFY(result["calibrated"].toBool());
    QCOMPARE(result["widthMm"].toInt(), 700);
    QVERIFY(result["valid"].toBool());
    QVERIFY(result["winNumber"].toInt() > 0);
  }
};
int main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);
  if (argc == 3 && (QByteArray(argv[1]) == "--write" || QByteArray(argv[1]) == "--read"))
    return child(QString::fromLocal8Bit(argv[2]), QByteArray(argv[1]) == "--write");
  DesktopWorkspaceTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "DesktopWorkspaceTests.moc"
