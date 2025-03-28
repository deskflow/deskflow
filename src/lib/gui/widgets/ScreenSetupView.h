/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QFlags>
#include <QTableView>

#include "gui/config/Screen.h"

class QWidget;
class QMouseEvent;
class QResizeEvent;
class QDragEnterEvent;
class ScreenSetupModel;

class ScreenSetupView : public QTableView
{
  Q_OBJECT

public:
  ScreenSetupView(QWidget *parent);

public:
  void setModel(QAbstractItemModel *model) override;
  ScreenSetupModel *model() const;

private:
  void showScreenConfig(int col, int row);

protected:
  void mouseDoubleClickEvent(QMouseEvent *) override;
  void setTableSize();
  void resizeEvent(QResizeEvent *) override;
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void startDrag(Qt::DropActions supportedActions) override;
  void initViewItemOption(QStyleOptionViewItem *option) const override;
  void scrollTo(const QModelIndex &, ScrollHint) override
  {
  }
};
