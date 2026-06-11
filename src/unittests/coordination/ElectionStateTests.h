/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>

class ElectionStateTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initialRoleIsInit();
  void inputBurstPromotes();
  void slowInputNeverPromotes();
  void selfCooldownBlocksPromotion();
  void serverNeverPromotesAgain();
  void cursorHereRaisesThreshold();
  void cursorToggleClearsBurst();
  void claimFromSelfIgnored();
  void claimDuringServerCooldownIgnored();
  void claimAfterServerCooldownFollowed();
  void sameHostHeartbeatIsNoOp();
  void claimMergesSequenceNumbers();
  void transitionsResetBurstAndCursor();
};
