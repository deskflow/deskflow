/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QGraphicsView>
#include <QPointer>

class ComputerList;
class ComputerItem;

/**
 * @brief Shows each computer as a rectangle sized by its resolution that can be dragged anywhere.
 * Computers whose edges touch are linked, see deskflow::gui::layout::computeLinks.
 */
class ComputerLayoutView : public QGraphicsView
{
  Q_OBJECT

public:
  explicit ComputerLayoutView(QWidget *parent = nullptr);

  static const QString &mimeType();

  void setComputers(ComputerList *computers);

  /**
   * @brief setTrashWidget computers released over this widget are removed
   */
  void setTrashWidget(QWidget *trash);

Q_SIGNALS:
  void computersChanged();

protected:
  void resizeEvent(QResizeEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
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
  ComputerItem *itemAt(const QPoint &pos) const;

  ComputerList *m_computers = nullptr;
  QPointer<QWidget> m_trash;
  ComputerItem *m_dragged = nullptr;
};
