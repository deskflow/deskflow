/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ComputerLayoutView.h"

#include "common/LayoutLinks.h"
#include "dialogs/ComputerSettingsDialog.h"
#include "gui/config/ComputerList.h"

#include <QDrag>
#include <QDragEnterEvent>
#include <QGraphicsRectItem>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMimeData>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionGraphicsItem>
#include <QToolButton>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

using namespace deskflow::layout;

namespace {
// Distance in view pixels within which a dragged computer snaps to a neighbour.
constexpr int kSnapPixels = 12;
constexpr int kMarginPixels = 16;
// Width in view pixels of the borders that resize a computer.
constexpr int kHandlePixels = 14;
constexpr QSize kMinimumSize{32, 18};
// Computers are about 100 units across, this leaves plenty of room to scroll around them.
constexpr QRectF kCanvas{-1000, -1000, 2000, 2000};
constexpr double kMaximumScale = 8.0;
constexpr double kZoomStep = 1.25;
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
    setToolTip(
        QObject::tr(
            "<center>Computer: <b>%1</b></center>"
            "<br>Drag next to another computer to link their edges"
            "<br>Drag an edge to resize"
            "<br>Double click to edit settings"
            "<br>Drag computer to the trashcan to remove it"
        )
            .arg(m_name)
    );
  }

  int index() const
  {
    return m_index;
  }

  const QString &name() const
  {
    return m_name;
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
    const QRectF textRect(
        device.left() + 4, device.center().y() + iconSize / 2.0 - textHeight / 2.0, device.width() - 8, textHeight
    );
    painter->setPen(palette.color(QPalette::Text));
    painter->drawText(
        textRect, Qt::AlignCenter,
        painter->fontMetrics().elidedText(m_name, Qt::ElideMiddle, static_cast<int>(textRect.width()))
    );

    painter->setPen(border);
    for (int offset = 4; offset < kHandlePixels; offset += 4) {
      painter->drawLine(device.bottomRight() - QPointF(offset, 2), device.bottomRight() - QPointF(2, offset));
    }
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
  setRenderHint(QPainter::Antialiasing);
  // Always shown so they never resize the view while zooming.
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setDragMode(QGraphicsView::ScrollHandDrag);
  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

  m_zoomButtons = new QWidget(this);
  auto *buttonLayout = new QHBoxLayout(m_zoomButtons);
  buttonLayout->setContentsMargins(0, 0, 0, 0);
  const auto addZoomButton = [this, buttonLayout](const QString &text, const QString &toolTip, double factor) {
    auto *button = new QToolButton(m_zoomButtons);
    button->setText(text);
    button->setToolTip(toolTip);
    connect(button, &QToolButton::clicked, this, [this, factor] {
      setTransformationAnchor(QGraphicsView::AnchorViewCenter);
      zoomBy(factor);
      setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    });
    buttonLayout->addWidget(button);
  };
  addZoomButton(QStringLiteral("\u2212"), tr("Zoom out"), 1 / kZoomStep);
  addZoomButton(QStringLiteral("+"), tr("Zoom in"), kZoomStep);
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

void ComputerLayoutView::rebuild()
{
  m_dragged = nullptr;
  m_resized = nullptr;
  scene()->clear();
  if (m_computers == nullptr)
    return;

  for (int i = 0; i < m_computers->size(); i++) {
    const auto &computer = m_computers->at(i);
    if (!computer.isNull())
      scene()->addItem(new ComputerItem(i, computer));
  }
  if (m_autoFit)
    fit();
  else
    growSceneRect(transform().m11());
  updateLinkLines();
}

void ComputerLayoutView::updateLinkLines()
{
  // Taken from the items rather than the stored geometry so the lines follow a computer while it is dragged.
  QMap<QString, QRect> rects;
  for (const auto *item : scene()->items()) {
    if (const auto *computer = dynamic_cast<const ComputerItem *>(item); computer != nullptr)
      rects.insert(computer->name(), QRectF(computer->pos(), computer->rect().size()).toRect());
  }

  m_linkLines.clear();
  for (const auto &link : computeLinks(rects)) {
    const QRectF r = rects.value(link.from);
    const auto along = [](double start, double length, double percent) { return start + length * percent / 100; };
    switch (link.side) {
    case Side::Right:
    case Side::Left: {
      const double x = link.side == Side::Right ? r.x() + r.width() : r.x();
      m_linkLines.append({x, along(r.y(), r.height(), link.fromStart), x, along(r.y(), r.height(), link.fromEnd)});
      break;
    }
    case Side::Up:
    case Side::Down: {
      const double y = link.side == Side::Down ? r.y() + r.height() : r.y();
      m_linkLines.append({along(r.x(), r.width(), link.fromStart), y, along(r.x(), r.width(), link.fromEnd), y});
      break;
    }
    }
  }
  viewport()->update();
}

void ComputerLayoutView::drawForeground(QPainter *painter, const QRectF &rect)
{
  QGraphicsView::drawForeground(painter, rect);
  // Green so links don't look like the highlighted server border; cosmetic so the width is the same at any zoom.
  QPen pen(QColor(0x34, 0xc7, 0x59), 3);
  pen.setCosmetic(true);
  pen.setCapStyle(Qt::FlatCap);
  painter->setPen(pen);
  painter->drawLines(m_linkLines);
}

void ComputerLayoutView::fit()
{
  const auto bounds = scene()->itemsBoundingRect();
  if (bounds.isEmpty())
    return;

  const auto scale = std::min(
      (viewport()->width() - 2.0 * kMarginPixels) / bounds.width(),
      (viewport()->height() - 2.0 * kMarginPixels) / bounds.height()
  );
  if (scale <= 0)
    return;
  const auto clamped = std::clamp(scale, minimumScale(), kMaximumScale);
  scene()->setSceneRect(QRectF());
  growSceneRect(clamped);
  setTransform(QTransform::fromScale(clamped, clamped));
  centerOn(bounds.center());
}

double ComputerLayoutView::minimumScale() const
{
  // Down to the whole canvas taking a quarter of the view.
  const auto content = kCanvas.united(scene()->itemsBoundingRect());
  return std::max(viewport()->width() / content.width(), viewport()->height() / content.height()) / 4;
}

void ComputerLayoutView::growSceneRect(double scale)
{
  // Padding by a whole view keeps the scene bigger than the view at this zoom, so it is never re-centred and the
  // zoom stays under the cursor. Only growing it means the scroll position is never clamped, so nothing jumps.
  const auto pad = QSizeF(viewport()->size()) / scale;
  const auto needed =
      kCanvas.united(scene()->itemsBoundingRect()).adjusted(-pad.width(), -pad.height(), pad.width(), pad.height());
  scene()->setSceneRect(scene()->sceneRect().united(needed));
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

Qt::Edges ComputerLayoutView::resizeEdges(const ComputerItem *item, const QPoint &pos) const
{
  const auto rect = mapFromScene(item->sceneBoundingRect()).boundingRect();
  // Small boxes keep their middle free for moving.
  const int handleX = std::min(kHandlePixels, rect.width() / 4);
  const int handleY = std::min(kHandlePixels, rect.height() / 4);
  Qt::Edges edges;
  if (pos.x() - rect.left() < handleX)
    edges |= Qt::LeftEdge;
  else if (rect.right() - pos.x() < handleX)
    edges |= Qt::RightEdge;
  if (pos.y() - rect.top() < handleY)
    edges |= Qt::TopEdge;
  else if (rect.bottom() - pos.y() < handleY)
    edges |= Qt::BottomEdge;
  return edges;
}

Qt::CursorShape ComputerLayoutView::resizeCursor(Qt::Edges edges)
{
  if (edges == (Qt::LeftEdge | Qt::TopEdge) || edges == (Qt::RightEdge | Qt::BottomEdge))
    return Qt::SizeFDiagCursor;
  if (edges == (Qt::RightEdge | Qt::TopEdge) || edges == (Qt::LeftEdge | Qt::BottomEdge))
    return Qt::SizeBDiagCursor;
  if (edges == Qt::LeftEdge || edges == Qt::RightEdge)
    return Qt::SizeHorCursor;
  if (edges == Qt::TopEdge || edges == Qt::BottomEdge)
    return Qt::SizeVerCursor;
  return Qt::OpenHandCursor;
}

void ComputerLayoutView::finishResize(ComputerItem *item)
{
  auto &computer = (*m_computers)[item->index()];
  const auto others = m_computers->rects(computer.name()).values();
  const QRect resized(item->pos().toPoint(), item->rect().size().toSize());
  const auto snapped = snapResize(resized, m_resizeEdges, others, qRound(kSnapPixels / transform().m11()));
  if (snapped != computer.geometry() && !overlapsAny(snapped, others)) {
    computer.setGeometry(snapped);
    Q_EMIT computersChanged();
  }
  rebuild();
}

void ComputerLayoutView::dragOut(ComputerItem *item)
{
  // Leaving the view hands the computer over to a real drag so it can follow the cursor onto the trash.
  const int index = item->index();
  m_dragged = nullptr;
  if (auto *grabber = scene()->mouseGrabberItem(); grabber != nullptr)
    grabber->ungrabMouse();

  auto *mimeData = new QMimeData();
  mimeData->setData(mimeType(), {});
  auto *drag = new QDrag(this);
  drag->setMimeData(mimeData);
  drag->setPixmap(m_computers->at(index).pixmap());
  if (drag->exec(Qt::MoveAction) == Qt::MoveAction)
    removeComputer(index);
  else
    rebuild();
}

void ComputerLayoutView::resizeEvent(QResizeEvent *event)
{
  QGraphicsView::resizeEvent(event);
  m_zoomButtons->adjustSize();
  const auto corner = viewport()->geometry().bottomRight();
  // Some styles draw the scroll bars over the view, so leave room for them.
  const int gap = style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, this) / 2 + 8;
  m_zoomButtons->move(corner - QPoint(m_zoomButtons->width(), m_zoomButtons->height()) - QPoint(gap, gap));
  if (m_autoFit)
    fit();
  else
    growSceneRect(transform().m11());
}

void ComputerLayoutView::showEvent(QShowEvent *event)
{
  QGraphicsView::showEvent(event);
  // Only fit when first shown; resizing the dialog afterwards keeps the zoom.
  if (m_autoFit)
    fit();
  m_autoFit = false;
}

void ComputerLayoutView::wheelEvent(QWheelEvent *event)
{
  if (!(event->modifiers() & Qt::ControlModifier)) {
    QGraphicsView::wheelEvent(event);
    return;
  }
  zoomBy(std::pow(2.0, event->angleDelta().y() / 480.0));
}

void ComputerLayoutView::zoomBy(double factor)
{
  m_autoFit = false;
  const auto current = transform().m11();
  const auto target = std::clamp(current * factor, minimumScale(), kMaximumScale);
  growSceneRect(std::min(current, target));
  QGraphicsView::scale(target / current, target / current);
}

void ComputerLayoutView::mousePressEvent(QMouseEvent *event)
{
  m_autoFit = false;
  auto *item = event->button() == Qt::LeftButton ? itemAt(event->pos()) : nullptr;
  if (item != nullptr && resizeEdges(item, event->pos())) {
    m_resized = item;
    m_resizeEdges = resizeEdges(item, event->pos());
    return;
  }
  m_dragged = item;
  QGraphicsView::mousePressEvent(event);
}

void ComputerLayoutView::mouseMoveEvent(QMouseEvent *event)
{
  if (m_resized != nullptr) {
    const auto cursor = mapToScene(event->pos());
    QRectF rect(m_resized->pos(), m_resized->rect().size());
    if (m_resizeEdges & Qt::LeftEdge)
      rect.setLeft(std::min(cursor.x(), rect.right() - kMinimumSize.width()));
    if (m_resizeEdges & Qt::RightEdge)
      rect.setRight(std::max(cursor.x(), rect.left() + kMinimumSize.width()));
    if (m_resizeEdges & Qt::TopEdge)
      rect.setTop(std::min(cursor.y(), rect.bottom() - kMinimumSize.height()));
    if (m_resizeEdges & Qt::BottomEdge)
      rect.setBottom(std::max(cursor.y(), rect.top() + kMinimumSize.height()));
    m_resized->setPos(rect.topLeft());
    m_resized->setRect(QRectF(QPointF(), rect.size()));
    updateLinkLines();
    return;
  }

  if (m_dragged != nullptr && !m_dragged->isServer() && !viewport()->rect().contains(event->pos())) {
    dragOut(m_dragged);
    return;
  }

  if (auto *item = itemAt(event->pos()); item != nullptr && event->buttons() == Qt::NoButton)
    item->setCursor(resizeCursor(resizeEdges(item, event->pos())));
  QGraphicsView::mouseMoveEvent(event);
  if (m_dragged != nullptr)
    updateLinkLines();
}

void ComputerLayoutView::mouseReleaseEvent(QMouseEvent *event)
{
  if (auto *resized = std::exchange(m_resized, nullptr); resized != nullptr) {
    finishResize(resized);
    return;
  }

  QGraphicsView::mouseReleaseEvent(event);
  auto *item = std::exchange(m_dragged, nullptr);
  if (item == nullptr || event->button() != Qt::LeftButton)
    return;

  auto &computer = (*m_computers)[item->index()];

  const auto others = m_computers->rects(computer.name()).values();
  const QRect moved(item->pos().toPoint(), computer.geometry().size());
  const auto snapped = nearestFreeSpot(snap(moved, others, qRound(kSnapPixels / transform().m11())), others);
  if (snapped != computer.geometry()) {
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
  if (m_computers != nullptr && event->source() != this && event->mimeData()->hasFormat(mimeType()))
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
  m_autoFit = false;

  auto encoded = event->mimeData()->data(mimeType());
  QDataStream stream(&encoded, QIODevice::ReadOnly);
  int unusedColumn = -1;
  int unusedRow = -1;
  Computer computer;
  stream >> unusedColumn >> unusedRow >> computer;

  // Sized in view pixels so a new computer is always easy to see at the current zoom.
  const auto size = (QSizeF(ComputerList::kDefaultSize) / transform().m11()).toSize().expandedTo(kMinimumSize);
  const auto center = mapToScene(event->position().toPoint()).toPoint();
  const auto others = m_computers->rects().values();
  const auto placed = snap(
      QRect(center - QPoint(size.width() / 2, size.height() / 2), size), others, qRound(kSnapPixels / transform().m11())
  );
  computer.setGeometry(nearestFreeSpot(placed, others));
  m_computers->append(computer);

  event->acceptProposedAction();
  rebuild();
  Q_EMIT computersChanged();
}
