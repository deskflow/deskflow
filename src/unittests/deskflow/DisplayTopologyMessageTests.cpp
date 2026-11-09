// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "base/Log.h"
#include "common/DesktopWorkspace.h"
#include "deskflow/DeskflowException.h"
#include "deskflow/DisplayTopologyMessage.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/ProtocolUtil.h"
#include "io/IStream.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>
#include <algorithm>
#include <cstring>
using namespace deskflow::display;
class MemoryStream : public deskflow::IStream
{
public:
  QByteArray bytes;
  void write(const void *p, uint32_t n) override
  {
    bytes.append(static_cast<const char *>(p), n);
  }
  uint32_t read(void *p, uint32_t n) override
  {
    const auto size = std::min<qsizetype>(n, bytes.size());
    if (p)
      std::memcpy(p, bytes.data(), size);
    bytes.remove(0, size);
    return size;
  }
  void close() override
  {
    bytes.clear();
  }
  void flush() override
  {
  }
  void shutdownInput() override
  {
    close();
  }
  void shutdownOutput() override
  {
  }
  void *getEventTarget() const override
  {
    return const_cast<MemoryStream *>(this);
  }
  bool isReady() const override
  {
    return !bytes.isEmpty();
  }
  uint32_t getSize() const override
  {
    return bytes.size();
  }
};
class DisplayTopologyMessageTests : public QObject
{
  Q_OBJECT
  Log m_log;
  Topology fixture() const
  {
    return {
        {{"left", {-1600, 0, 1600, 900}, {-1600, 0, 1600, 900}, {3200, 1800}, {}},
         {"right", {0, 0, 1920, 1080}, {0, 0, 1920, 1080}, {3840, 2160}, {}}}
    };
  }
private Q_SLOTS:
  void negotiatedOnlyOnVerifiedTls()
  {
    QCOMPARE(offeredMinor(true, true), 9);
    QCOMPARE(offeredMinor(false, true), 8);
    QCOMPARE(offeredMinor(true, false), 8);
    QCOMPARE(offeredMinor(false, false), 8);
    QCOMPARE(std::min(offeredMinor(true, true), 8), 8);
  }
  void oldDinfBytesAndExtendedTopologyRemainSeparate()
  {
    MemoryStream s;
    ProtocolUtil::writef(&s, kMsgDInfo, -1600, 0, 3520, 1080, 0, 100, 200);
    QCOMPARE(s.bytes, QByteArray("DINF") + QByteArray::fromHex("f9c0 0000 0dc0 0438 0000 0064 00c8"));
    const auto dinf = s.bytes;
    writeTopologyMessage(&s, "Mac", fixture());
    QVERIFY(s.bytes.startsWith(dinf + "DTOP"));
    s.bytes.remove(0, dinf.size() + 4);
    const auto message = readTopologyMessage(&s, "Mac");
    QCOMPARE(message.device, QString("Mac"));
    QVERIFY(message.topology);
    QCOMPARE(message.topology->encode(), fixture().encode());
    QVERIFY(s.bytes.isEmpty());
  }
  void unavailableIsExplicit()
  {
    MemoryStream s;
    writeTopologyMessage(&s, "Mac", {});
    QVERIFY(s.bytes.startsWith("DTOP"));
    s.bytes.remove(0, 4);
    QVERIFY(!readTopologyMessage(&s, "Mac").topology);
  }
  void verifiedStreamCanCarryReadOnlyWorkspaceWithoutChangingDinf()
  {
    WorkspaceSnapshot snapshot;
    snapshot.devices = {"Mac"};
    snapshot.onlineDevices = {"Mac"};
    snapshot.topologyCapable = {"Mac"};
    snapshot.topologies["Mac"] = fixture();
    MemoryStream s;
    writeTopologyMessage(&s, "Mac", fixture(), snapshot.encode());
    s.bytes.remove(0, 4);
    const auto message = readTopologyMessage(&s, "Mac");
    QCOMPARE(message.workspace, snapshot.encode());
    QCOMPARE(message.topology->encode(), fixture().encode());
  }
  void olderTopologyDecoderIgnoresOptionalWorkspace()
  {
    WorkspaceSnapshot snapshot;
    snapshot.devices = {"Mac"};
    snapshot.topologies["Mac"] = fixture();
    MemoryStream s;
    writeTopologyMessage(&s, "Mac", fixture(), snapshot.encode());
    s.bytes.remove(0, 4);
    std::string name, payload;
    QVERIFY(ProtocolUtil::readf(&s, "%s%s", &name, &payload));
    const auto topology = Topology::decode(QByteArray::fromStdString(payload));
    QVERIFY(topology);
    QCOMPARE(topology->encode(), fixture().encode());
  }
  void unavailableTopologyDoesNotSendUnusableExtension()
  {
    WorkspaceSnapshot snapshot;
    snapshot.devices = {"Mac"};
    MemoryStream s;
    writeTopologyMessage(&s, "Mac", {}, snapshot.encode());
    s.bytes.remove(0, 4);
    const auto message = readTopologyMessage(&s, "Mac");
    QVERIFY(!message.topology);
    QVERIFY(message.workspace.isEmpty());
  }
  void mismatchedWorkspaceSenderRejected()
  {
    WorkspaceSnapshot snapshot;
    snapshot.devices = {"Other"};
    snapshot.topologies["Other"] = fixture();
    auto object = QJsonDocument::fromJson(fixture().encode()).object();
    object["workspace"] = QJsonDocument::fromJson(snapshot.encode()).object();
    MemoryStream s;
    const std::string name = "Mac", payload = QJsonDocument(object).toJson(QJsonDocument::Compact).toStdString();
    ProtocolUtil::writef(&s, "%s%s", &name, &payload);
    QVERIFY_EXCEPTION_THROWN(readTopologyMessage(&s, "Mac"), BadClientException);
  }
  void mismatchedWorkspaceTopologyRejected()
  {
    WorkspaceSnapshot snapshot;
    snapshot.devices = {"Mac"};
    snapshot.topologies["Mac"] = fixture();
    snapshot.topologies["Mac"].monitors[0].pixels = {6400, 3600};
    auto object = QJsonDocument::fromJson(fixture().encode()).object();
    object["workspace"] = QJsonDocument::fromJson(snapshot.encode()).object();
    MemoryStream s;
    const std::string name = "Mac", payload = QJsonDocument(object).toJson(QJsonDocument::Compact).toStdString();
    ProtocolUtil::writef(&s, "%s%s", &name, &payload);
    QVERIFY_EXCEPTION_THROWN(readTopologyMessage(&s, "Mac"), BadClientException);
  }
  void invalidPeerDataIsRejected_data()
  {
    QTest::addColumn<QString>("name");
    QTest::addColumn<QByteArray>("payload");
    QTest::addColumn<QString>("expected");
    QTest::newRow("wrong-peer") << QString("Other") << fixture().encode() << QString("Mac");
    QTest::newRow("invalid-name") << QString("bad/name") << fixture().encode() << QString();
    QTest::newRow("unknown-schema") << QString("Mac") << QByteArray("{\"schema\":9,\"monitors\":[]}") << QString("Mac");
    QTest::newRow("oversize") << QString("Mac") << QByteArray(16385, 'x') << QString("Mac");
  }
  void invalidPeerDataIsRejected()
  {
    QFETCH(QString, name);
    QFETCH(QByteArray, payload);
    QFETCH(QString, expected);
    MemoryStream s;
    const auto n = name.toStdString(), p = payload.toStdString();
    ProtocolUtil::writef(&s, "%s%s", &n, &p);
    QVERIFY_EXCEPTION_THROWN(readTopologyMessage(&s, expected), BadClientException);
  }
  void truncatedPayloadRejected()
  {
    MemoryStream s;
    writeTopologyMessage(&s, "Mac", fixture());
    s.bytes.remove(0, 4);
    s.bytes.chop(1);
    QVERIFY_EXCEPTION_THROWN(readTopologyMessage(&s, "Mac"), BadClientException);
  }
};
QTEST_GUILESS_MAIN(DisplayTopologyMessageTests)
#include "DisplayTopologyMessageTests.moc"
