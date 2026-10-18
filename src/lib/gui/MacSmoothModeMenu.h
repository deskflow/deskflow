/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 lxf82dev <lxf82dev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QMenu>

class MacSmoothModeController;

// User-level controls for the separately installed, fixed-purpose AWDL worker.
// Creating the menu never installs services or changes interface flags.
class MacSmoothModeMenu : public QMenu
{
  Q_OBJECT
public:
  explicit MacSmoothModeMenu(QWidget *parent, QString controlPath = {}, QString statusPath = {});

public Q_SLOTS:
  void refresh();

Q_SIGNALS:
  void controlError(const QString &message);

private:
  void updateControls();
  MacSmoothModeController *m_controller;
  QAction *m_summary;
  QAction *m_automatic;
  QAction *m_airdrop;
  QAction *m_resume;
};
