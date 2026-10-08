/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>

class OSXMouseEventTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void markedMovement_copyKeepsMarkerAndCannotRestore_data();
  void markedMovement_copyKeepsMarkerAndCannotRestore();
  void unmarkedMovement_restoresAtAnyPosition_data();
  void unmarkedMovement_restoresAtAnyPosition();
  void unrelatedEvents_doNotRestore_data();
  void unrelatedEvents_doNotRestore();
  void anotherApplicationsUserData_stillRestores();
  void nullEvents_neverRestore();
};
