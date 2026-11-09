// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/DisplayTopology.h"
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>
#ifdef Q_OS_MACOS
#include <CoreGraphics/CoreGraphics.h>
#endif
using namespace deskflow::display;
class DisplayRoutingTests : public QObject
{
  Q_OBJECT
  Topologies topologies() const
  {
    return {
        {"Windows", Topology{{{"w", {0, 0, 1920, 1080}, {0, 0, 3840, 2160}, {3840, 2160}, {}}}}},
        {"Mac", Topology{
                    {{"m-left", {-1600, 0, 1600, 900}, {-1600, 0, 1600, 900}, {3200, 1800}, {}},
                     {"m-right", {0, 0, 1920, 1080}, {0, 0, 1920, 1080}, {3840, 2160}, {}}}
                }}
    };
  }
  Layout layout() const
  {
    return {{{"Mac", "m-left", {-3520, 0}}, {"Mac", "m-right", {-1920, 0}}, {"Windows", "w", {0, 0}}}};
  }
private Q_SLOTS:
  void topologyRoundTripPreservesStableIdsAndCoordinateSpaces()
  {
    auto t = topologies();
    for (const auto &v : t) {
      QVERIFY(v.error().isEmpty());
      const auto decoded = Topology::decode(v.encode());
      QVERIFY(decoded);
      QVERIFY(*decoded == v || decoded->encode() == v.encode());
      QCOMPARE(decoded->revision(), v.revision());
    }
    auto moved = t["Mac"];
    moved.monitors[0].input.translate(-100, 0);
    QCOMPARE(moved.monitors[0].id, QString("m-left"));
    QVERIFY(moved.revision() != t["Mac"].revision());
    QVERIFY(!t["Mac"].monitors[0].physicalMm);
    QCOMPARE(t["Mac"].monitors[0].pixels, QSize(3200, 1800));
  }
  void twoDevicesOneDualScreenMixedDpiRoundTrip()
  {
    const auto t = topologies();
    const auto l = layout();
    QVERIFY(l.error(t).isEmpty());
    const auto saved = Layout::decode(l.encode());
    QVERIFY(saved);
    QVERIFY(saved->placements == l.placements);
    const auto crossing = saved->cross(t, "Windows", "w", Side::Left, {0, 900});
    QVERIFY(crossing);
    QCOMPARE(crossing->device, QString("Mac"));
    QCOMPARE(crossing->monitor, QString("m-right"));
    QCOMPARE(crossing->input, QPoint(1918, 450));
    const auto back = l.cross(t, "Mac", "m-right", Side::Right, {1919, 450});
    QVERIFY(back);
    QCOMPARE(back->input.x(), 1);
    QVERIFY(qAbs(back->input.y() - 900) <= 1);
  }
  void interiorNativeEdgesNeverBecomeRemoteEdges()
  {
    const auto t = topologies();
    Layout misleading{{{"Mac", "m-left", {-1600, 0}}, {"Windows", "w", {0, 0}}, {"Mac", "m-right", {1920, 0}}}};
    QVERIFY(!misleading.cross(t, "Mac", "m-left", Side::Right, {-1, 300}));
    QVERIFY(!misleading.cross(t, "Windows", "w", Side::Left, {0, 600}));
    QVERIFY(!misleading.leaving(t, "Mac", "m-left", {-2, 300}, {1, 300}));
  }
  void negativeCoordinatesGapsAndCorners()
  {
    auto t = topologies();
    t["Mac"].monitors[0].input = QRect(-2240, 0, 1600, 900);
    t["Mac"].monitors[0].logical = QRectF(-2240, 0, 1600, 900);
    Layout l{{{"Mac", "m-left", {-1600, 0}}, {"Windows", "w", {0, 0}}, {"Mac", "m-right", {1920, 0}}}};
    auto crossing = l.cross(t, "Mac", "m-left", Side::Right, {-641, 450});
    QVERIFY(crossing);
    QCOMPARE(crossing->device, QString("Windows"));
    QCOMPARE(l.clamp(t, "Mac", "m-left", {-500, 450}), QPoint(-641, 450));
    QVERIFY(!l.monitorAt(t, "Mac", {-500, 450}));
    l.placements[1].position.setY(900);
    QVERIFY(!l.cross(t, "Mac", "m-left", Side::Right, {-641, 899}));
  }
  void partialNativeEdgeOcclusionMatchesPreviewAndRoute()
  {
    Topologies t{
        {"Mac",
         Topology{
             {{"a", {0, 0, 1000, 1000}, {0, 0, 1000, 1000}, {2000, 2000}, {}},
              {"b", {1000, 0, 1000, 400}, {1000, 0, 1000, 400}, {2000, 800}, {}}}
         }},
        {"Windows", Topology{{{"w", {0, 0, 1000, 1000}, {0, 0, 1000, 1000}, {1000, 1000}, {}}}}}
    };
    Layout l{{{"Mac", "a", {0, 0}}, {"Mac", "b", {1000, -400}}, {"Windows", "w", {1000, 0}}}};
    QVERIFY2(l.error(t).isEmpty(), qPrintable(l.error(t)));
    QVERIFY(!l.cross(t, "Mac", "a", Side::Right, {999, 200}));
    const auto crossing = l.cross(t, "Mac", "a", Side::Right, {999, 600});
    QVERIFY(crossing);
    QCOMPARE(crossing->input, QPoint(1, 600));
    QVERIFY(!l.cross(t, "Windows", "w", Side::Left, {0, 200}));
    bool preview = false;
    for (const auto &r : l.routes(t))
      if (r.device == "Mac" && r.monitor == "a" && r.targetDevice == "Windows") {
        QCOMPARE(r.sourceStart, 0.4);
        QCOMPARE(r.sourceEnd, 1.0);
        QCOMPARE(r.segment, QLineF(1000, 400, 1000, 1000));
        preview = true;
      }
    QVERIFY(preview);
    QCOMPARE(l.crossValidated(t, l.routes(t), "Mac", "a", Side::Right, {999, 600})->input, crossing->input);
  }
  void unreachableCanvasContactIsRejected()
  {
    auto t = topologies();
    t["Mac"].monitors[0].input.setHeight(1080);
    t["Mac"].monitors[0].logical.setHeight(1080);
    t["Mac"].monitors[0].pixels.setHeight(2160);
    Layout l{{{"Mac", "m-left", {-1600, 0}}, {"Windows", "w", {0, 0}}, {"Mac", "m-right", {1920, 0}}}};
    QCOMPARE(l.error(t), QString("no-reachable-display-edge"));
  }
  void invalidAndUnknownSchemasDoNotRoute()
  {
    QVERIFY(!Topology::decode(QByteArray(16385, 'x')));
    QVERIFY(!Topology::decode("{\"schema\":2,\"monitors\":[]}"));
    QVERIFY(!Layout::decode("{\"schema\":2,\"placements\":[]}"));
    auto t = topologies();
    t["Mac"].monitors[1].id = "m-left";
    QVERIFY(!t["Mac"].error().isEmpty());
    QVERIFY(!layout().cross(t, "Windows", "w", Side::Left, {0, 200}));
    t = topologies();
    t["Windows"].monitors[0].input.moveLeft(32000);
    QVERIFY(!t["Windows"].error().isEmpty());
  }
  void disconnectAndMissingMonitorDoNotUseStaleGeometry()
  {
    auto t = topologies();
    t.remove("Mac");
    QVERIFY(!layout().cross(t, "Windows", "w", Side::Left, {0, 300}));
  }
  void readOnlyNativeSnapshotHasStableIdentity()
  {
#ifdef Q_OS_MACOS
    uint32_t count = 0;
    const auto status = CGGetActiveDisplayList(0, nullptr, &count);
    QCOMPARE(status, kCGErrorSuccess);
    if (count == 0)
      QSKIP(
          "The executor has no active native displays (CoreGraphics reports success and count 0); native capture needs "
          "a connected desktop session."
      );
    const auto a = localTopology(), b = localTopology();
    QVERIFY(a.error().isEmpty());
    QCOMPARE(a.encode(), b.encode());
    for (const auto &m : a.monitors) {
      QCOMPARE(m.id.size(), 64);
      QVERIFY(!m.physicalMm);
    }
#else
    QSKIP(
        "Native Windows capture requires the normal Windows Qt platform; this offscreen suite tests injected "
        "topologies."
    );
#endif
  }
};
int main(int argc, char **argv)
{
  QGuiApplication app(argc, argv);
  DisplayRoutingTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "DisplayRoutingTests.moc"
