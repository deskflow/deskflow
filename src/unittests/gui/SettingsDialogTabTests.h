/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTest>

class SettingsDialogTabTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void initTestCase();
  void tab_switch_preserves_selection();
  void updateDialogHeight_preserves_active_tab();

private:
  inline static const QString m_settingsPath = QStringLiteral("tmp/settings-dialog-tabs");
  inline static const QString m_settingsFile = QStringLiteral("%1/Deskflow.conf").arg(m_settingsPath);
  inline static const QString m_stateFile = QStringLiteral("%1/Deskflow.state").arg(m_settingsPath);
};
