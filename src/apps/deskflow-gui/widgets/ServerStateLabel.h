/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QLabel>
#include <QStringList>

namespace deskflow::gui::widgets {

class ServerStateLabel : public QLabel
{
public:
  explicit ServerStateLabel(QWidget *parent = nullptr);
  void updateServerState(const QString &line);

private:
  QStringList m_clients;

  void updateState();
};

} // namespace deskflow::gui::widgets
