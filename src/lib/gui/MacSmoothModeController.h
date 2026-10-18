/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 lxf82dev <lxf82dev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>

// Shared user-level protocol for the menu and Preferences. Never installs a
// service or changes an interface directly.
class MacSmoothModeController : public QObject
{
  Q_OBJECT
public:
  struct State
  {
    bool available = false;
    bool automatic = false;
    bool paused = false;
    QString summary;
  };
  explicit MacSmoothModeController(QObject *parent, QString controlPath = {}, QString statusPath = {});
  const State &state() const;
  bool setAutomatic(bool enabled);
  bool allowAirDrop();
  bool resume();

public Q_SLOTS:
  void refresh();

Q_SIGNALS:
  void stateChanged();
  void controlError(const QString &message);

private:
  bool writeControl(const QByteArray &value);
  QString m_controlPath;
  QString m_statusPath;
  State m_state;
};
