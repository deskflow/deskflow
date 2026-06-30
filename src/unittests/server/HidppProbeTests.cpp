/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "HidppProbeTests.h"

#include "server/HidppProbe.h"

#include <QTest>

void HidppProbeTests::decodeToJsonIncludesFeatIdx()
{
  deskflow::server::HidppDecodeContext context;
  context.featIdx = 0x0B;
  context.gestureCid = 0x01A0;
  context.extraDiverts[0x00C4] = "thumb_button";

  const auto json = deskflow::server::hidppDecodeToJson(context);
  QCOMPARE(json[QStringLiteral("feat_idx")].toInt(), 0x0B);
  QCOMPARE(json[QStringLiteral("gesture_cid")].toString(), QStringLiteral("0x01a0"));
  QVERIFY(json[QStringLiteral("extra_diverts")].toObject().contains(QStringLiteral("0x00c4")));
}

void HidppProbeTests::invalidContextProducesEmptyJson()
{
  const auto json = deskflow::server::hidppDecodeToJson({});
  QVERIFY(json.isEmpty());
}

void HidppProbeTests::probeCapableOnKnownPlatforms()
{
#if defined(__APPLE__) || defined(_WIN32)
  QVERIFY(deskflow::server::hidppProbeCapable());
#else
  QVERIFY(!deskflow::server::hidppProbeCapable());
#endif
}

QTEST_MAIN(HidppProbeTests)
