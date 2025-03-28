/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ScreenSetupView.h"

#include "ScreenSetupModel.h"
#include "dialogs/ScreenSettingsDialog.h"

#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QHeaderView>
#include <QMimeData>
#include <QMouseEvent>
#include <QResizeEvent>

ScreenSetupView::ScreenSetupView(QWidget *parent) : QTableView(parent)
{
  setDropIndicatorShown(true);
  setDragDropMode(DragDrop);
  setSelectionMode(SingleSelection);

  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  setIconSize(QSize(96, 96));
  horizontalHeader()->hide();
  verticalHeader()->hide();
}

void ScreenSetupView::setModel(QAbstractItemModel *model)
{
  QTableView::setModel(model);
  setTableSize();
}

ScreenSetupModel *ScreenSetupView::model() const
{
  return qobject_cast<ScreenSetupModel *>(QTableView::model());
}

void ScreenSetupView::showScreenConfig(int col, int row)
{
  ScreenSettingsDialog dlg(this, &model()->screen(col, row), &model()->m_Screens);
  dlg.exec();
  Q_EMIT model()->screensChanged();
}

void ScreenSetupView::setTableSize()
{
  for (int i = 0; i < model()->columnCount(); i++)
    setColumnWidth(i, width() / model()->columnCount());

  for (int i = 0; i < model()->rowCount(); i++)
    setRowHeight(i, height() / model()->rowCount());
}

void ScreenSetupView::resizeEvent(QResizeEvent *event)
{
  setTableSize();
  event->ignore();
}

void ScreenSetupView::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (event->buttons() & Qt::LeftButton) {
    int col = columnAt(event->pos().x());
    int row = rowAt(event->pos().y());

    if (!model()->screen(col, row).isNull()) {
      showScreenConfig(col, row);
    }
  } else
    event->ignore();
}

void ScreenSetupView::dragEnterEvent(QDragEnterEvent *event)
{
  // we accept anything that enters us by a drag as long as the
  // mime type is okay. anything else is dealt with in dragMoveEvent()
  if (event->mimeData()->hasFormat(ScreenSetupModel::mimeType()))
    event->accept();
  else
    event->ignore();
}

void ScreenSetupView::dragMoveEvent(QDragMoveEvent *event)
{
  if (event->mimeData()->hasFormat(ScreenSetupModel::mimeType())) {
    // where does the event come from? myself or someone else?
    if (event->source() == this) {
      // myself is ok, but then it must be a move action, never a copy
      event->setDropAction(Qt::MoveAction);
      event->accept();
    } else {
      const auto &point = event->position().toPoint();
      int col = columnAt(point.x());
      int row = rowAt(point.y());

      // a drop from outside is not allowed if there's a screen already there.
      if (!model()->screen(col, row).isNull())
        event->ignore();
      else {
        event->acceptProposedAction();
      }
    }
  } else
    event->ignore();
}

// this is reimplemented from QAbstractItemView::startDrag()
void ScreenSetupView::startDrag(Qt::DropActions)
{
  QModelIndexList indexes = selectedIndexes();

  if (indexes.count() != 1)
    return;

  QMimeData *pData = model()->mimeData(indexes);
  if (pData == NULL)
    return;

  const QPixmap &pixmap = model()->screen(indexes[0]).pixmap();
  QDrag *pDrag = new QDrag(this);
  pDrag->setPixmap(pixmap);
  pDrag->setMimeData(pData);
  pDrag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));

  if (pDrag->exec(Qt::MoveAction, Qt::MoveAction) == Qt::MoveAction) {
    selectionModel()->clear();

    // make sure to only delete the drag source if screens weren't swapped
    // see ScreenSetupModel::dropMimeData
    if (!model()->screen(indexes[0]).swapped())
      model()->screen(indexes[0]) = Screen();
    else
      model()->screen(indexes[0]).setSwapped(false);

    Q_EMIT model()->screensChanged();
  }
}

void ScreenSetupView::initViewItemOption(QStyleOptionViewItem *option) const
{
  // HACK make a basic widget and init from it
  auto w = new QWidget();
  option->initFrom(w);
  w->deleteLater();
  delete w;

  option->decorationSize = QSize(96, 96);
  option->showDecorationSelected = true;
  option->decorationPosition = QStyleOptionViewItem::Top;
  option->decorationAlignment = Qt::AlignHCenter | Qt::AlignVCenter;
  option->displayAlignment = Qt::AlignTop | Qt::AlignHCenter;
  option->textElideMode = Qt::ElideMiddle;
}
