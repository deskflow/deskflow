/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <QTest>

class CoreProcessTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void initTestCase();
  void stop_terminatesDesktopChild();
  void stop_killsDesktopChildIgnoringTerminate();

private:
  inline static const QString m_settingsPath = QStringLiteral("tmp/CoreProcessTests");
  inline static const QString m_settingsFile = QStringLiteral("%1/Deskflow.conf").arg(m_settingsPath);
  inline static const QString m_stateFile = QStringLiteral("%1/Deskflow.state").arg(m_settingsPath);
};
