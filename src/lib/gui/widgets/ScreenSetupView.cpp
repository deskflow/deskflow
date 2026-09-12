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
#include <QDropEvent>
#include <QHeaderView>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QStyledItemDelegate>

#include <algorithm>

namespace {

// keeps the icon+name block vertically centred (so a tall screen's label is
// not stranded at the top) and outlines spanning screens so a wide or tall
// item reads as a single unit
class ScreenDelegate : public QStyledItemDelegate
{
public:
  using QStyledItemDelegate::QStyledItemDelegate;

  void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
  {
    QStyleOptionViewItem opt = option;
    const int contentHeight = opt.decorationSize.height() + opt.fontMetrics.height() + 4;
    if (opt.rect.height() > contentHeight) {
      const int dy = (opt.rect.height() - contentHeight) / 2;
      opt.rect.adjust(0, dy, 0, -dy);
    }
    QStyledItemDelegate::paint(painter, opt, index);

    if (const QSize span = index.data(ScreenSetupModel::SpanRole).toSize(); span.width() > 1 || span.height() > 1) {
      painter->save();
      painter->setRenderHint(QPainter::Antialiasing);
      // a thin neutral outline so a wide/tall item reads as a single unit.
      // deliberately NOT the selection highlight, which made spanning screens
      // look permanently selected
      QPen pen(option.palette.color(QPalette::Mid));
      pen.setWidth(1);
      painter->setPen(pen);
      painter->setBrush(Qt::NoBrush);
      painter->drawRoundedRect(QRectF(option.rect).adjusted(1, 1, -1, -1), 6, 6);
      painter->restore();
    }
  }
};

} // namespace

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
  setItemDelegate(new ScreenDelegate(this));

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

QModelIndex ScreenSetupView::resizeGripAt(const QPoint &pos, ResizeAxis &axis) const
{
  const auto index = indexAt(pos);
  if (!index.isValid() || model()->screen(index).isNull())
    return {};

  // visualRect() covers the whole span for a spanning screen
  const QRect rect = visualRect(index);
  const int gripSize = 10;
  const bool nearRight = pos.x() >= rect.right() - gripSize;
  const bool nearBottom = pos.y() >= rect.bottom() - gripSize;

  if (nearRight && nearBottom)
    axis = ResizeAxis::Both;
  else if (nearRight)
    axis = ResizeAxis::Width;
  else if (nearBottom)
    axis = ResizeAxis::Height;
  else
    return {};

  return index;
}

void ScreenSetupView::resizeSpanTo(const QPoint &pos)
{
  if (!m_resizeIndex.isValid())
    return;

  const auto &screen = model()->screen(m_resizeIndex.column(), m_resizeIndex.row());
  int width = screen.width();
  int height = screen.height();

  if (m_resizeAxis != ResizeAxis::Height) {
    int col = columnAt(pos.x());
    if (col == -1)
      col = pos.x() < 0 ? 0 : model()->columnCount() - 1;
    width = std::max(col - m_resizeIndex.column() + 1, 1);
  }
  if (m_resizeAxis != ResizeAxis::Width) {
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
    ResizeAxis axis = ResizeAxis::Width;
    if (const auto grip = resizeGripAt(event->pos(), axis); grip.isValid()) {
      m_resizeIndex = grip;
      m_resizeAxis = axis;
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

  ResizeAxis axis = ResizeAxis::Width;
  if (resizeGripAt(event->pos(), axis).isValid()) {
    const auto cursor = axis == ResizeAxis::Both    ? Qt::SizeFDiagCursor
                        : axis == ResizeAxis::Width ? Qt::SizeHorCursor
                                                    : Qt::SizeVerCursor;
    viewport()->setCursor(cursor);
  } else
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

void ScreenSetupView::dropEvent(QDropEvent *event)
{
  if (!event->mimeData()->hasFormat(ScreenSetupModel::mimeType())) {
    event->ignore();
    return;
  }

  // Resolve the true cell under the cursor. The base QTableView drop path uses
  // indexAt(), which collapses any point inside a span back to the span's
  // anchor -- that makes a spanning screen impossible to nudge by a single cell
  // or drop into a corner (the target always resolves to its current anchor).
  // columnAt()/rowAt() are span-agnostic, so use them as the drop anchor.
  const QPoint pos = event->position().toPoint();
  const int col = columnAt(pos.x());
  const int row = rowAt(pos.y());
  if (col < 0 || row < 0) {
    event->ignore();
    return;
  }

  const bool internal = event->source() == this;
  if (model()->dropMimeData(event->mimeData(), Qt::MoveAction, -1, -1, model()->index(row, col))) {
    if (internal) {
      event->setDropAction(Qt::MoveAction);
      event->accept();
    } else {
      event->acceptProposedAction();
    }
  } else {
    event->ignore();
  }
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

  model()->m_dropHandled = false;
  if (pDrag->exec(Qt::MoveAction, Qt::MoveAction) == Qt::MoveAction) {
    selectionModel()->clear();

    // a grid drop moves the source itself (dropMimeData); a trash drop does
    // not touch the model, so the source is removed here in that case
    if (!model()->m_dropHandled) {
      model()->screen(indexes[0]) = Screen();
      Q_EMIT model()->screensChanged();
    }
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
