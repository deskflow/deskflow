/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 lxf82dev <lxf82dev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MacSmoothModeMenu.h"
#include "MacSmoothModeController.h"

#include <QDesktopServices>
#include <QUrl>

#include <utility>

MacSmoothModeMenu::MacSmoothModeMenu(QWidget *parent, QString controlPath, QString statusPath)
    : QMenu(tr("Smooth Mode"), parent),
      m_controller(new MacSmoothModeController(this, std::move(controlPath), std::move(statusPath)))
{
  m_summary = addAction(QString());
  m_summary->setObjectName("smoothStatus");
  m_summary->setEnabled(false);
  addSeparator();
  m_automatic = addAction(tr("Automatic smoothing"));
  m_automatic->setObjectName("smoothAutomatic");
  m_automatic->setCheckable(true);
  m_airdrop = addAction(tr("Allow AirDrop for 10 minutes"));
  m_airdrop->setObjectName("smoothAirDrop");
  m_airdrop->setCheckable(true);
  m_resume = addAction(tr("Resume automatic mode now"));
  m_resume->setObjectName("smoothResume");
  addSeparator();
  auto *log = addAction(tr("Show controller log"));
  connect(log, &QAction::triggered, this, [] {
    QDesktopServices::openUrl(QUrl::fromLocalFile("/Library/Logs/DeskflowMXAWDL/guard.log"));
  });
  auto *help = addAction(tr("Setup and help"));
  connect(help, &QAction::triggered, this, [] {
    QDesktopServices::openUrl(QUrl("https://github.com/deskflow/deskflow/blob/master/tools/macos-smooth-mode/README.md")
    );
  });
  connect(m_automatic, &QAction::triggered, m_controller, &MacSmoothModeController::setAutomatic);
  connect(m_airdrop, &QAction::triggered, m_controller, &MacSmoothModeController::allowAirDrop);
  connect(m_resume, &QAction::triggered, m_controller, &MacSmoothModeController::resume);
  connect(m_controller, &MacSmoothModeController::controlError, this, &MacSmoothModeMenu::controlError);
  connect(m_controller, &MacSmoothModeController::stateChanged, this, &MacSmoothModeMenu::updateControls);
  connect(this, &QMenu::aboutToShow, this, &MacSmoothModeMenu::refresh);
  updateControls();
}

void MacSmoothModeMenu::refresh()
{
  m_controller->refresh();
}

void MacSmoothModeMenu::updateControls()
{
  const auto &state = m_controller->state();
  m_summary->setText(state.summary);
  m_automatic->setEnabled(state.available);
  m_automatic->setChecked(state.automatic);
  m_airdrop->setEnabled(state.available && !state.paused);
  m_airdrop->setChecked(state.paused);
  m_resume->setEnabled(state.available && (state.paused || !state.automatic));
}
