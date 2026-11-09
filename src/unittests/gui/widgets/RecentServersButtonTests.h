/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once
#include <QTest>

class RecentServersButtonTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void successfulConnection_enablesOneClickReconnect();
  void menuSelection_keepsTheSelectedPort();
  void securityDisabled_doesNotRecordOrReconnect();
  void cancelledAttempt_ignoresLateMessages();
  void history_roundTripsWithoutStartingAConnection();
  void clearHistory_doesNotChangeTrustOrConnect();
  void malformedHistory_staysDisabled();
  void longTarget_isBoundedAndKeyboardAccessible();
};
