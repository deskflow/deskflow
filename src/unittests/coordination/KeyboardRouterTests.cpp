/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "KeyboardRouterTests.h"

#include "coordination/KeyboardRelayDecision.h"
#include "coordination/KeyboardRouter.h"

#include <QTest>

using deskflow::coordination::KeyboardRoute;
using deskflow::coordination::KeyboardRouteDecision;
using deskflow::coordination::KeyboardRouteInput;
using deskflow::coordination::routeKeyboard;

namespace {

KeyboardRouteInput makeInput(
    const char *self, const char *cursorHost, bool known, double elapsed = 1.0
)
{
  KeyboardRouteInput input;
  input.selfName = self;
  input.cursorHost = cursorHost;
  input.cursorHostKnown = known;
  input.secondsSinceRelayStart = elapsed;
  return input;
}

} // namespace

void KeyboardRouterTests::cursorOnSelf_isLocal()
{
  const auto decision = routeKeyboard(makeInput("server", "server", true));
  QCOMPARE(decision.route, KeyboardRoute::Local);
  QVERIFY(decision.forwardHost.empty());
}

void KeyboardRouterTests::cursorOnRemote_forwardsToHost()
{
  const auto decision = routeKeyboard(makeInput("laptop", "desktop", true));
  QCOMPARE(decision.route, KeyboardRoute::Forward);
  QCOMPARE(decision.forwardHost, std::string("desktop"));
}

void KeyboardRouterTests::unknownCursor_usesBootGrace()
{
  KeyboardRouteInput duringGrace;
  duringGrace.selfName = "laptop";
  duringGrace.cursorHostKnown = false;
  duringGrace.secondsSinceRelayStart = 0.1;
  QCOMPARE(routeKeyboard(duringGrace).route, KeyboardRoute::Local);

  KeyboardRouteInput afterGrace;
  afterGrace.selfName = "laptop";
  afterGrace.cursorHostKnown = false;
  afterGrace.secondsSinceRelayStart = 1.0;
  const auto decision = routeKeyboard(afterGrace);
  QCOMPARE(decision.route, KeyboardRoute::Forward);
  QVERIFY(decision.forwardHost.empty());
}

void KeyboardRouterTests::matrix_serverLocal_slaveForwards()
{
  const auto server = routeKeyboard(makeInput("hackintosh", "hackintosh", true));
  QCOMPARE(server.route, KeyboardRoute::Local);

  const auto slave = routeKeyboard(makeInput("macbookpro", "hackintosh", true));
  QCOMPARE(slave.route, KeyboardRoute::Forward);
  QCOMPARE(slave.forwardHost, std::string("hackintosh"));
}

void KeyboardRouterTests::matrix_remoteCursor_serverForwards()
{
  const auto server = routeKeyboard(makeInput("hackintosh", "tiny11", true));
  QCOMPARE(server.route, KeyboardRoute::Forward);
  QCOMPARE(server.forwardHost, std::string("tiny11"));

  const auto remote = routeKeyboard(makeInput("tiny11", "tiny11", true));
  QCOMPARE(remote.route, KeyboardRoute::Local);
}

void KeyboardRouterTests::matrix_planAcceptance_data()
{
  QTest::addColumn<QString>("self");
  QTest::addColumn<QString>("cursor");
  QTest::addColumn<int>("expectedRoute");
  QTest::addColumn<QString>("expectedHost");

  const auto local = static_cast<int>(KeyboardRoute::Local);
  const auto forward = static_cast<int>(KeyboardRoute::Forward);

  // hackintosh as elected server / local cursor host
  QTest::newRow("hackintosh-local-macbookpro-types") << QStringLiteral("macbookpro") << QStringLiteral("hackintosh")
                                                     << forward << QStringLiteral("hackintosh");
  QTest::newRow("hackintosh-local-hackintosh-types") << QStringLiteral("hackintosh") << QStringLiteral("hackintosh")
                                                     << local << QString();
  QTest::newRow("tiny11-remote-macbookpro-types") << QStringLiteral("macbookpro") << QStringLiteral("tiny11") << forward
                                                  << QStringLiteral("tiny11");
  QTest::newRow("tiny11-remote-hackintosh-types") << QStringLiteral("hackintosh") << QStringLiteral("tiny11") << forward
                                                  << QStringLiteral("tiny11");

  // macbookpro as elected server / local cursor host
  QTest::newRow("macbookpro-local-hackintosh-types") << QStringLiteral("hackintosh") << QStringLiteral("macbookpro")
                                                     << forward << QStringLiteral("macbookpro");
  QTest::newRow("macbookpro-local-macbookpro-types") << QStringLiteral("macbookpro") << QStringLiteral("macbookpro")
                                                     << local << QString();
  QTest::newRow("tiny11-remote-hackintosh-from-macbookpro") << QStringLiteral("hackintosh") << QStringLiteral("tiny11")
                                                            << forward << QStringLiteral("tiny11");
  QTest::newRow("tiny11-remote-macbookpro-from-macbookpro") << QStringLiteral("macbookpro") << QStringLiteral("tiny11")
                                                            << forward << QStringLiteral("tiny11");

  // tiny11 as elected server / local cursor host
  QTest::newRow("tiny11-local-hackintosh-types") << QStringLiteral("hackintosh") << QStringLiteral("tiny11") << forward
                                                 << QStringLiteral("tiny11");
  QTest::newRow("tiny11-local-tiny11-types") << QStringLiteral("tiny11") << QStringLiteral("tiny11") << local
                                             << QString();
  QTest::newRow("hackintosh-remote-macbookpro-types") << QStringLiteral("macbookpro") << QStringLiteral("hackintosh")
                                                      << forward << QStringLiteral("hackintosh");
  QTest::newRow("hackintosh-remote-tiny11-types") << QStringLiteral("tiny11") << QStringLiteral("hackintosh") << forward
                                                  << QStringLiteral("hackintosh");
}

void KeyboardRouterTests::matrix_planAcceptance()
{
  QFETCH(QString, self);
  QFETCH(QString, cursor);
  QFETCH(int, expectedRoute);
  QFETCH(QString, expectedHost);

  const auto decision = routeKeyboard(makeInput(self.toUtf8().constData(), cursor.toUtf8().constData(), true));
  QCOMPARE(static_cast<int>(decision.route), expectedRoute);
  QCOMPARE(QString::fromStdString(decision.forwardHost), expectedHost);
}

QTEST_MAIN(KeyboardRouterTests)
