/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "HidConsumerTests.h"

#include "client/HidConsumer.h"

#include <QTest>

void HidConsumerTests::encodesRawReportAsMouserLine()
{
  const std::string bytes = {'\x11', '\xFF', '\x0B'};
  const auto line = deskflow::client::encodeHidReportAsMouserLine(7, bytes);
  QCOMPARE(
      line,
      R"({"type": "report", "device_id": 7, "data": "11ff0b"})"
  );
}

void HidConsumerTests::emptyBytesProduceNoDelivery()
{
  const auto line = deskflow::client::encodeHidReportAsMouserLine(1, std::string{});
  QCOMPARE(line, R"({"type": "report", "device_id": 1, "data": ""})");
}

QTEST_MAIN(HidConsumerTests)
