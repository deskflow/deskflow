/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Mikhail Slyusarev <slyusarevmikhail@gmail.com>
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ScreenSetupModel.h"

#include <QIODevice>
#include <QIcon>
#include <QMimeData>

#include "gui/config/Screen.h"

const QString ScreenSetupModel::m_MimeType = "application/x-deskflow-screen";

namespace {

// a layout is valid when every screen's span stays inside the grid and
// covers only empty cells
bool isValidLayout(const ScreenList &screens, int numColumns)
{
  const int numRows = static_cast<int>(screens.size()) / numColumns;
  for (int i = 0; i < screens.size(); i++) {
    const auto &screen = screens[i];
    if (screen.isNull() || (screen.width() == 1 && screen.height() == 1))
      continue;

    const int column = i % numColumns;
    const int row = i / numColumns;
    if (column + screen.width() > numColumns || row + screen.height() > numRows)
      return false;

    for (int r = row; r < row + screen.height(); r++) {
      for (int c = column; c < column + screen.width(); c++) {
        if (r * numColumns + c != i && !screens[r * numColumns + c].isNull())
          return false;
      }
    }
  }
  return true;
}

} // namespace

ScreenSetupModel::ScreenSetupModel(ScreenList &screens, int numColumns, int numRows)
    : QAbstractTableModel(nullptr),
      m_Screens(screens),
      m_NumColumns(numColumns),
      m_NumRows(numRows)
{

  // bound rows and columns to prevent multiply overflow.
  // this is unlikely to happen, as the grid size is only 3x9.
  if (m_NumColumns > 100 || m_NumRows > 100) {
    qFatal("grid size out of bounds: %d columns x %d rows", m_NumColumns, m_NumRows);
    return;
  }

  const long span = static_cast<long>(m_NumColumns) * m_NumRows;
  if (span > screens.size()) {
    qFatal("scrren list (%lld) too small for %d columns x %d rows", screens.size(), m_NumColumns, m_NumRows);
  }
}

QVariant ScreenSetupModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid() || index.row() > m_NumRows || index.row() < 0 || index.column() < 0 ||
      index.column() > m_NumColumns)
    return QVariant();

  if (screen(index).isNull())
    return QVariant();

  switch (role) {
  case Qt::DecorationRole:
    return screen(index).pixmap();

  case Qt::ToolTipRole:
    return QString(tr("<center>Screen: <b>%1</b></center>"
                      "<br>Double click to edit settings"
                      "<br>Drag screen to the trashcan to remove it"
                      "<br>Drag the right or bottom edge to span more cells"))
        .arg(screen(index).name());

  case Qt::DisplayRole:
    return screen(index).name();

  default:
    break;
  }
  return QVariant();
}

Qt::ItemFlags ScreenSetupModel::flags(const QModelIndex &index) const
{
  if (!index.isValid() || index.row() >= m_NumRows || index.column() >= m_NumColumns)
    return Qt::NoItemFlags;

  if (!screen(index).isNull())
    return Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;

  return Qt::ItemIsDropEnabled;
}

Qt::DropActions ScreenSetupModel::supportedDropActions() const
{
  return Qt::MoveAction | Qt::CopyAction;
}

QStringList ScreenSetupModel::mimeTypes() const
{
  return QStringList() << m_MimeType;
}

QMimeData *ScreenSetupModel::mimeData(const QModelIndexList &indexes) const
{
  auto *pMimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  for (const QModelIndex &index : indexes) {
    if (index.isValid())
      stream << index.column() << index.row() << screen(index);
  }

  pMimeData->setData(m_MimeType, encodedData);

  return pMimeData;
}

bool ScreenSetupModel::dropMimeData(
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

  // the mime payload is external input, the source is either -1/-1 for a
  // new screen or a cell inside the grid
  const bool hasSource = sourceColumn != -1 || sourceRow != -1;
  if (hasSource && (sourceColumn < 0 || sourceColumn >= m_NumColumns || sourceRow < 0 || sourceRow >= m_NumRows))
    return false;

  const auto pColumn = parent.column();
  const auto pRow = parent.row();

  // don't drop screen onto itself
  if (sourceColumn == pColumn && sourceRow == pRow)
    return false;

  Screen droppedScreen;
  stream >> droppedScreen;

  // simulate the drop (and swap) to reject moves where a spanning screen
  // would stick out of the grid or cover an occupied cell
  ScreenList trial = m_Screens;
  if (sourceColumn != -1 && sourceRow != -1)
    trial[sourceRow * m_NumColumns + sourceColumn] = screen(pColumn, pRow);
  trial[pRow * m_NumColumns + pColumn] = droppedScreen;
  if (!isValidLayout(trial, m_NumColumns))
    return false;

  if (auto oldScreen = Screen(screen(pColumn, pRow)); !oldScreen.isNull() && sourceColumn != -1 && sourceRow != -1) {
    // mark the screen so it isn't deleted after the dragndrop succeeded
    // see ScreenSetupView::startDrag()
    oldScreen.setSwapped(true);
    screen(sourceColumn, sourceRow) = oldScreen;
  }

  screen(pColumn, pRow) = droppedScreen;

  Q_EMIT screensChanged();

  return true;
}

void ScreenSetupModel::addScreen(const Screen &newScreen)
{
  m_Screens.addScreenByPriority(newScreen);
  Q_EMIT screensChanged();
}

bool ScreenSetupModel::trySetSpan(int column, int row, int width, int height)
{
  auto &target = screen(column, row);
  if (target.isNull() || (width == target.width() && height == target.height()))
    return false;

  ScreenList trial = m_Screens;
  auto &trialScreen = trial[row * m_NumColumns + column];
  trialScreen.setWidth(width);
  trialScreen.setHeight(height);
  if (!isValidLayout(trial, m_NumColumns))
    return false;

  target.setWidth(width);
  target.setHeight(height);
  Q_EMIT screensChanged();
  return true;
}

bool ScreenSetupModel::isFull() const
{
  for (int i = 0; i < m_Screens.size(); i++) {
    if (m_Screens.screenIndexAt(i % m_NumColumns, i / m_NumColumns) == -1)
      return false;
  }
  return true;
}
