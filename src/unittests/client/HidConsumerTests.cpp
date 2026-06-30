/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "HidConsumerTests.h"

#include "client/HidConsumer.h"
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
  std::vector<std::string> lines;
  const std::string bytes = {'\x01', '\x02'};
  deskflow::client::deliverRawHidReport([&lines](const std::string &line) { lines.push_back(line); }, 3, bytes);
  QCOMPARE(lines.size(), static_cast<size_t>(1));
  QCOMPARE(lines[0], deskflow::client::encodeHidReportAsMouserLine(3, bytes));
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
  QVERIFY(true); // no crash; delivery is a no-op
}

QTEST_MAIN(HidConsumerTests)
