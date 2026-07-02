/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTest>

class KeyboardRouterTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void cursorOnSelf_isLocal();
  void cursorOnRemote_forwardsToHost();
  void unknownCursor_usesBootGrace();
  void matrix_serverLocal_slaveForwards();
  void matrix_remoteCursor_serverForwards();
  void matrix_planAcceptance_data();
  void matrix_planAcceptance();
};
