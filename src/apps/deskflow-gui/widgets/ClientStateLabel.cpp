/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ClientStateLabel.h"

namespace deskflow::gui::widgets {

ClientStateLabel::ClientStateLabel(QWidget *parent) : QLabel(parent)
{
  hide();
}

void ClientStateLabel::updateClientState(const QString &line)
{
  if (line.contains("connected to server")) {
    show();
  } else if (line.contains("disconnected from server") || line.contains("process exited")) {
    hide();
  }
}

} // namespace deskflow::gui::widgets
