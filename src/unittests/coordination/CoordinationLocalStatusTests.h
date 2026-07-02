/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>

class CoordinationLocalStatusTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void serverHostsFromStatus_excludesSelf();
  void serverHostsFromStatus_includesPeerAddresses();
  void pollLocalFleetStatus_readsSnapshot();
};
