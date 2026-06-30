/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "HidPassthroughTests.h"

#include "server/HidPassthrough.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

using deskflow::server::hidBytesToHex;
using deskflow::server::makeUpdateDecodeLine;
using deskflow::server::mergeDecodeIntoConnectLine;
using deskflow::server::parseHidDeviceSelectors;

void HidPassthroughTests::parsesDeviceSelectors()
{
  const auto selectors = parseHidDeviceSelectors(" 046D:B042, 046d:*, 1234:0001 ");

  QCOMPARE(selectors.size(), static_cast<size_t>(3));
  QCOMPARE(selectors[0].vid, 0x046D);
  QCOMPARE(selectors[0].pid, 0xB042);
  QCOMPARE(selectors[1].vid, 0x046D);
  QCOMPARE(selectors[1].pid, 0); // wildcard product
  QCOMPARE(selectors[2].vid, 0x1234);
  QCOMPARE(selectors[2].pid, 0x0001);
}

void HidPassthroughTests::rejectsMalformedSelectors()
{
  QVERIFY(parseHidDeviceSelectors("").empty());
  QVERIFY(parseHidDeviceSelectors("garbage").empty());
  QVERIFY(parseHidDeviceSelectors(":B042").empty());
  QVERIFY(parseHidDeviceSelectors("046D:").empty());
  QVERIFY(parseHidDeviceSelectors("ZZZZ:0001").empty());
  QVERIFY(parseHidDeviceSelectors("046D:WXYZ").empty());

  // Valid entries survive next to malformed ones.
  const auto mixed = parseHidDeviceSelectors("bad, 046D:B042, also:bad:bad");
  QCOMPARE(mixed.size(), static_cast<size_t>(1));
  QCOMPARE(mixed[0].pid, 0xB042);
}

void HidPassthroughTests::encodesFrameHex()
{
  const uint8_t frame[] = {0x11, 0xFF, 0x0B, 0x00, 0xC3};
  QCOMPARE(hidBytesToHex(frame, sizeof(frame)), std::string("11ff0b00c3"));
  QCOMPARE(hidBytesToHex(frame, 0), std::string());
}

void HidPassthroughTests::mergesDecodeIntoConnectLine()
{
  const std::string connect = R"({"type":"connect","device":{"product_id":"0xB042"}})";
  QJsonObject decode;
  decode[QStringLiteral("feat_idx")] = 11;
  decode[QStringLiteral("gesture_cid")] = QStringLiteral("0x01A0");

  const auto merged = mergeDecodeIntoConnectLine(connect, decode);
  const auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(merged));
  QVERIFY(doc.isObject());
  const auto device = doc.object()[QStringLiteral("device")].toObject();
  QCOMPARE(device[QStringLiteral("decode")].toObject()[QStringLiteral("feat_idx")].toInt(), 11);
}

void HidPassthroughTests::mergeLeavesMalformedConnectUntouched()
{
  const std::string garbage = "not-json";
  QJsonObject decode;
  decode[QStringLiteral("feat_idx")] = 11;
  QCOMPARE(mergeDecodeIntoConnectLine(garbage, decode), garbage);
}

void HidPassthroughTests::makesUpdateDecodeLine()
{
  QJsonObject decode;
  decode[QStringLiteral("feat_idx")] = 11;
  const auto line = makeUpdateDecodeLine(decode);
  const auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(line));
  QVERIFY(doc.isObject());
  QCOMPARE(doc.object()[QStringLiteral("type")].toString(), QStringLiteral("update_decode"));
  QCOMPARE(doc.object()[QStringLiteral("decode")].toObject()[QStringLiteral("feat_idx")].toInt(), 11);
}

QTEST_MAIN(HidPassthroughTests)
