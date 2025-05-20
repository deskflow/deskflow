/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QLabel>

class QWidget;
class QDragEnterEvent;
class QDropEvent;

class TrashScreenWidget : public QLabel
{
  Q_OBJECT

public:
  TrashScreenWidget(QWidget *parent) : QLabel(parent)
  {
    // do nothing
  }

public:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;

signals:
  void screenRemoved();
};
