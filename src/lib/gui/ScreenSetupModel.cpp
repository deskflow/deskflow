/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ScreenSetupModel.h"

#include <QIODevice>
#include <QMimeData>

#include "gui/config/Screen.h"
#include "gui/widgets/MonitorLayoutWidget.h"

const QString ScreenSetupModel::m_MimeType = "application/x-deskflow-screen";

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
    return {};

  if (screen(index).isNull())
    return {};

  switch (role) {
  case Qt::DecorationRole: {
    const auto &monitors = screen(index).monitors();
    
    qDebug() << "DecorationRole for screen" << screen(index).name() 
             << "- monitors:" << monitors.size();

    if (monitors.size() > 1) {
      qDebug() << "Creating multi-monitor pixmap for" << screen(index).name();
      QPixmap pixmap(96, 96);
      pixmap.fill(Qt::transparent);
      QPainter painter(&pixmap);
      painter.setRenderHint(QPainter::Antialiasing);
      MonitorLayoutWidget tempWidget;
      tempWidget.setMonitors(monitors);
      tempWidget.resize(96, 96);
      tempWidget.render(&painter);
      
      return pixmap;
    }

    qDebug() << "Using default icon for" << screen(index).name();
    return screen(index).pixmap();
  }

  case Qt::ToolTipRole: {
    QString tooltip = QString(tr("<center>Screen: <b>%1</b></center>"))
        .arg(screen(index).name());

    if (const auto &monitors = screen(index).monitors(); !monitors.isEmpty()) {
      tooltip += QString(tr("<br><b>Monitors:</b> %1")).arg(monitors.size());
      for (const auto &monitor : monitors) {
        tooltip += QString(tr("<br>  • %1: %2x%3 at (%4, %5)%6"))
            .arg(monitor.name)
            .arg(monitor.geometry.width())
            .arg(monitor.geometry.height())
            .arg(monitor.geometry.x())
            .arg(monitor.geometry.y())
            .arg(monitor.isPrimary ? tr(" (primary)") : "");
      }
    }
    
    tooltip += tr("<br>Double click to edit settings"
                  "<br>Drag screen to the trashcan to remove it");
    return tooltip;
  }

  case Qt::DisplayRole: {
    return screen(index).name();
  }

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

  const auto pColumn = parent.column();
  const auto pRow = parent.row();

  // don't drop screen onto itself
  if (sourceColumn == pColumn && sourceRow == pRow)
    return false;

  Screen droppedScreen;
  stream >> droppedScreen;

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

bool ScreenSetupModel::isFull() const
{
  auto emptyScreen = std::ranges::find_if(m_Screens, [](const Screen &item) { return item.isNull(); });
  return (static_cast<QList<Screen>::const_iterator>(emptyScreen) == m_Screens.cend());
}
