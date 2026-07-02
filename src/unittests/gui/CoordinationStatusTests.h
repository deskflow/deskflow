/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class CoordinationStatusTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void formatFleetGraph_emptyFleet();
  void formatFleetGraph_screensAndEdges();
  void poll_emitsOnlineWithFleetGraph();
  void poll_emitsOfflineWhenNoCoordinator();
};
