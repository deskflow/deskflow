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
#include <QSize>

#include <cstddef>
#include <vector>

#include "gui/config/Screen.h"

const QString ScreenSetupModel::m_MimeType = "application/x-deskflow-screen";

namespace {

// a layout is valid when every screen stays inside the grid and no two screens
// claim the same cell. Occupancy has to be tracked across all screens, not
// checked per-span: a wide screen and a tall screen can cross on a cell that is
// a covered (raw-empty) cell of both, so a per-span "is this raw cell empty"
// test would miss the overlap.
bool isValidLayout(const ScreenList &screens, int numColumns)
{
  const int numRows = static_cast<int>(screens.size()) / numColumns;
  std::vector<bool> occupied(static_cast<std::size_t>(numColumns) * numRows, false);
  for (int i = 0; i < screens.size(); i++) {
    const auto &screen = screens[i];
    if (screen.isNull())
      continue;

    const int column = i % numColumns;
    const int row = i / numColumns;
    if (column + screen.width() > numColumns || row + screen.height() > numRows)
      return false;

    for (int r = row; r < row + screen.height(); r++) {
      for (int c = column; c < column + screen.width(); c++) {
        const std::size_t cell = static_cast<std::size_t>(r) * numColumns + c;
        if (occupied[cell])
          return false;
        occupied[cell] = true;
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

  case SpanRole:
    return QSize(screen(index).width(), screen(index).height());

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

  // build the resulting layout and apply it atomically. The dropped screen
  // takes the target as its new anchor; the moved screen's whole footprint is
  // lifted first (via its anchor) so a spanning screen can shift by a single
  // cell or into a corner -- targets that overlap its own previous cells. Any
  // screen covering the target relocates onto the freed source anchor (a 1:1
  // swap). The exact state is validated, so a rejected drop leaves the grid
  // untouched.
  const int targetIndex = pRow * m_NumColumns + pColumn;
  ScreenList trial = m_Screens;
  if (hasSource) {
    const int sourceIndex = sourceRow * m_NumColumns + sourceColumn;
    trial[sourceIndex] = Screen();
    if (const int occupantIndex = m_Screens.screenIndexAt(pColumn, pRow);
        occupantIndex != -1 && occupantIndex != sourceIndex) {
      trial[occupantIndex] = Screen();
      trial[sourceIndex] = m_Screens[occupantIndex];
    }
  }
  trial[targetIndex] = droppedScreen;
  if (!isValidLayout(trial, m_NumColumns))
    return false;

  m_Screens = trial;
  m_dropHandled = true;

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
