/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Mikhail Slyusarev <slyusarevmikhail@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QAbstractTableModel>
#include <QList>
#include <QString>
#include <QStringList>

#include "gui/config/ScreenList.h"

class ScreenSetupView;
class ServerConfigDialog;

class ScreenSetupModel : public QAbstractTableModel
{
  Q_OBJECT

  friend class ScreenSetupView;
  friend class ServerConfigDialog;

public:
  // QSize(width, height), in grid cells, of the screen at the index -- so a
  // delegate can distinguish a 1x1 screen from a spanning one, and a wide span
  // from a tall one, without reaching into the model
  static constexpr int SpanRole = Qt::UserRole + 1;

  ScreenSetupModel(ScreenList &screens, int numColumns, int numRows);

  static const QString &mimeType()
  {
    return m_MimeType;
  }
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  int rowCount() const
  {
    return m_NumRows;
  }
  int columnCount() const
  {
    return m_NumColumns;
  }
  int rowCount(const QModelIndex &) const override
  {
    return rowCount();
  }
  int columnCount(const QModelIndex &) const override
  {
    return columnCount();
  }
  Qt::DropActions supportedDropActions() const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QStringList mimeTypes() const override;
  QMimeData *mimeData(const QModelIndexList &indexes) const override;
  bool isFull() const;

  //! Resize the screen anchored at the given cell, rejecting spans that
  //! would leave the grid or cover an occupied cell
  bool trySetSpan(int column, int row, int width, int height);

Q_SIGNALS:
  void screensChanged();

protected:
  bool
  dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
  const Screen &screen(const QModelIndex &index) const
  {
    return screen(index.column(), index.row());
  }
  Screen &screen(const QModelIndex &index)
  {
    return screen(index.column(), index.row());
  }
  const Screen &screen(int column, int row) const
  {
    return m_Screens[row * m_NumColumns + column];
  }
  Screen &screen(int column, int row)
  {
    return m_Screens[row * m_NumColumns + column];
  }
  void addScreen(const Screen &newScreen);

private:
  ScreenList &m_Screens;
  const int m_NumColumns;
  const int m_NumRows;

  // set by dropMimeData when it moves a screen within the grid, so startDrag
  // knows the source was already handled and only a trash drop must clear it
  bool m_dropHandled = false;

  static const QString m_MimeType;
};
