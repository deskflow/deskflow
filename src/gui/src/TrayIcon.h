/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2021 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>
#include <QTimer>
#include <functional>
#include <memory>

namespace deskflow::gui {

class TrayIcon : public QObject
{
  Q_OBJECT
public:
  explicit TrayIcon() = default;

  void create(std::vector<QAction *> const &actions);
  void setIcon(const QIcon &icon);

signals:
  void activated(QSystemTrayIcon::ActivationReason reason);

private:
  void showRetryLoop();

  std::unique_ptr<QSystemTrayIcon> m_pTrayIcon;
  std::unique_ptr<QMenu> m_pTrayIconMenu;
  std::function<void()> m_init;
  std::function<void(const QIcon &icon)> m_setIcon;
  QIcon m_icon;
};

} // namespace deskflow::gui
