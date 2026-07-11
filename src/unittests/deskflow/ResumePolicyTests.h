/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "client/ResumePolicy.h"

#include <QTest>

class ResumePolicyTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void suspendResumeReconnectsWhenPreviouslyConnected();
  void suspendResumeDoesNothingWhenNotPreviouslyConnected();
  void missedSuspendWhileConnectedDropsStaleConnection();
  void missedSuspendWhileDisconnectedDoesNothing();
};
