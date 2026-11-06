/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class IKeyStateTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void allocDestination();
  void alloc_noActiveComputerOnly_false();
  void alloc_activeComputerOnly_true();
  void allocCopy_activeComputerOnly_true();
  void equal_activeComputerOnlyDiffers_false();
};
