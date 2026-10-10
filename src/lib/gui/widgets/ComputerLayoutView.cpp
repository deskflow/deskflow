/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ComputerLayoutView.h"

#include "dialogs/ComputerSettingsDialog.h"
#include "gui/config/ComputerList.h"
#include "gui/config/LayoutLinks.h"

#include <QDragEnterEvent>
#include <QGraphicsRectItem>
#include <QKeyEvent>
#include <QMimeData>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

using namespace deskflow::gui::layout;

namespace {
// Distance in view pixels within which a dragged computer snaps to a neighbour.
constexpr int kSnapPixels = 12;
constexpr int kMarginPixels = 16;
} // namespace

class ComputerItem : public QGraphicsRectItem
{
public:
  ComputerItem(int index, const Computer &computer)
      : QGraphicsRectItem(QRectF(QPointF(), computer.geometry().size())),
        m_index(index),
        m_name(computer.name()),
        m_pixmap(computer.pixmap()),
        m_isServer(computer.isServer())
  {
    setPos(computer.geometry().topLeft());
    setFlags(ItemIsMovable | ItemIsSelectable);
    setCursor(Qt::OpenHandCursor);
    setToolTip(QObject::tr("<center>Computer: <b>%1</b></center>"
                           "<br>Drag next to another computer to link their edges"
                           "<br>Double click to edit settings"
                           "<br>Drag computer to the trashcan to remove it")
                   .arg(m_name));
  }

  int index() const
  {
    return m_index;
  }

  bool isServer() const
  {
    return m_isServer;
  }

  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override
  {
    // Draw in device pixels so the icon and name stay readable however far the layout is zoomed out.
    const QRectF device = painter->worldTransform().mapRect(rect()).adjusted(1, 1, -1, -1);
    const auto &palette = widget != nullptr ? widget->palette() : option->palette;

    painter->save();
    painter->resetTransform();
    painter->setRenderHint(QPainter::Antialiasing);
    auto border = palette.color(QPalette::Text);
    border.setAlphaF(0.35f);
    painter->setPen(m_isServer ? QPen(palette.color(QPalette::Highlight), 2) : QPen(border, 1));
    painter->setBrush(palette.color(isSelected() ? QPalette::Midlight : QPalette::Button));
    painter->drawRoundedRect(device, 4, 4);

    const int textHeight = painter->fontMetrics().height();
    const int iconSize = std::min(64, static_cast<int>(device.height()) - textHeight - 8);
    if (iconSize >= 16) {
      const QRect iconRect(
          static_cast<int>(device.center().x()) - iconSize / 2,
          static_cast<int>(device.center().y()) - (iconSize + textHeight) / 2, iconSize, iconSize
      );
      painter->drawPixmap(iconRect, m_pixmap);
    }
    const QRectF textRect(device.left() + 4, device.center().y() + iconSize / 2.0 - textHeight / 2.0,
                          device.width() - 8, textHeight);
    painter->setPen(palette.color(QPalette::Text));
    painter->drawText(
        textRect, Qt::AlignCenter,
        painter->fontMetrics().elidedText(m_name, Qt::ElideMiddle, static_cast<int>(textRect.width()))
    );
    painter->restore();
  }

private:
  int m_index;
  QString m_name;
  QPixmap m_pixmap;
  bool m_isServer;
};

ComputerLayoutView::ComputerLayoutView(QWidget *parent) : QGraphicsView(parent)
{
  setScene(new QGraphicsScene(this));
  setAcceptDrops(true);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setRenderHint(QPainter::Antialiasing);
  setAlignment(Qt::AlignCenter);
}

const QString &ComputerLayoutView::mimeType()
{
  static const QString type = QStringLiteral("application/x-deskflow-computer");
  return type;
}

void ComputerLayoutView::setComputers(ComputerList *computers)
{
  m_computers = computers;
  rebuild();
}

void ComputerLayoutView::setTrashWidget(QWidget *trash)
{
  m_trash = trash;
}

void ComputerLayoutView::rebuild()
{
  m_dragged = nullptr;
  scene()->clear();
  if (m_computers == nullptr)
    return;

  for (int i = 0; i < m_computers->size(); i++) {
    const auto &computer = m_computers->at(i);
    if (!computer.isNull())
      scene()->addItem(new ComputerItem(i, computer));
  }
  fit();
}

void ComputerLayoutView::fit()
{
  const auto bounds = scene()->itemsBoundingRect();
  if (bounds.isEmpty())
    return;

  // Leave room around the computers, measured in view pixels.
  const auto scale = std::min(
      (viewport()->width() - 2.0 * kMarginPixels) / bounds.width(),
      (viewport()->height() - 2.0 * kMarginPixels) / bounds.height()
  );
  if (scale <= 0)
    return;
  const auto margin = kMarginPixels / scale;
  const auto padded = bounds.adjusted(-margin, -margin, margin, margin);
  scene()->setSceneRect(padded);
  fitInView(padded, Qt::KeepAspectRatio);
}

void ComputerLayoutView::removeComputer(int index)
{
  if (m_computers->at(index).isServer())
    return;
  m_computers->removeAt(index);
  rebuild();
  Q_EMIT computersChanged();
}

ComputerItem *ComputerLayoutView::itemAt(const QPoint &pos) const
{
  return dynamic_cast<ComputerItem *>(QGraphicsView::itemAt(pos));
}

void ComputerLayoutView::resizeEvent(QResizeEvent *event)
{
  QGraphicsView::resizeEvent(event);
  fit();
}

void ComputerLayoutView::mousePressEvent(QMouseEvent *event)
{
  m_dragged = event->button() == Qt::LeftButton ? itemAt(event->pos()) : nullptr;
  QGraphicsView::mousePressEvent(event);
}

void ComputerLayoutView::mouseReleaseEvent(QMouseEvent *event)
{
  QGraphicsView::mouseReleaseEvent(event);
  auto *item = std::exchange(m_dragged, nullptr);
  if (item == nullptr || event->button() != Qt::LeftButton)
    return;

  auto &computer = (*m_computers)[item->index()];
  if (m_trash && m_trash->rect().contains(m_trash->mapFromGlobal(event->globalPosition().toPoint()))) {
    removeComputer(item->index());
    return;
  }

  const auto others = m_computers->rects(&computer);
  const QRect moved(item->pos().toPoint(), computer.geometry().size());
  const auto snapped = snap(moved, others, qRound(kSnapPixels / transform().m11()));
  if (snapped != computer.geometry() && !overlapsAny(snapped, others)) {
    computer.setGeometry(snapped);
    Q_EMIT computersChanged();
  }
  // Redraw every item from the stored geometry, which also undoes moves of other selected items.
  rebuild();
}

void ComputerLayoutView::mouseDoubleClickEvent(QMouseEvent *event)
{
  auto *item = itemAt(event->pos());
  if (item == nullptr || event->button() != Qt::LeftButton) {
    QGraphicsView::mouseDoubleClickEvent(event);
    return;
  }

  ComputerSettingsDialog dialog(this, &(*m_computers)[item->index()], m_computers);
  dialog.exec();
  rebuild();
  Q_EMIT computersChanged();
}

void ComputerLayoutView::keyPressEvent(QKeyEvent *event)
{
  const auto selected = scene()->selectedItems();
  if ((event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) && selected.size() == 1) {
    removeComputer(static_cast<ComputerItem *>(selected.first())->index());
    return;
  }
  QGraphicsView::keyPressEvent(event);
}

void ComputerLayoutView::dragEnterEvent(QDragEnterEvent *event)
{
  dragMoveEvent(event);
}

void ComputerLayoutView::dragMoveEvent(QDragMoveEvent *event)
{
  if (m_computers != nullptr && event->mimeData()->hasFormat(mimeType()))
    event->acceptProposedAction();
  else
    event->ignore();
}

void ComputerLayoutView::dropEvent(QDropEvent *event)
{
  if (m_computers == nullptr || !event->mimeData()->hasFormat(mimeType())) {
    event->ignore();
    return;
  }

  auto encoded = event->mimeData()->data(mimeType());
  QDataStream stream(&encoded, QIODevice::ReadOnly);
  int unusedColumn = -1;
  int unusedRow = -1;
  Computer computer;
  stream >> unusedColumn >> unusedRow >> computer;

  // Center the new computer on the drop point, falling back to the next free spot by the server.
  const auto size = ComputerList::kDefaultSize;
  const auto center = mapToScene(event->position().toPoint()).toPoint();
  const auto others = m_computers->rects();
  const auto placed = snap(QRect(center - QPoint(size.width() / 2, size.height() / 2), size), others,
                           qRound(kSnapPixels / transform().m11()));
  if (overlapsAny(placed, others)) {
    m_computers->addComputerByPriority(computer);
  } else {
    computer.setGeometry(placed);
    m_computers->append(computer);
  }

  event->acceptProposedAction();
  rebuild();
  Q_EMIT computersChanged();
}
