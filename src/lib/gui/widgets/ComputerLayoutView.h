/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QGraphicsView>

class ComputerList;
class ComputerItem;

/**
 * @brief Shows each computer as a rectangle that can be dragged anywhere and resized from its bottom right corner.
 * Computers whose edges touch are linked, see deskflow::layout::computeLinks.
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
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private:
  void rebuild();
  void fit();
  void removeComputer(int index);
  void finishResize(ComputerItem *item);
  void dragOut(ComputerItem *item);
  ComputerItem *itemAt(const QPoint &pos) const;
  bool onResizeHandle(const ComputerItem *item, const QPoint &pos) const;

  ComputerList *m_computers = nullptr;
  ComputerItem *m_dragged = nullptr;
  ComputerItem *m_resized = nullptr;
};
