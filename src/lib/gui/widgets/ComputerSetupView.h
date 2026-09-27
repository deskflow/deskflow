/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QFlags>
#include <QTableView>

class QWidget;
class QMouseEvent;
class QResizeEvent;
class QDragEnterEvent;
class ComputerSetupModel;

class ComputerSetupView : public QTableView
{
  Q_OBJECT

public:
  explicit ComputerSetupView(QWidget *parent);
  void setModel(QAbstractItemModel *model) override;
  ComputerSetupModel *model() const;

private:
  void showComputerConfig(int col, int row);

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
    // do nothing
  }
};
