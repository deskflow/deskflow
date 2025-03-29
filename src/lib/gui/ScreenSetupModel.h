/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
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
  ScreenSetupModel(ScreenList &screens, int numColumns, int numRows);

public:
  static const QString &mimeType()
  {
    return m_MimeType;
  }
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  int rowCount() const
  {
    return m_NumRows;
  }
  int columnCount() const
  {
    return m_NumColumns;
  }
  int rowCount(const QModelIndex &) const
  {
    return rowCount();
  }
  int columnCount(const QModelIndex &) const
  {
    return columnCount();
  }
  Qt::DropActions supportedDropActions() const;
  Qt::ItemFlags flags(const QModelIndex &index) const;
  QStringList mimeTypes() const;
  QMimeData *mimeData(const QModelIndexList &indexes) const;
  bool isFull() const;

signals:
  void screensChanged();

protected:
  bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
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

  static const QString m_MimeType;
};
