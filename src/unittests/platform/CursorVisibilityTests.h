/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>

class CursorVisibilityTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void setHidden_localRestoreAndRemoteTransitions_balancesNativeCount();
  void setHidden_failedTransitions_canBeRetried();
  void setHidden_concurrentTransitions_serializeNativeCalls();
};
