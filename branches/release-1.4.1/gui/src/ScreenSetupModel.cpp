#include "ScreenSetupModel.h"
#include "Screen.h"

#include <QtCore>
#include <QtGui>

const QString ScreenSetupModel::m_MimeType = "application/x-qsynergy-screen";

ScreenSetupModel::ScreenSetupModel(ScreenList& screens, int numColumns, int numRows) :
	QAbstractTableModel(NULL),
	m_Screens(screens),
	m_NumColumns(numColumns),
	m_NumRows(numRows)
{
	if (m_NumColumns * m_NumRows > screens.size())
		qFatal("Not enough elements (%u) in screens QList for %d columns and %d rows", screens.size(), m_NumColumns, m_NumRows);
}

QVariant ScreenSetupModel::data(const QModelIndex& index, int role) const
{
	if (index.isValid() && index.row() < m_NumRows && index.column() < m_NumColumns)
	{
		switch(role)
		{
			case Qt::DecorationRole:
				if (screen(index).isNull())
					break;
				return QIcon(*screen(index).pixmap());

			case Qt::ToolTipRole:
				if (screen(index).isNull())
					break;
				return QString(tr(
							"<center>Screen: <b>%1</b></center>"
							"<br>Double click to edit settings"
							"<br>Drag screen to the trashcan to remove it")).arg(screen(index).name());

			case Qt::DisplayRole:
				if (screen(index).isNull())
					break;
				return screen(index).name();
		}
	}

	return QVariant();
}

Qt::ItemFlags ScreenSetupModel::flags(const QModelIndex& index) const
{
	if (!index.isValid() || index.row() >= m_NumRows || index.column() >= m_NumColumns)
		return 0;

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

QMimeData* ScreenSetupModel::mimeData(const QModelIndexList& indexes) const
{
	QMimeData* pMimeData = new QMimeData();
	QByteArray encodedData;

	QDataStream stream(&encodedData, QIODevice::WriteOnly);

	foreach (const QModelIndex& index, indexes)
		if (index.isValid())
			stream << index.column() << index.row() << screen(index);

	pMimeData->setData(m_MimeType, encodedData);

	return pMimeData;
}

bool ScreenSetupModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
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

	// don't drop screen onto itself
	if (sourceColumn == parent.column() && sourceRow == parent.row())
		return false;

	Screen droppedScreen;
	stream >> droppedScreen;

	Screen oldScreen = screen(parent.column(), parent.row());
	if (!oldScreen.isNull() && sourceColumn != -1 && sourceRow != -1)
	{
		// mark the screen so it isn't deleted after the dragndrop succeeded
		// see ScreenSetupView::startDrag()
		oldScreen.setSwapped(true);
		screen(sourceColumn, sourceRow) = oldScreen;
	}

	screen(parent.column(), parent.row()) = droppedScreen;

	return true;
}

