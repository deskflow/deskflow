/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>

class VirtualHostTrackerTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void connectsOnRemoteFocus();
  void movesVirtualHostWhenFocusChanges();
  void skipsConnectOnPrimaryFocus();
  void clearHostIfClearsMatchingHost();
  void detachSendsDisconnectAndClearsHost();
  void hostsActiveClientMatchesRelayTarget();
  void connectPayloadOverrideBypassesCachedLine();
};
