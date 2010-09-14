#include "NewScreenWidget.h"
#include "ScreenSetupModel.h"

#include <QtCore>
#include <QtGui>

NewScreenWidget::NewScreenWidget(QWidget* parent) :
	QLabel(parent)
{
}

void NewScreenWidget::mousePressEvent(QMouseEvent* event)
{
	Screen newScreen(tr("Unnamed"));

	QByteArray itemData;
	QDataStream dataStream(&itemData, QIODevice::WriteOnly);
	dataStream << -1 << -1 << newScreen;

	QMimeData* pMimeData = new QMimeData;
	pMimeData->setData(ScreenSetupModel::mimeType(), itemData);

	QDrag* pDrag = new QDrag(this);
	pDrag->setMimeData(pMimeData);
	pDrag->setPixmap(*pixmap());
	pDrag->setHotSpot(event->pos());

	pDrag->exec(Qt::CopyAction, Qt::CopyAction);
}

