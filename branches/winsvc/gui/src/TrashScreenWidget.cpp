#include "TrashScreenWidget.h"
#include "ScreenSetupModel.h"

#include <QtCore>
#include <QtGui>

void TrashScreenWidget::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasFormat(ScreenSetupModel::mimeType()))
	{
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}
	else
		event->ignore();
}

void TrashScreenWidget::dropEvent(QDropEvent* event)
{
	if (event->mimeData()->hasFormat(ScreenSetupModel::mimeType()))
		event->acceptProposedAction();
	else
		event->ignore();
}

