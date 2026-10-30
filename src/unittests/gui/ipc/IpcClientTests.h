/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class IpcClientTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void destroyedWhileHandlingMessages();
};
