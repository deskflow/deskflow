/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTest>

class MainWindowVisibilityTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void initTestCase();
  void cleanupTestCase();
  void hide_marks_window_not_visible();
  void show_after_hide_marks_window_visible();
#if defined(Q_OS_MACOS)
  void macOS_dock_policy_round_trip();
  void macOS_hide_removes_dock_icon();
  void macOS_show_restores_dock_icon();
#endif

private:
  inline static const QString m_settingsPath = QStringLiteral("tmp/mainwindow-visibility");
  inline static const QString m_settingsFile = QStringLiteral("%1/Deskflow.conf").arg(m_settingsPath);
  inline static const QString m_stateFile = QStringLiteral("%1/Deskflow.state").arg(m_settingsPath);
};
