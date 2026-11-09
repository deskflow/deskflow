/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTest>

class StatusBarTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void setStatus_connectingWithoutRetry_keepsTimerStopped();
  void setConnectionInterval_connecting_updatesTextImmediately();
  void setConnectionInterval_disconnected_showsScheduledRetry();
  void setConnectionInterval_newRetry_replacesWait();
  void setStatus_repeatedConnecting_doesNotConsumeWait();
  void setStatus_repeatedDisconnected_preservesWait();
  void setStatus_leavingConnecting_clearsWait_data();
  void setStatus_leavingConnecting_clearsWait();
  void setConnectionInterval_inactive_ignoresLateRetry_data();
  void setConnectionInterval_inactive_ignoresLateRetry();
  void setConnectionInterval_nonPositive_clearsWait_data();
  void setConnectionInterval_nonPositive_clearsWait();
  void setConnectionInterval_oneSecond_showsPendingWait();
  void setConnectionInterval_largeValue_doesNotOverflow();
  void setStatus_afterEventLoopDelay_usesElapsedTime();
  void retryTimer_afterDeadline_stops();
  void retryTimer_lateTimeout_keepsStoppedStatus();
};
