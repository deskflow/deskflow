/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

#include <string>

namespace deskflow::coordination {
class Coordinator;
}

class Arch;

class CoordinatorFleetPublishTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void cleanupTestCase();
  void updateCursorHost_updatesFleetSnapshot();
  void publishFleetTopology_updatesLinksAndScreens();
  void serverIgnoresInboundFleetMessage();
  void clientMergesInboundFleetMessage();

private:
  static void armAsServer(deskflow::coordination::Coordinator &coordinator, const std::string &selfName);
  static void armAsClient(deskflow::coordination::Coordinator &coordinator);
};
