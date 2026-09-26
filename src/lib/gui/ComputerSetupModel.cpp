/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ComputerSetupModel.h"

#include "common/Constants.h"
#include "gui/config/Screen.h"

#include <QIODevice>
#include <QIcon>
#include <QMimeData>

const QString ComputerSetupModel::m_MimeType = "application/x-deskflow-screen";

ComputerSetupModel::ComputerSetupModel(ScreenList &screens, int numColumns, int numRows)
    : QAbstractTableModel(nullptr),
      m_computers(screens),
      m_NumColumns(numColumns),
      m_NumRows(numRows)
{
  // bound the grid so that multiplying columns by rows cannot overflow.
  if (m_NumColumns < 1 || m_NumColumns > kMaxGridSize || m_NumRows < 1 || m_NumRows > kMaxGridSize) {
    qCritical("grid size out of bounds: %d columns x %d rows", m_NumColumns, m_NumRows);
    m_NumColumns = kServerGridWidth;
    m_NumRows = kServerGridHeight;
  }

  const int span = m_NumColumns * m_NumRows;
  if (span > m_computers.size()) {
    qCritical(
        "computer list too small for grid, computers: %lld, cells: %d", static_cast<long long>(m_computers.size()), span
    );
    m_computers.resize(span);
  }
}

QVariant ComputerSetupModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid() || index.row() > m_NumRows || index.row() < 0 || index.column() < 0 ||
      index.column() > m_NumColumns)
    return QVariant();

  if (computer(index).isNull())
    return QVariant();

  switch (role) {
  case Qt::DecorationRole:
    return computer(index).pixmap();

  case Qt::ToolTipRole:
    return QString(tr("<center>Computer: <b>%1</b></center>"
                      "<br>Double click to edit settings"
                      "<br>Drag computer to the trashcan to remove it"))
        .arg(computer(index).name());

  case Qt::DisplayRole:
    return computer(index).name();

  default:
    break;
  }
  return QVariant();
}

Qt::ItemFlags ComputerSetupModel::flags(const QModelIndex &index) const
{
  if (!index.isValid() || index.row() >= m_NumRows || index.column() >= m_NumColumns)
    return Qt::NoItemFlags;

  if (!computer(index).isNull())
    return Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;

  return Qt::ItemIsDropEnabled;
}

Qt::DropActions ComputerSetupModel::supportedDropActions() const
{
  return Qt::MoveAction | Qt::CopyAction;
}

QStringList ComputerSetupModel::mimeTypes() const
{
  return QStringList() << m_MimeType;
}

QMimeData *ComputerSetupModel::mimeData(const QModelIndexList &indexes) const
{
  auto *pMimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  for (const QModelIndex &index : indexes) {
    if (index.isValid())
      stream << index.column() << index.row() << computer(index);
  }

  pMimeData->setData(m_MimeType, encodedData);

  return pMimeData;
}

bool ComputerSetupModel::dropMimeData(
    const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent
)
{
  if (action == Qt::IgnoreAction)
    return true;

  if (!data->hasFormat(m_MimeType))
    return false;

  if (!parent.isValid() || row != -1 || column != -1)
    return false;

  QByteArray encodedData = data->data(m_MimeType);
  QDataStream stream(&encodedData, QIODevice::ReadOnly);

  int sourceColumn = -1;
  int sourceRow = -1;

  stream >> sourceColumn;
  stream >> sourceRow;

  const auto pColumn = parent.column();
  const auto pRow = parent.row();

  // don't drop screen onto itself
  if (sourceColumn == pColumn && sourceRow == pRow)
    return false;

  Screen droppedScreen;
  stream >> droppedScreen;

  if (auto oldScreen = Screen(computer(pColumn, pRow)); !oldScreen.isNull() && sourceColumn != -1 && sourceRow != -1) {
    // mark the screen so it isn't deleted after the dragndrop succeeded
    // see ScreenSetupView::startDrag()
    oldScreen.setSwapped(true);
    computer(sourceColumn, sourceRow) = oldScreen;
  }

  computer(pColumn, pRow) = droppedScreen;

  Q_EMIT computersChanged();

  return true;
}

void ComputerSetupModel::addComputer(const Screen &newComputer)
{
  m_computers.addScreenByPriority(newComputer);
  Q_EMIT computersChanged();
}

bool ComputerSetupModel::isFull() const
{
  auto emptyScreen = std::ranges::find_if(m_computers, [](const Screen &item) { return item.isNull(); });
  return (static_cast<QList<Screen>::const_iterator>(emptyScreen) == m_computers.cend());
}
