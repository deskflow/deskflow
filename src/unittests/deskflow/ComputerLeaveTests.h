/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/Arch.h"
#include "base/Log.h"

#include <QTest>

class ComputerLeaveTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void disconnectReleasesHeldButton();
  void disconnectSkipsReleasedButton();

private:
  Arch m_arch;
  Log m_log;
  inline static const QString m_settingsFile = QStringLiteral("tmp/test/ComputerLeaveTests.conf");
};
