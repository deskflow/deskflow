/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QLabel>

class QMouseEvent;
class QWidget;

class NewScreenWidget : public QLabel
{
  Q_OBJECT

public:
  NewScreenWidget(QWidget *parent);

protected:
  void mousePressEvent(QMouseEvent *event) override;
};
