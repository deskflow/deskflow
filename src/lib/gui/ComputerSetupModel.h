/*
 * Deskflow -- mouse and keyboard sharing utility
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

class ComputerSetupView;
class ServerConfigDialog;

class ComputerSetupModel : public QAbstractTableModel
{
  Q_OBJECT

  friend class ComputerSetupView;
  friend class ServerConfigDialog;

public:
  ComputerSetupModel(ScreenList &screens, int numColumns, int numRows);

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

Q_SIGNALS:
  void computersChanged();

protected:
  bool
  dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
  const Screen &computer(const QModelIndex &index) const
  {
    return computer(index.column(), index.row());
  }
  Screen &computer(const QModelIndex &index)
  {
    return computer(index.column(), index.row());
  }
  const Screen &computer(int column, int row) const
  {
    return m_computers[row * m_NumColumns + column];
  }
  Screen &computer(int column, int row)
  {
    return m_computers[row * m_NumColumns + column];
  }
  void addComputer(const Screen &newComputer);

private:
  static constexpr int kMaxGridSize = 100;

  ScreenList &m_computers;
  int m_NumColumns;
  int m_NumRows;

  static const QString m_MimeType;
};
