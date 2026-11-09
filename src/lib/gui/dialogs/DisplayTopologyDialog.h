// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "gui/config/MonitorTopology.h"
#include <QDialog>
class DisplayTopologyDialog : public QDialog
{
  Q_OBJECT
public:
  explicit DisplayTopologyDialog(const deskflow::gui::MonitorTopology &topology, QWidget *parent = nullptr);
};
