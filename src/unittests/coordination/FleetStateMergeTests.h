/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>

class FleetStateMergeTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void emptyFragmentIsNoop();
  void serverFragmentReplacesTopology();
  void staleFragmentIsIgnored();
  void cursorUpdatePreservesOrdering();
  void topologyBecameReadyFlag();
};
