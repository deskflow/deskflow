/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class LayoutLinksTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void fullEdge();
  void halfEdge();
  void differentHeights();
  void twoOnOneSide();
  void upDown();
  void gap();
  void cornerOnly();
  void snapFlush();
  void snapNoneWhenFar();
  void overlap();
};
