/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoordinationProtocolTests.h"

#include "coordination/CoordinationProtocol.h"
#include "coordination/Peer.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

using deskflow::coordination::Message;
using deskflow::coordination::parsePeerList;
using deskflow::coordination::Role;
namespace protocol = deskflow::coordination::protocol;

void CoordinationProtocolTests::claimRoundTrip()
{
  const auto line = protocol::encodeClaim("alpha", "10.0.0.1", "alpha.local", 42, "secret");
  const auto message = protocol::decode(line);

  QCOMPARE(message.type, Message::Type::Claim);
  QCOMPARE(message.name, std::string("alpha"));
  QCOMPARE(message.ip, std::string("10.0.0.1"));
  QCOMPARE(message.lan, std::string("alpha.local"));
  QCOMPARE(message.seq, 42);
  QCOMPARE(message.token, std::string("secret"));
}

void CoordinationProtocolTests::promoteRoundTrip()
{
  const auto message = protocol::decode(protocol::encodePromote(""));
  QCOMPARE(message.type, Message::Type::Promote);
  QVERIFY(message.token.empty());
}

void CoordinationProtocolTests::statusRoundTrip()
{
  const auto message = protocol::decode(protocol::encodeStatus("tok"));
  QCOMPARE(message.type, Message::Type::Status);
  QCOMPARE(message.token, std::string("tok"));
}

void CoordinationProtocolTests::decodesLegacyCoordinatorClaim()
{
  // Exact shape the legacy Python coordinator emits (no token field).
  const std::string legacy = R"({"t":"claim","name":"bravo","ip":"100.64.0.20","lan":"bravo.local","seq":183464})";
  const auto message = protocol::decode(legacy);

  QCOMPARE(message.type, Message::Type::Claim);
  QCOMPARE(message.name, std::string("bravo"));
  QCOMPARE(message.lan, std::string("bravo.local"));
  QCOMPARE(message.seq, 183464);
  QVERIFY(message.token.empty());
}

void CoordinationProtocolTests::toleratesStringSequenceNumbers()
{
  const auto message = protocol::decode(R"({"t":"claim","name":"x","ip":"1.1.1.1","seq":"17"})");
  QCOMPARE(message.seq, 17);
}

void CoordinationProtocolTests::malformedInputIsInvalid()
{
  QCOMPARE(protocol::decode("not json").type, Message::Type::Invalid);
  QCOMPARE(protocol::decode("{}").type, Message::Type::Invalid);
  QCOMPARE(protocol::decode(R"({"t":"teleport"})").type, Message::Type::Invalid);
  QCOMPARE(protocol::decode("[1,2,3]").type, Message::Type::Invalid);
}

void CoordinationProtocolTests::statusReplyMatchesLegacyShape()
{
  const auto reply = protocol::encodeStatusReply(Role::Server, "", 7, 1781148503.5, "alpha");
  const auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(reply));

  QVERIFY(doc.isObject());
  const auto object = doc.object();
  QCOMPARE(object["role"].toString(), QStringLiteral("server"));
  QVERIFY(object["server_ip"].isNull()); // server reports no upstream
  QCOMPARE(object["seq"].toInt(), 7);
  QCOMPARE(object["name"].toString(), QStringLiteral("alpha"));
  QVERIFY(object.contains("last_switch"));

  const auto clientReply = protocol::encodeStatusReply(Role::Client, "10.0.0.9", 8, 0.0, "beta");
  const auto clientObject = QJsonDocument::fromJson(QByteArray::fromStdString(clientReply)).object();
  QCOMPARE(clientObject["role"].toString(), QStringLiteral("client"));
  QCOMPARE(clientObject["server_ip"].toString(), QStringLiteral("10.0.0.9"));
}

void CoordinationProtocolTests::peerListParsing()
{
  const auto peers = parsePeerList(" desktop=192.0.2.10|desktop.local, laptop=192.0.2.11 ,=x,name= ");

  QCOMPARE(peers.size(), static_cast<size_t>(2));
  QCOMPARE(peers[0].name, std::string("desktop"));
  QCOMPARE(peers[0].ip, std::string("192.0.2.10"));
  QCOMPARE(peers[0].lan, std::string("desktop.local"));
  QCOMPARE(peers[1].name, std::string("laptop"));
  QCOMPARE(peers[1].lan, std::string("192.0.2.11")); // lan defaults to ip
  QVERIFY(peers[1].hasAddress("192.0.2.11"));
  QVERIFY(!peers[1].hasAddress(""));

  // Bare entries: machine names or addresses, no `=` required.
  const auto bare = parsePeerList("gamepc, laptop.local, studio.example.net");
  QCOMPARE(bare.size(), static_cast<size_t>(3));
  QCOMPARE(bare[0].name, std::string("gamepc"));
  QCOMPARE(bare[0].ip, std::string("gamepc"));
  QCOMPARE(bare[0].lan, std::string("gamepc.local")); // mDNS candidate for plain names
  QCOMPARE(bare[1].name, std::string("laptop"));      // name is the first dot-label
  QCOMPARE(bare[1].ip, std::string("laptop.local"));
  QCOMPARE(bare[1].lan, std::string("laptop.local"));
  QCOMPARE(bare[2].name, std::string("studio"));
  QCOMPARE(bare[2].ip, std::string("studio.example.net"));
}

QTEST_MAIN(CoordinationProtocolTests)
