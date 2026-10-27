/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/LayoutLinks.h"

#include <QGraphicsView>

class ComputerList;
class ComputerItem;

/**
 * @brief Shows each computer as a rectangle that can be dragged anywhere and resized from any edge, Ctrl + scroll or
 * the zoom buttons zoom in and out. Each edge links to the nearest computers beyond it, see
 * deskflow::layout::computeLinks. Links are drawn as rays whose corners can be dragged to set a link by hand.
 */
class ComputerLayoutView : public QGraphicsView
{
  Q_OBJECT

public:
  explicit ComputerLayoutView(QWidget *parent = nullptr);

  static const QString &mimeType();

  void setComputers(ComputerList *computers);

Q_SIGNALS:
  void computersChanged();

protected:
  void resizeEvent(QResizeEvent *event) override;
  void showEvent(QShowEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dropEvent(QDropEvent *event) override;
  void drawForeground(QPainter *painter, const QRectF &rect) override;
  void contextMenuEvent(QContextMenuEvent *event) override;

private:
  void rebuild();
  void updateLinkRays();
  void dropStaleManualLinks();
  void setManualLink(const deskflow::layout::Link &link);
  void removeManualLink(const QString &a, const QString &b);
  void fit();
  double minimumScale() const;
  void growSceneRect(double scale);
  void removeComputer(int index);
  void finishResize(ComputerItem *item);
  void dragOut(ComputerItem *item);
  void zoomBy(double factor);
  ComputerItem *itemAt(const QPoint &pos) const;
  Qt::Edges resizeEdges(const ComputerItem *item, const QPoint &pos) const;
  static Qt::CursorShape resizeCursor(Qt::Edges edges);

  ComputerList *m_computers = nullptr;
  ComputerItem *m_dragged = nullptr;
  ComputerItem *m_resized = nullptr;
  Qt::Edges m_resizeEdges;
  // Fits the layout to the view until it is first shown or the user moves, resizes or zooms anything.
  bool m_autoFit = true;
  QWidget *m_zoomButtons = nullptr;
  struct LinkRay
  {
    deskflow::layout::Link link;
    // Scene coordinates from the start of the linked range on one computer's edge to the range on the other's and
    // back to the end of the first range.
    QPolygonF polygon;
    bool manual;
  };
  QList<LinkRay> m_linkRays;
  // The link whose ray corner is being dragged, and which corner of its polygon.
  std::optional<deskflow::layout::Link> m_adjusted;
  qsizetype m_adjustedCorner = 0;
};
