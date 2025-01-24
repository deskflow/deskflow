/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QLabel>

namespace deskflow::gui::widgets {

class ClientStateLabel : public QLabel
{
public:
  explicit ClientStateLabel(QWidget *parent = nullptr);
  void updateClientState(const QString &line);
};

} // namespace deskflow::gui::widgets
