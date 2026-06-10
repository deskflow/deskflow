/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Mikhail Slyusarev <slyusarevmikhail@gmail.com>
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
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

#include <algorithm>

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

  // needed to show resize cursors when hovering over a screen's edges
  setMouseTracking(true);
}

void ScreenSetupView::setModel(QAbstractItemModel *model)
{
  QTableView::setModel(model);
  setTableSize();
  connect(this->model(), &ScreenSetupModel::screensChanged, this, &ScreenSetupView::updateSpans);
  updateSpans();
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

void ScreenSetupView::updateSpans()
{
  clearSpans();
  for (int row = 0; row < model()->rowCount(); row++) {
    for (int col = 0; col < model()->columnCount(); col++) {
      if (const auto &screen = model()->screen(col, row);
          !screen.isNull() && (screen.width() > 1 || screen.height() > 1))
        setSpan(row, col, screen.height(), screen.width());
    }
  }
}

QModelIndex ScreenSetupView::resizeGripAt(const QPoint &pos, Qt::Orientation &orientation) const
{
  const auto index = indexAt(pos);
  if (!index.isValid() || model()->screen(index).isNull())
    return {};

  // visualRect() covers the whole span for a spanning screen
  const QRect rect = visualRect(index);
  const int gripSize = 10;
  if (pos.x() >= rect.right() - gripSize) {
    orientation = Qt::Horizontal;
    return index;
  }
  if (pos.y() >= rect.bottom() - gripSize) {
    orientation = Qt::Vertical;
    return index;
  }
  return {};
}

void ScreenSetupView::resizeSpanTo(const QPoint &pos)
{
  if (!m_resizeIndex.isValid())
    return;

  const auto &screen = model()->screen(m_resizeIndex.column(), m_resizeIndex.row());
  int width = screen.width();
  int height = screen.height();

  if (m_resizeOrientation == Qt::Horizontal) {
    int col = columnAt(pos.x());
    if (col == -1)
      col = pos.x() < 0 ? 0 : model()->columnCount() - 1;
    width = std::max(col - m_resizeIndex.column() + 1, 1);
  } else {
    int row = rowAt(pos.y());
    if (row == -1)
      row = pos.y() < 0 ? 0 : model()->rowCount() - 1;
    height = std::max(row - m_resizeIndex.row() + 1, 1);
  }

  model()->trySetSpan(m_resizeIndex.column(), m_resizeIndex.row(), width, height);
}

void ScreenSetupView::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton) {
    Qt::Orientation orientation = Qt::Horizontal;
    if (const auto grip = resizeGripAt(event->pos(), orientation); grip.isValid()) {
      m_resizeIndex = grip;
      m_resizeOrientation = orientation;
      return;
    }
  }
  QTableView::mousePressEvent(event);
}

void ScreenSetupView::mouseMoveEvent(QMouseEvent *event)
{
  if (m_resizeIndex.isValid()) {
    resizeSpanTo(event->pos());
    return;
  }

  Qt::Orientation orientation = Qt::Horizontal;
  if (resizeGripAt(event->pos(), orientation).isValid())
    viewport()->setCursor(orientation == Qt::Horizontal ? Qt::SizeHorCursor : Qt::SizeVerCursor);
  else
    viewport()->unsetCursor();

  QTableView::mouseMoveEvent(event);
}

void ScreenSetupView::mouseReleaseEvent(QMouseEvent *event)
{
  if (m_resizeIndex.isValid()) {
    m_resizeIndex = QPersistentModelIndex();
    return;
  }
  QTableView::mouseReleaseEvent(event);
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
    // indexAt() resolves clicks inside a span to its top left cell
    const auto index = indexAt(event->pos());

    if (index.isValid() && !model()->screen(index).isNull()) {
      showScreenConfig(index.column(), index.row());
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
      if (model()->m_Screens.screenIndexAt(col, row) != -1)
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
  if (pData == nullptr)
    return;

  const QPixmap &pixmap = model()->screen(indexes[0]).pixmap();
  auto *pDrag = new QDrag(this);
  pDrag->setPixmap(pixmap);
  pDrag->setMimeData(pData);
  pDrag->setHotSpot(QPoint(iconSize().width() / 2, iconSize().height() / 2));

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
