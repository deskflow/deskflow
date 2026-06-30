/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "HidConsumerTests.h"

#include "client/HidConsumer.h"
#include "client/HidSink.h"
#include "common/Settings.h"

#include <QTest>

#include <string>
#include <vector>

void HidConsumerTests::encodesRawReportAsMouserLine()
{
  const std::string bytes = {'\x11', '\xFF', '\x0B'};
  const auto line = deskflow::client::encodeHidReportAsMouserLine(7, bytes);
  QCOMPARE(
      line,
      R"({"type": "report", "device_id": 7, "data": "11ff0b"})"
  );
}

void HidConsumerTests::shouldDeliverRejectsEmptyAndOversizedPayloads()
{
  QVERIFY(!deskflow::client::shouldDeliverRawHidReport(0));
  QVERIFY(deskflow::client::shouldDeliverRawHidReport(1));
  QVERIFY(deskflow::client::shouldDeliverRawHidReport(deskflow::client::kMaxHidReportPayloadBytes));
  QVERIFY(!deskflow::client::shouldDeliverRawHidReport(deskflow::client::kMaxHidReportPayloadBytes + 1));
}

void HidConsumerTests::deliverRawHidReportSkipsEmptyPayload()
{
  std::vector<std::string> lines;
  deskflow::client::deliverRawHidReport([&lines](const std::string &line) { lines.push_back(line); }, 1, std::string{});
  QVERIFY(lines.empty());
}

void HidConsumerTests::deliverRawHidReportForwardsNonEmptyPayload()
{
  std::vector<std::string> frames;
  const std::string bytes = {'\x01', '\x02'};
  deskflow::client::deliverRawHidReport([&frames](const std::string &frame) { frames.push_back(frame); }, 3, bytes);
  QCOMPARE(frames.size(), static_cast<size_t>(1));
  QVERIFY(deskflow::client::isHidReportFrame(frames[0]));
  QCOMPARE(frames[0], deskflow::client::encodeHidReportFrame(3, bytes));
}

void HidConsumerTests::encodesSinkFrame()
{
  const std::string bytes = {'\x11', '\xFF', '\x0B'};
  const auto frame = deskflow::client::encodeHidReportFrame(9, bytes);
  QVERIFY(deskflow::client::isHidReportFrame(frame));
  QCOMPARE(frame.size(), deskflow::client::kHidSinkHeaderBytes + bytes.size());
}

void HidConsumerTests::mouserHidDeliveryEnabledFromSettings()
{
  const QString settingsFile = QStringLiteral("tmp/hid-consumer-settings.conf");
  Settings::setSettingsFile(settingsFile);

  Settings::setValue(Settings::Client::MouserEnabled, false);
  QVERIFY(!deskflow::client::mouserHidDeliveryEnabled());

  Settings::setValue(Settings::Client::MouserEnabled, true);
  QVERIFY(deskflow::client::mouserHidDeliveryEnabled());
}

void HidConsumerTests::deliverRawHidReportToMouserSkipsNullClient()
{
  const std::string bytes = {'\x01'};
  deskflow::client::deliverRawHidReportToMouser(nullptr, 1, bytes);
  // no crash; null client is a no-op (no frame to verify)
}

QTEST_MAIN(HidConsumerTests)
